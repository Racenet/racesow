/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "qcommon.h"

#if defined ( __MACOSX__ )
#include <arpa/inet.h>
#endif

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	game port

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher than the last reliable sequence, but without the correct even/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted. It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

If the sequence number is -1, the packet should be handled without a netcon.

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are always placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection. This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.


The game port field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the game port matches, then the
channel matches even if the IP port differs. The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/

// application level protocol port number, to differentiate multiple clients
// from the same IP address, and still work even if client's UDP port number suddenly changes
static int game_port;

static cvar_t *showpackets;
static cvar_t *showdrop;
static cvar_t *net_showfragments;

/*
* Netchan_OutOfBand
* 
* Sends an out-of-band datagram
*/
void Netchan_OutOfBand( const socket_t *socket, const netadr_t *address, size_t length, const qbyte *data )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );

	MSG_WriteLong( &send, -1 ); // -1 sequence means out of band
	MSG_WriteData( &send, data, length );

	// send the datagram
	if( !NET_SendPacket( socket, send.data, send.cursize, address ) )
		Com_Printf( "NET_SendPacket: Error: %s\n", NET_ErrorString() );
}

/*
* Netchan_OutOfBandPrint
* 
* Sends a text message in an out-of-band datagram
*/
void Netchan_OutOfBandPrint( const socket_t *socket, const netadr_t *address, const char *format, ... )
{
	va_list	argptr;
	static char string[MAX_PACKETLEN - 4];

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	Netchan_OutOfBand( socket, address, sizeof( char ) * (int)strlen( string ), (qbyte *)string );
}

/*
* Netchan_Setup
* 
* called to open a channel to a remote system
*/
void Netchan_Setup( netchan_t *chan, const socket_t *socket, const netadr_t *address, int game_port )
{
	memset( chan, 0, sizeof( *chan ) );

	chan->socket = socket;
	chan->remoteAddress = *address;
	chan->game_port = game_port;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
}


static qbyte msg_process_data[MAX_MSGLEN];

//=============================================================
// Zlib compression
//=============================================================

#include "zlib.h"

#ifdef ALT_ZLIB_COMPRESSION
/*
http://www.zlib.net/manual.html#compress2
int compress2 (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
Compresses the source buffer into the destination buffer. The level parameter has the same meaning
as in deflateInit. sourceLen is the byte length of the source buffer. Upon entry, destLen is the
total size of the destination buffer, which must be at least 0.1% larger than sourceLen plus 12 bytes.
Upon exit, destLen is the actual size of the compressed buffer.

compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough memory, Z_BUF_ERROR if there
was not enough room in the output buffer, Z_STREAM_ERROR if the level parameter is invalid.
*/
static int Netchan_ZLibCompressChunk( const qbyte *source, unsigned long sourceLen, qbyte *dest, unsigned long destLen,
									 int level, int wbits )
{
	int result, zlerror;

	zlerror = compress2( dest, &destLen, source, sourceLen, level );
	switch( zlerror )
	{
	case Z_OK:
		result = destLen; // returns the new length into destLen
		break;
	case Z_MEM_ERROR:
		Com_DPrintf( "ZLib data error! Z_MEM_ERROR on compress.\n" );
		result = -1;
		break;
	case Z_BUF_ERROR:
		Com_DPrintf( "ZLib data error! Z_BUF_ERROR on compress.\n" );
		result = -1;
		break;
	case Z_STREAM_ERROR:
		Com_DPrintf( "ZLib data error! Z_STREAM_ERROR on compress.\n" );
		result = -1;
		break;
	default:
		Com_DPrintf( "ZLib data error! Error code %i on compress.\n", zlerror );
		result = -1;
		break;
	}

	return result;
}
/*
http://www.zlib.net/manual.html#uncompress
int uncompress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
Decompresses the source buffer into the destination buffer. sourceLen is the byte length
of the source buffer. Upon entry, destLen is the total size of the destination buffer,
which must be large enough to hold the entire uncompressed data. (The size of the uncompressed
data must have been saved previously by the compressor and transmitted to the decompressor
by some mechanism outside the scope of this compression library.) Upon exit, destLen is the
actual size of the compressed buffer.

This function can be used to decompress a whole file at once if the input file is mmap'ed.

uncompress returns Z_OK if success, Z_MEM_ERROR if there was not enough memory, Z_BUF_ERROR if
there was not enough room in the output buffer, or Z_DATA_ERROR if the input data was corrupted.
*/
static int Netchan_ZLibDecompressChunk( const qbyte *source, unsigned long sourceLen, qbyte *dest, unsigned long destLen,
									   int wbits )
{
	int result, zlerror;

	zlerror = uncompress( dest, &destLen, source, sourceLen );
	switch( zlerror )
	{
	case Z_OK:
		result = destLen; // returns the new length into destLen
		break;
	case Z_MEM_ERROR:
		Com_DPrintf( "ZLib data error! Z_MEM_ERROR on decompress.\n" );
		result = -1;
		break;
	case Z_BUF_ERROR:
		Com_DPrintf( "ZLib data error! Z_BUF_ERROR on decompress.\n" );
		result = -1;
		break;
	case Z_DATA_ERROR:
		Com_DPrintf( "ZLib data error! Z_DATA_ERROR on decompress.\n" );
		result = -1;
		break;
	default:
		Com_DPrintf( "ZLib data error! Error code %i on decompress.\n", zlerror );
		result = -1;
		break;
	}

	return result;
}
#else // ALT_ZLIB_COMPRESSION

