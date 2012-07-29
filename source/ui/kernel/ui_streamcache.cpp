/*
Copyright (C) 2011 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_streamcache.h"
#include <time.h>

namespace WSWUI
{

AsyncStream::AsyncStream() : privatep(NULL), key(""), tmpFilename(""), tmpFilenum(0), noCache(false)
{
}

// ======================================================

StreamCache::StreamCache()
{
	streams.clear();

	ui_cachepurgedate = trap::Cvar_Get( "ui_cachepurgedate", "", CVAR_ARCHIVE );
}

void StreamCache::Init()
{
	bool cacheEmpty = true;

	// remove the cache dir on the 1st and 15nth days of month
	time_t long_time;
	struct tm *nt;

	// date & time
	time( &long_time );

	// purge cache on the given date
	if( ui_cachepurgedate->string[0] != '\0') {
		int year, month, day;

		cacheEmpty = false;
		if( sscanf( ui_cachepurgedate->string, "%i-%i-%i", &year, &month, &day ) == 3 ) {
			struct tm purgetime;

			memset( &purgetime, 0, sizeof( purgetime ) );
			purgetime.tm_year = year - 1900;
			purgetime.tm_mon = month - 1;
			purgetime.tm_mday = day;

			if( long_time >= mktime( &purgetime ) ) {
				PurgeCache();
				cacheEmpty = true;
			}
		}
	}

	// set next date & time
	if( cacheEmpty ) {
		long_time += time_t( WSW_UI_STREAMCACHE_CACHE_PURGE_INTERVAL ) * 24 * 60 * 60;
		nt = localtime( &long_time );
		trap::Cvar_ForceSet( ui_cachepurgedate->name, va( "%04i-%02i-%02i", nt->tm_year + 1900, nt->tm_mon+1, nt->tm_mday ) );
	}
}

void StreamCache::Shutdown()
{
}

void StreamCache::PurgeCache( void )
{
	std::string cacheDir( WSW_UI_STREAMCACHE_DIR );

	std::vector<std::string> cachedFiles;
	getFileList( cachedFiles, cacheDir, "*", true );

	for( std::vector<std::string>::const_iterator it = cachedFiles.begin(); it != cachedFiles.end(); ++it ) {
		trap::FS_RemoveFile( (cacheDir + "/" + *it).c_str() );
	}
}

size_t StreamCache::StreamRead( const void *buf, size_t numb, float percentage, const char *contentType, void *privatep )
{
	AsyncStream *stream;

	stream = ( AsyncStream * )privatep;

	if( stream->read_cb ) {
		return stream->read_cb( buf, numb, percentage, contentType, stream->privatep );
	}
	else if( stream->cache_cb ) {
		// write to temporary cache file
		return trap::FS_Write( buf, numb, stream->tmpFilenum );
	}

	// undefined
	return 0;
}

void StreamCache::StreamDone( int status, const char *contentType, void *privatep )
{
	AsyncStream *stream;

	stream = ( AsyncStream * )privatep;

	if( stream->done_cb ) {
		stream->done_cb( status, contentType, stream->privatep );
		__delete__( stream );
	}
	else if( stream->cache_cb ) {
		std::string &tmpFile = stream->tmpFilename;
		std::string _contentType = "", realFile;

		// strip off temporary extension and optionally force extension by mime type
		if( contentType && *contentType ) {
			_contentType = std::string( contentType );
		}
		realFile = stream->parent->RealFileForCacheFile( tmpFile.substr( 0, tmpFile.size() - strlen( WSW_UI_STREAMCACHE_EXT ) ), _contentType );

		// close the temp file, that'll also flush yet unwritten data to disk
		trap::FS_FCloseFile( stream->tmpFilenum );

		// remove the target file so that a new one can be moved in its place
		trap::FS_RemoveFile( realFile.c_str() );

		bool moved = false;
		if( status == HTTP_CODE_OK ) {
			// verify that the move succeeds
			moved = ( trap::FS_MoveFile( tmpFile.c_str(), realFile.c_str() ) == qtrue );
		}
		else {
			Com_Printf( S_COLOR_YELLOW "StreamCache::StreamDone: error %i fetching '%s'\n", status, stream->url.c_str() );

			// remove the temp file
			trap::FS_RemoveFile( tmpFile.c_str() );
		}

		// this is also going to delete the stream object
		stream->parent->CallCacheCbByStreamKey( stream->key, realFile, moved );

		// remove the file if caching is disabled
		// NOTE: this breaks lazy texture loading
		if( stream->noCache ) {
			// trap::FS_RemoveFile( realFile.c_str() );
		}
	}
	else {
		// undefined
		__delete__( stream );
	}
}

void StreamCache::CallCacheCbByStreamKey( const std::string &key, const std::string &fileName, bool success )
{
	StreamList &list = streams[key];

	// for all streams marked by the same key, fire the cache callback in case of success
	// then release them
	for( StreamList::iterator it = list.begin(); it != list.end(); it++ ) {
		AsyncStream *stream = *it;

		// only fire the callback in case of success (that means the cache file actually exists)
		if( success ) {
			stream->cache_cb( fileName.c_str(), stream->privatep );
		}

		__delete__( stream );
	}

	list.clear();
}

void StreamCache::PerformRequest( const char *url, const char *method, const char *data,
	ui_async_stream_read_cb_t read_cb, ui_async_stream_done_cb_t done_cb, stream_cache_cb cache_cb,
	void *privatep, int timeout, int cacheTTL )
{
	std::string cacheFilename, tmpFilename;
	bool noCache = cacheTTL == 0;

	cacheFilename = CacheFileForUrl( url, noCache );
	tmpFilename = cacheFilename + WSW_UI_STREAMCACHE_EXT;

	// check in cache first
	if( cache_cb ) {
		// redundant check
		//if( trap::FS_FOpenFile( cacheFilename.c_str(), NULL, FS_READ ) >= 0 )
		{
			time_t mTime;

			// examine last modified datetime for the cache file
			// note, that mTime is -1 for non-existing files
			// or 0 if mTime could not be obtained)
			mTime = trap::FS_FileMTime( cacheFilename.c_str() );
			if( mTime + cacheTTL * 60 > time( NULL ) ) {
				cache_cb( cacheFilename.c_str(), privatep );
				return;
			}
			else {
				// Com_Printf( "Cached expired for %s: %i\n", url, mTime );
			}
		}
	}

	// allocate a new stream
	AsyncStream *stream;
	stream = __new__( AsyncStream );
	stream->url = url;
	stream->privatep = privatep;
	stream->read_cb = read_cb;
	stream->done_cb = done_cb;
	stream->cache_cb = cache_cb;
	stream->parent = this;
	stream->noCache = noCache;

	// track cached streams by key so we don't fire multiple async requests
	// for the same URL. When the first request with this key completes, it'll
	// fire cache callbacks for other streams with the same key
	if( cache_cb ) {
		bool inProgress;

		std::string &cacheKey = cacheFilename;
		stream->key = cacheKey;

		// check whether there's already at least one stream with the same key
		inProgress = streams[cacheKey].size() > 0;
		if( !inProgress ) {
			stream->tmpFilename = tmpFilename;

			if( trap::FS_FOpenFile( tmpFilename.c_str(), &stream->tmpFilenum, FS_WRITE ) < 0 ) {
				Com_Printf( S_COLOR_YELLOW "WARNING: Failed to open %s for writing\n", tmpFilename.c_str() );
				__delete__( stream );
				return;
			}
		}

		streams[cacheKey].push_back( stream );
		if( inProgress ) {
			return;
		}
	}

	// fire the async request
	trap::AsyncStream_PerformRequest(
		url, method, data, timeout,
		&StreamRead, &StreamDone, ( void * )stream
	);
}

std::string StreamCache::CacheFileForUrl( const std::string url, bool noCache )
{
	unsigned int hashkey;
	std::string fileName;

	// compute hash key for the URL and convert to hex
	hashkey = trap::Hash_BlockChecksum( ( const qbyte * )url.c_str(), url.size() );
	std::stringstream outstream;
	outstream << std::hex << hashkey;	// to hex

	// strip path and query string from the target file name
	fileName = std::string( COM_FileBase( url.c_str() ) );
	std::string::size_type delim = fileName.find( '?' );
	if( delim != fileName.npos ) {
		fileName = fileName.substr( 0, delim );
	}

	return std::string( WSW_UI_STREAMCACHE_DIR ) + "/" + outstream.str() + (noCache ? "_0" : "_1") + "_" + fileName;
}

std::string StreamCache::RealFileForCacheFile( const std::string cacheFile, const std::string contentType )
{
	std::string realFile = cacheFile;

	// force extensions for some known mime types because renderer has no idea about mime types
	// FIXME: this breaks caching due to cache file name mismatch at the start of PerformRequest
	if( contentType != "" ) {
		std::string forceExtension( "" );

		if( contentType == "image/x-tga" ) {
			forceExtension = ".tga";
		}
		else if( contentType == "image/jpeg" || contentType == "image/jpg" ) {
			forceExtension = ".jpg";
		}
		else if( contentType == "image/pcx" ) {
			forceExtension = ".pcx";
		}
		else if( contentType == "image/png" ) {
			forceExtension = ".png";
		}

		// remove existing extension (if any), append forced extension
		if( forceExtension != "" ) {
			std::string::size_type dot = realFile.rfind( '.' );
			std::string::size_type slash = realFile.rfind( '/' );
			if( dot != realFile.npos && ( slash == realFile.npos || dot > slash ) ) {
				realFile = realFile.substr( 0, dot );
			}
			realFile += forceExtension;
		}
	}

	return realFile;
}

}