int Netchan_ZLibDecompressChunk( qbyte *in, int inlen, qbyte *out, int outlen, int wbits )
{
	z_stream zs;
	int result;

	memset( &zs, 0, sizeof( zs ) );

	zs.next_in = in;
	zs.avail_in = 0;

	zs.next_out = out;
	zs.avail_out = outlen;

	result = inflateInit2( &zs, wbits );
	if( result != Z_OK )
	{
		Com_DPrintf( "ZLib data error! Error %d on inflateInit.\nMessage: %s", result, zs.msg );
		return result;
	}

	zs.avail_in = inlen;

	result = inflate( &zs, Z_FINISH );
	if( result != Z_STREAM_END )
	{
		Com_DPrintf( "ZLib data error! Error %d on inflate.\nMessage: %s", result, zs.msg );
		zs.total_out = 0;
		return -1;
	}

	result = inflateEnd( &zs );
	if( result != Z_OK )
	{
		Com_DPrintf( "ZLib data error! Error %d on inflateEnd.\nMessage: %s", result, zs.msg );
		return -1;
	}

	return zs.total_out;
}

int Netchan_ZLibCompressChunk( qbyte *in, int len_in, qbyte *out, int max_len_out, int method, int wbits )
{
	z_stream zs;
	int result;

	zs.next_in = in;
	zs.avail_in = len_in;
	zs.total_in = 0;

	zs.next_out = out;
	zs.avail_out = max_len_out;
	zs.total_out = 0;

	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;

	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	result = deflateInit2( &zs, method, Z_DEFLATED, wbits, 9, Z_DEFAULT_STRATEGY );
	if( result != Z_OK )
		return -1;

	result = deflate( &zs, Z_FINISH );
	if( result != Z_STREAM_END )
		return -1;

	result = deflateEnd( &zs );
	if( result != Z_OK )
		return -1;

	return zs.total_out;
}

#endif // ALT_ZLIB_COMPRESSION

/*
* Netchan_CompressMessage
*/
int Netchan_CompressMessage( msg_t *msg )
{
	int length;

	if( msg == NULL || !msg->data )
		return 0;

	// zero-fill our buffer
	length = 0;
	memset( msg_process_data, 0, sizeof( msg_process_data ) );

	//compress the message
	length = Netchan_ZLibCompressChunk( msg->data, msg->cursize, msg_process_data, sizeof( msg_process_data ), Z_DEFAULT_COMPRESSION, -MAX_WBITS );
	if( length < 0 )  // failed to compress, return the error
		return length;

	if( (size_t)length >= msg->cursize || length >= MAX_MSGLEN )
	{
		return 0; // compressed was bigger. Send uncompressed
	}

	//write it back into the original container
	MSG_Clear( msg );
	MSG_CopyData( msg, msg_process_data, length );
	msg->compressed = qtrue;

	return length; // return the new size
}

/*
* Netchan_DecompressMessage
*/
int Netchan_DecompressMessage( msg_t *msg )
{
	int length;

	if( msg == NULL || !msg->data )
		return 0;

	if( msg->compressed == qfalse )
		return 0;

	length = Netchan_ZLibDecompressChunk( msg->data + msg->readcount, msg->cursize - msg->readcount, msg_process_data, ( sizeof( msg_process_data ) - msg->readcount ), -MAX_WBITS );
	if( length < 0 )
		return length;

	if( ( msg->readcount + length ) >= msg->maxsize )
	{
		Com_Printf( "Netchan_DecompressMessage: Packet too big\n" );
		return -1;
	}

	//write it back into the original container
	msg->cursize = msg->readcount;
	MSG_CopyData( msg, msg_process_data, length );
	msg->compressed = qfalse;

	return length;
}

/*
* Netchan_DropAllFragments
* 
* Send all remaining fragments at once
*/
static void Netchan_DropAllFragments( netchan_t *chan )
{
	if( chan->unsentFragments )
	{
		chan->outgoingSequence++;
		chan->unsentFragments = qfalse;
	}
}

/*
* Netchan_TransmitNextFragment
* 
* Send one fragment of the current message
*/
qboolean Netchan_TransmitNextFragment( netchan_t *chan )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];
	int fragmentLength;
	qboolean last;

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );
	MSG_Clear( &send );

	if( net_showfragments->integer )
		Com_Printf( "Transmit fragment (%s) (id:%i)\n", NET_SocketToString( chan->socket ), chan->outgoingSequence );

	MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );
	// wsw : jal : by now our header sends incoming ack too (q3 doesn't)
	// wsw : also add compressed bit if it's compressed
	if( chan->unsentIsCompressed )
		MSG_WriteLong( &send, chan->incomingSequence | FRAGMENT_BIT );
	else
		MSG_WriteLong( &send, chan->incomingSequence );

	// send the game port if we are a client
	if( !chan->socket->server )
		MSG_WriteShort( &send, game_port );

	// copy the reliable message to the packet first
	if( chan->unsentFragmentStart + FRAGMENT_SIZE > chan->unsentLength )
	{
		fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
		last = qtrue;
	}
	else
	{
		fragmentLength = ceil( ( chan->unsentLength - chan->unsentFragmentStart )*1.0 / ceil( ( chan->unsentLength - chan->unsentFragmentStart )*1.0 / FRAGMENT_SIZE ) );
		last = qfalse;
	}

	MSG_WriteShort( &send, chan->unsentFragmentStart );
	MSG_WriteShort( &send, ( last ? ( fragmentLength | FRAGMENT_LAST ) : fragmentLength ) );
	MSG_CopyData( &send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );

	// send the datagram
	if( !NET_SendPacket( chan->socket, send.data, send.cursize, &chan->remoteAddress ) )
	{
		Netchan_DropAllFragments( chan );
		return qfalse;
	}

	if( showpackets->integer )
	{
		Com_Printf( "%s send %4i : s=%i fragment=%i,%i\n", NET_SocketToString( chan->socket ), send.cursize,
			chan->outgoingSequence, chan->unsentFragmentStart, fragmentLength );
	}

	chan->unsentFragmentStart += fragmentLength;

	// this exit condition is a little tricky, because a packet
	// that is exactly the fragment length still needs to send
	// a second packet of zero length so that the other side
	// can tell there aren't more to follow
	if( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE )
	{
		chan->outgoingSequence++;
		chan->unsentFragments = qfalse;
	}

	return qtrue;
}

/*
* Netchan_PushAllFragments
* 
* Send all remaining fragments at once
*/
qboolean Netchan_PushAllFragments( netchan_t *chan )
{
	while( chan->unsentFragments )
	{
		if( !Netchan_TransmitNextFragment( chan ) )
			return qfalse;
	}

	return qtrue;
}

/*
* Netchan_Transmit
* 
* Sends a message to a connection, fragmenting if necessary
* A 0 length will still generate a packet.
*/
qboolean Netchan_Transmit( netchan_t *chan, msg_t *msg )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];

	assert( msg );

	if( msg->cursize > MAX_MSGLEN )
	{
		Com_Error( ERR_DROP, "Netchan_Transmit: Excessive length = %i", msg->cursize );
		return qfalse;
	}
	chan->unsentFragmentStart = 0;
	chan->unsentIsCompressed = qfalse;

	// fragment large reliable messages
	if( msg->cursize >= FRAGMENT_SIZE )
	{
		chan->unsentFragments = qtrue;
		chan->unsentLength = msg->cursize;
		chan->unsentIsCompressed = msg->compressed;
		memcpy( chan->unsentBuffer, msg->data, msg->cursize );

		// only send the first fragment now
		return Netchan_TransmitNextFragment( chan );
	}

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );
	MSG_Clear( &send );

	MSG_WriteLong( &send, chan->outgoingSequence );
	// wsw : jal : by now our header sends incoming ack too (q3 doesn't)
	// wsw : jal : also add compressed information if it's compressed
	if( msg->compressed )
		MSG_WriteLong( &send, chan->incomingSequence | FRAGMENT_BIT );
	else
		MSG_WriteLong( &send, chan->incomingSequence );

	chan->outgoingSequence++;

	// send the game port if we are a client
	if( !chan->socket->server )
		MSG_WriteShort( &send, game_port );

	MSG_CopyData( &send, msg->data, msg->cursize );

	// send the datagram
	if( !NET_SendPacket( chan->socket, send.data, send.cursize, &chan->remoteAddress ) )
		return qfalse;

	if( showpackets->integer )
	{
		Com_Printf( "%s send %4i : s=%i ack=%i\n", NET_SocketToString( chan->socket ), send.cursize,
			chan->outgoingSequence - 1, chan->incomingSequence );
	}

	return qtrue;
}

/*
* Netchan_Process
* 
* Returns qfalse if the message should not be processed due to being
* out of order or a fragment.
* 
* Msg must be large enough to hold MAX_MSGLEN, because if this is the
* final fragment of a multi-part message, the entire thing will be
* copied out.
*/
qboolean Netchan_Process( netchan_t *chan, msg_t *msg )
{
	int sequence, sequence_ack;
	int game_port = -1;
	int fragmentStart, fragmentLength;
	qboolean fragmented = qfalse;
	int headerlength;
	qboolean compressed = qfalse;
	qboolean lastfragment = qfalse;

	// get sequence numbers
	MSG_BeginReading( msg );
	sequence = MSG_ReadLong( msg );
	sequence_ack = MSG_ReadLong( msg ); // wsw : jal : by now our header sends incoming ack too (q3 doesn't)

	// check for fragment information
	if( sequence & FRAGMENT_BIT )
	{
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;

		if( net_showfragments->integer )
			Com_Printf( "Process fragmented packet (%s) (id:%i)\n", NET_SocketToString( chan->socket ), sequence );
	}
	else
	{
		fragmented = qfalse;
	}

	// wsw : jal : check for compressed information
	if( sequence_ack & FRAGMENT_BIT )
	{
		sequence_ack &= ~FRAGMENT_BIT;
		compressed = qtrue;
		if( !fragmented )
			msg->compressed = qtrue;
	}

	// read the game port if we are a server
	if( chan->socket->server )
		game_port = MSG_ReadShort( msg );

	// read the fragment information
	if( fragmented )
	{
		fragmentStart = MSG_ReadShort( msg );
		fragmentLength = MSG_ReadShort( msg );
		if( fragmentLength & FRAGMENT_LAST )
		{
			lastfragment = qtrue;
			fragmentLength &= ~FRAGMENT_LAST;
		}
	}
	else
	{
		fragmentStart = 0; // stop warning message
		fragmentLength = 0;
	}

	if( showpackets->integer )
	{
		if( fragmented )
		{
			Com_Printf( "%s recv %4i : s=%i fragment=%i,%i\n", NET_SocketToString( chan->socket ), msg->cursize,
				sequence, fragmentStart, fragmentLength );
		}
		else
		{
			Com_Printf( "%s recv %4i : s=%i\n", NET_SocketToString( chan->socket ), msg->cursize, sequence );
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if( sequence <= chan->incomingSequence )
	{
		if( showdrop->integer || showpackets->integer )
		{
			Com_Printf( "%s:Out of order packet %i at %i\n", NET_AddressToString( &chan->remoteAddress ), sequence,
				chan->incomingSequence );
		}
		return qfalse;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - ( chan->incomingSequence+1 );
	if( chan->dropped > 0 )
	{
		if( showdrop->integer || showpackets->integer )
		{
			Com_Printf( "%s:Dropped %i packets at %i\n", NET_AddressToString( &chan->remoteAddress ), chan->dropped,
				sequence );
		}
	}

	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence
	//
	if( fragmented )
	{
		// TTimo
		// make sure we add the fragments in correct order
		// either a packet was dropped, or we received this one too soon
		// we don't reconstruct the fragments. we will wait till this fragment gets to us again
		// (NOTE: we could probably try to rebuild by out of order chunks if needed)
		if( sequence != chan->fragmentSequence )
		{
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if( fragmentStart != (int) chan->fragmentLength )
		{
			if( showdrop->integer || showpackets->integer )
			{
				Com_Printf( "%s:Dropped a message fragment\n", NET_AddressToString( &chan->remoteAddress ), sequence );
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) )
		{
			if( showdrop->integer || showpackets->integer )
			{
				Com_Printf( "%s:illegal fragment length\n", NET_AddressToString( &chan->remoteAddress ) );
			}
			return qfalse;
		}

		memcpy( chan->fragmentBuffer + chan->fragmentLength, msg->data + msg->readcount, fragmentLength );

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if( !lastfragment )
		{
			return qfalse;
		}

		if( chan->fragmentLength > msg->maxsize )
		{
			Com_Printf( "%s:fragmentLength %i > msg->maxsize\n", NET_AddressToString( &chan->remoteAddress ),
				chan->fragmentLength );
			return qfalse;
		}

		// wsw : jal : reconstruct the message

		MSG_Clear( msg );
		MSG_WriteLong( msg, sequence );
		MSG_WriteLong( msg, sequence_ack );
		if( chan->socket->server )
			MSG_WriteShort( msg, game_port );

		msg->compressed = compressed;

		headerlength = msg->cursize;
		MSG_CopyData( msg, chan->fragmentBuffer, chan->fragmentLength );
		msg->readcount = headerlength; // put read pointer after header again
		chan->fragmentLength = 0;

		//let it be finished as standard packets
	}

	// the message can now be read from the current message pointer
	chan->incomingSequence = sequence;

	// wsw : jal[start] :  get the ack from the very first fragment
	chan->incoming_acknowledged = sequence_ack;
	// wsw : jal[end]

	return qtrue;
}

/*
* Netchan_GamePort
*/
int Netchan_GamePort( void )
{
	return game_port;
}

/*
* Netchan_Init
*/
void Netchan_Init( void )
{
	// pick a game port value that should be nice and random
	game_port = Sys_Milliseconds() & 0xffff;

	showpackets = Cvar_Get( "showpackets", "0", 0 );
	showdrop = Cvar_Get( "showdrop", "0", 0 );
	net_showfragments = Cvar_Get( "net_showfragments", "0", 0 );
}

/*
* Netchan_Shutdown
*/
void Netchan_Shutdown( void )
{
}
