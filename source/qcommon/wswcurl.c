/*************************************************************************
 *             Easy to use async CURL wrapper for Warsow
 *
 * Author     : Bart Meuris (KoFFiE)
 * E-Mail     : bart.meuris@gmail.com
 *
 * Todo:
 *  - Add wait for download function.
 *  - Add file resuming for downloading maps etc.
 *  - Add timeouts.
 *
 *************************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "wswcurl.h"
#include "qcommon.h"

#ifdef WIN32
# define snprintf				_snprintf
# define strdup					_strdup
#endif


///////////////////////
#define WMALLOC(x)		_Mem_Alloc(wswcurl_mempool, x, 0, 0, __FILE__, __LINE__)
#define WREALLOC(x, y)	( (x) ? Mem_Realloc((x), (y) ) : WMALLOC(y) )
#define WFREE(x)		Mem_Free(x)

// Curl setopt wrapper
#define CURLSETOPT(c,r,o,v) { if (c) { r = curl_easy_setopt(c,o,v); if (r) { printf("\nCURL ERROR: %d: %s\n", r, curl_easy_strerror(r)); curl_easy_cleanup(c) ; c = NULL; } } }
#define CURLDBG(x)			Com_DPrintf x

#define WCONNECTTIMEOUT		0
#define WTIMEOUT			15

// 400k of buffering at max
#define WMAXBUFFERING		102400*4
// 100k at least
#define WMINBUFFERING		102400

// the maximum number of curl_multi handles to be processed simultaneously
#define WMAXMULTIHANDLES	4

#define WSTATUS_NONE		0	// not started
#define WSTATUS_STARTED		1	// started
#define WSTATUS_FINISHED	2	// finished
#define WSTATUS_QUEUED		3	//  queued

///////////////////////
// Typedefs
typedef struct chained_buffer_s {

	size_t rxsize;	// size of buffer
	size_t rxoffset;	// read-head
	struct chained_buffer_s *next;
	char data[1];
} chained_buffer_t;

struct wswcurl_req_s {

	int status; // < 0 : error
	char *url;	// Url of the request

	// List of buffers, 1 for each received block
	// singly-linked list with head+tail
	chained_buffer_t *bhead;
	chained_buffer_t *btail;

	size_t rxreturned;	// Amount of bytes returned to callee
	size_t rxreceived;	// Amount of bytes received from stream
	size_t rx_expsize;	// Expected size of the file, 0 if unknown (this is possible!)

	long respcode;	// HTTP response code when request was handled completely.

	char headers_done;
	char paused;
	time_t last_action;
	time_t timeout;

	// Custom pointer to pass to callback functions below
	void *customp;

	// Callback when done
	wswcurl_done_cb callback_done;
	// Callback when data is available
	wswcurl_read_cb callback_read;
	// Callback when header data is available
	wswcurl_header_cb callback_header;

	CURL *curl;		// Curl handle
	CURLcode res;	// Curl response code

	// Additional header stuff
	struct curl_slist *txhead;

	// Internal form stuff - see http://curl.haxx.se/libcurl/c/curl_formadd.html
	struct curl_httppost *post;
	struct curl_httppost *post_last;

	// Linked list stuff
	struct wswcurl_req_s *next;
	struct wswcurl_req_s *prev;
};

///////////////////////
// Function defines
static int wswcurl_checkmsg();
static size_t wswcurl_write(void *ptr, size_t size, size_t nmemb, void *stream);
static size_t wswcurl_readheader(void *ptr, size_t size, size_t nmemb, void *stream);
static void wswcurl_pause(wswcurl_req *req);
static void wswcurl_unpause(wswcurl_req *req);
static time_t wswcurl_now( void );

///////////////////////
// Local variables
static wswcurl_req *http_requests = NULL; // Linked list of active requests
static wswcurl_req *http_requests_hnode;   // The item node in the list
static CURLM    *curlmulti = NULL;		// Curl MULTI handle
static int curlmulti_num_handles = 0;

static struct mempool_s *wswcurl_mempool;
static CURL		*curldummy = NULL;

static cvar_t *http_proxy;
static cvar_t *http_proxyuserpwd;

int wswcurl_formadd(wswcurl_req *req, const char *field, const char *value, ...)
{
	va_list arg;
	char buf[1024];
	if (!req) return -1;
	if (!field) return -2;
	if (!value) return -3;

	va_start(arg, value);
	vsnprintf(buf, sizeof(buf), value, arg);
	va_end(arg);
	curl_formadd(&req->post, &req->post_last, CURLFORM_COPYNAME, field, CURLFORM_COPYCONTENTS, buf, CURLFORM_END);
	return 0;
}

int wswcurl_formadd_raw(wswcurl_req *req, const char *field, void *data, size_t size )
{
	if (!req) return -1;
	if (!data) return -2;
	if (!size) return -3;

	// TODO: set the Content-type: to some other or just accept base64(URL) encoding here?
	curl_formadd(&req->post, &req->post_last,
					CURLFORM_COPYNAME, field,
					CURLFORM_COPYCONTENTS, data,
					CURLFORM_CONTENTSLENGTH, size,
					CURLFORM_END);
	return 0;
}

int wswcurl_set_postfields( wswcurl_req *req, const char *fields, size_t size )
{
	if( !req ) {
		return -1;
	}
	if( !fields ) {
		return -2;
	}
	if( !size ) {
		return -3;
	}

	// Specify we want to POST data
	curl_easy_setopt( req->curl, CURLOPT_POST, 1 );

	// Set the expected POST size
	curl_easy_setopt( req->curl, CURLOPT_POSTFIELDSIZE, (long)size );

	// Set the POST data
	curl_easy_setopt( req->curl, CURLOPT_POSTFIELDS, fields );

	return 0;
}

void wswcurl_urlencode( const char *src, char *dst, size_t size )
{
	char *curl_esc;

	assert( src );
	assert( dst );

	if( !src || !dst ) {
		return;
	}

	// libcurl needs a curl pointer to be passed to a function that
	// should clearly be "static", how inconvenient...
	if( !curldummy ) {
		curldummy = curl_easy_init();
	}

	curl_esc = curl_easy_escape( curldummy, src, 0 );
	Q_strncpyz( dst, curl_esc, size );
	curl_free( curl_esc );
}

void wswcurl_urlencode_unsafechars( const char *src, char *dst, size_t size )
{
	size_t i, n, len;

	assert( src );
	assert( dst );

	if( !src || !dst || !size ) {
		return;
	}

	len = strlen( src );
	if( len >= size ) {
		len = size - 1;
	}

	// urlencode
	n = 0;
	for( i = 0; i < len && n < size - 1; i++ ) {
		char c = src[i];

		if( c == ' ' || c == '#' || c == '%' || 
			c == '<' || c == '>' || c == '{' || c == '}' || 
			c == '|' || c == '\\' || c == '^' || c == '~' || 
			c == '[' || c == ']' ) {
			// urlencode
			if( n + 3 >= size ) {
				// not enough space
				break;
			}

			dst[n  ] = '%';
			sprintf( &dst[n+1], "%02x", (int)c );
			n += 3;
		}
		else {
			dst[n] = src[i];
			n++;
		}
	}
	dst[n] = '\0';
}

size_t wswcurl_urldecode( const char *src, char *dst, size_t size )
{
	int unesc_len;
	char *curl_unesc;

	assert( src );
	assert( dst );

	if( !src || !dst ) {
		return 0;
	}

	// libcurl needs a curl pointer to be passed to a function that
	// should clearly be "static", how inconvenient...
	if( !curldummy ) {
		curldummy = curl_easy_init();
	}

	curl_unesc = curl_easy_unescape( curldummy, src, 0, &unesc_len );
	Q_strncpyz( dst, curl_unesc, size );
	curl_free( curl_unesc );

	return (size_t)unesc_len;
}

void wswcurl_start(wswcurl_req *req)
{
	CURLcode res;

	if (!req)
		return;

	if (req->status)
		return; // was already started
	if (!req)
		return;
	if (!req->curl)
	{
		wswcurl_delete(req);
		return;
	}

	if (req->txhead)
	{
		CURLSETOPT(req->curl, res, CURLOPT_HTTPHEADER, req->txhead);
	}
	if (req->post)
	{
		CURLSETOPT(req->curl, res, CURLOPT_HTTPPOST, req->post);
		req->post_last = NULL;
	}
	// Initialize multi handle if needed
	if (curlmulti == NULL)
	{
		curlmulti = curl_multi_init();
		if (curlmulti == NULL) {
			CURLDBG(("OOPS: CURL MULTI NULL!!!"));
		}
	}

	req->status = WSTATUS_QUEUED; // queued
}

size_t wswcurl_getsize( wswcurl_req *req, size_t *rxreceived )
{
	while( !req->headers_done && req->status >= 0 && req->status != WSTATUS_FINISHED/* && req->status != WSTATUS_QUEUED*/ ) {
		// blocking read until we finish reading all headers
		CURLDBG(("   CURL BLOCKING GETSIZE LOOP\n"));
		wswcurl_perform_single( req );
	}

	if( rxreceived ) {
		*rxreceived = req->rxreceived;
	}
	if( req->status < 0 ) {
		return 0;
	}

	return req->rx_expsize;
}

void wswcurl_stream_callbacks(wswcurl_req *req, wswcurl_read_cb read_cb, wswcurl_done_cb done_cb, 
							  wswcurl_header_cb header_cb, void *customp)
{
	req->callback_read = read_cb;
	req->callback_done = done_cb;
	req->callback_header = header_cb;
	req->customp = customp;
}

size_t wswcurl_read(wswcurl_req *req, void *buffer, size_t size)
{
	size_t written = 0;
	chained_buffer_t *cb;

	if( (req->rxreceived-req->rxreturned) < (size+WMINBUFFERING) && req->paused )
		wswcurl_unpause(req);

	// Make sure we have data in buffer
	while ( req->status >= 0 && req->status != WSTATUS_FINISHED && req->status != WSTATUS_QUEUED && (req->rxreceived-req->rxreturned) < size ) {
		CURLDBG(("   CURL BLOCKING READ LOOP\n"));
		wswcurl_perform_single (req);
	}

	// hmm, signal an error?
	if( req->status < 0 )
		return 0;

	// go through the buffers in chain, dropping them if not needed
	// start from the beginning (chronological order)
	cb = req->bhead;
	while( cb && written < size )
	{
		long numb = cb->rxsize - cb->rxoffset;
		if( numb + written > size )
			numb = size - written;

		memcpy( ((char*)buffer)+written, cb->data+cb->rxoffset, numb );
		written += numb;
		cb->rxoffset += numb;

		if( cb->rxoffset >= cb->rxsize )
		{
			// flush the buffer away
			req->bhead = cb->next;
			if( req->btail == cb )
				req->btail = NULL;
			WFREE( cb );
			// advance to the next buffer in chain
			cb = req->bhead;
		}
		// else break;
	}

	if( written < size )
		((char*)buffer)[written] = '\0';

	req->rxreturned += written;

#if 0
	if( req->paused)
		Com_Printf(S_COLOR_RED "%d - ", req->rxreturned - req->rxreceived);
	else
		Com_Printf(S_COLOR_CYAN "%d - ", req->rxreturned - req->rxreceived);
#endif

	return written;
}

void wswcurl_init( void )
{
	wswcurl_mempool = Mem_AllocPool( NULL, "Curl" );
	curldummy = curl_easy_init();

	// HTTP proxy settings
	http_proxy = Cvar_Get( "http_proxy", "", CVAR_ARCHIVE );
	http_proxyuserpwd = Cvar_Get( "http_proxyuserpwd", "", CVAR_ARCHIVE );
}

void wswcurl_cleanup( void )
{
	while( http_requests ) {
		wswcurl_delete( http_requests );
	}

	if( curldummy ) {
		curl_easy_cleanup( curldummy );
		curldummy = NULL;
	}

	Mem_FreePool( &wswcurl_mempool );
}

int wswcurl_perform()
{
	int ret = 0;
	wswcurl_req *r, *next;

	if (!curlmulti) return 0;

	// process requests in FIFO manner

	// check for timed out requests and requests that need to be paused
	r = http_requests_hnode;
	while( r )
	{
		next = r->prev;

		if (r->status == WSTATUS_QUEUED) {
			// queued
			if (curlmulti_num_handles < WMAXMULTIHANDLES) {
				if (curl_multi_add_handle(curlmulti, r->curl)) {
					CURLDBG(("OOPS: CURL MULTI ADD HANDLE FAIL!!!"));
				}
				r->status = WSTATUS_STARTED;
				r->last_action = wswcurl_now();
				curlmulti_num_handles++;
			}
			else {
				// stay in queue
			}
		}

		// handle timeouts
		if( r->status == WSTATUS_STARTED ) {
			time_t now = wswcurl_now();

			if( r->paused ) {
				// paused
				r->last_action = now;
			} else if( r->timeout && ( r->last_action + r->timeout <= now ) ) {
				// timed out
				r->respcode = -1;
				r->status = -CURLE_OPERATION_TIMEDOUT;
				if( r->callback_done ) {
					r->callback_done( r, r->status, r->customp );
				}
			}
		}

		// handle pauses for synchronous requests
		if( r->status == WSTATUS_STARTED && !r->callback_read ) {
			if( r->rxreceived >= r->rxreturned + WMAXBUFFERING ) {
				wswcurl_pause( r );
			}
		}

		r = next;
	}

	//CURLDBG(("CURL BEFORE MULTI_PERFORM\n"));
	while ( curl_multi_perform(curlmulti, &ret) == CURLM_CALL_MULTI_PERFORM) {
		CURLDBG(("   CURL MULTI LOOP\n"));
	}
	ret += wswcurl_checkmsg();
	//CURLDBG(("CURL after checkmsg\n"));
	return ret;
}

void wswcurl_perform_single (wswcurl_req *req)
{
#if 0
	wswcurl_req *r;
	// curl_easy_perform (req->curl);

	// remove all other pending transfers
	if( curlmulti )
	{
		r = http_requests;
		while( r )
		{
			if( r != req && r->status )
				curl_multi_remove_handle(curlmulti, r->curl);
			r = r->next;
		}
	}

	wswcurl_perform();

	// put the transfers back in
	if( curlmulti )
	{
		r = http_requests;
		while( r )
		{
			if( r != req && r->status )
				curl_multi_add_handle(curlmulti, r->curl);
			r = r->next;
		}
	}
#else
	wswcurl_perform();
#endif
}

int wswcurl_header( wswcurl_req *req, const char *key, const char *value, ...)
{
	char buf[1024], *ptr;
	va_list arg;
	if (req->status) return -1;
	if (!req->curl) return -2;

	snprintf(buf, sizeof(buf), "%s: ", key);
	ptr = &buf[strlen(buf)];

	va_start(arg, value);
	vsnprintf(ptr, (sizeof(buf) - (ptr - buf)), value, arg);
	va_end(arg);

	req->txhead = curl_slist_append(req->txhead, buf);
	return (req->txhead == NULL);
}

wswcurl_req *wswcurl_create( const char *furl, ... )
{
	wswcurl_req *retreq;
	CURL *curl;
	CURLcode res;
	char url[4 * 1024]; // 4kb url buffer?
	va_list arg;
	const char *proxy = http_proxy->string;
	const char *proxy_userpwd = http_proxyuserpwd->string;

	// Prepare url formatting with variable arguments
	va_start( arg, furl );
	vsnprintf( url, sizeof( url ), furl, arg );
	va_end( arg );

	// Initialize structure
	if( !(curl = curl_easy_init()) ) {
		return NULL;
	}

	// allocate, copy
	retreq = ( wswcurl_req * )WMALLOC( sizeof( wswcurl_req ) );
	memset( retreq, 0, sizeof( *retreq ) );

	retreq->curl = curl;
	retreq->url = ( char* )WMALLOC( strlen( url ) + 1 );
	memcpy( retreq->url, url, strlen( url ) + 1 );

	CURLSETOPT( curl, res, CURLOPT_URL, retreq->url );
	CURLSETOPT( curl, res, CURLOPT_WRITEFUNCTION, wswcurl_write );
	CURLSETOPT( curl, res, CURLOPT_NOPROGRESS, 1 );
	CURLSETOPT( curl, res, CURLOPT_FOLLOWLOCATION, 1 );
	CURLSETOPT( curl, res, CURLOPT_HEADERFUNCTION, wswcurl_readheader );
	CURLSETOPT( curl, res, CURLOPT_CONNECTTIMEOUT, WCONNECTTIMEOUT );
#if defined( APPLICATION ) && defined( APP_VERSION_STR ) && defined( OSNAME ) && defined( CPUSTRING )
	CURLSETOPT( curl, res, CURLOPT_USERAGENT, APPLICATION"/"APP_VERSION_STR" (compatible; N; "OSNAME"; "CPUSTRING")" );
#endif
	CURLSETOPT( curl, res, CURLOPT_WRITEDATA, ( void * )retreq );
	CURLSETOPT( curl, res, CURLOPT_WRITEHEADER, ( void * )retreq );
	CURLSETOPT( curl, res, CURLOPT_PRIVATE, ( void * )retreq );

	// HTTP proxy settings
	if( proxy && *proxy ) {
		CURLSETOPT( curl, res, CURLOPT_PROXYTYPE, CURLPROXY_HTTP );

		CURLSETOPT( curl, res, CURLOPT_PROXY, proxy );
		if( proxy_userpwd && *proxy_userpwd ) {
			CURLSETOPT( curl, res, CURLOPT_PROXYUSERPWD, proxy_userpwd );
		}
	}

	wswcurl_set_timeout( retreq, WTIMEOUT );

	// link
	retreq->prev = NULL;
	retreq->next = http_requests;
	if( retreq->next ) {
		retreq->next->prev = retreq;
	}
	else {
		http_requests_hnode = retreq;
	}
	http_requests = retreq;

	CURLDBG((va("   CURL CREATE %s\n", url)));

	return retreq;
}

void wswcurl_set_timeout(wswcurl_req *req, int timeout)
{
	//CURLcode res;
	//CURLSETOPT( req->curl, res, CURLOPT_TIMEOUT, WTIMEOUT );
	req->timeout = timeout;
}

void wswcurl_set_resume_from( wswcurl_req *req, long resume )
{
	CURLcode code;

	if( !req || !req->curl ) {
		return;
	}

	code = curl_easy_setopt( req->curl, CURLOPT_RESUME_FROM, resume );
	if( code != CURLE_OK ) {
		Com_Printf( "Failed to set file resume from length\n" );
	}
}

void wswcurl_delete(wswcurl_req *req)
{
	if ( (!req) || (!wswcurl_isvalidhandle(req)) )
	{
		return;
	}

	// if (req->callback_done && req->active )
	//	req->callback_done ( req, 0, req->customp );

	if (req->txhead)
	{
		curl_slist_free_all(req->txhead);
		req->txhead = NULL;
	}

	if (req->post)
	{
		curl_formfree(req->post);
		req->post      = NULL;
		req->post_last = NULL;
	}

	if (req->curl)
	{
		if (curlmulti && req->status && req->status != WSTATUS_QUEUED) {
			curl_multi_remove_handle(curlmulti, req->curl);
			curlmulti_num_handles--;
		}
		curl_easy_cleanup(req->curl);
		req->curl = NULL;
	}

	if ( (req->next == NULL) && (req == http_requests) )
	{
		// Last item in list
		curl_multi_cleanup(curlmulti);
		curlmulti = NULL;
	}

	if (req->url)
	{
		WFREE(req->url);
	}

	if( req->bhead )
	{
		chained_buffer_t *cb = req->bhead;
		chained_buffer_t *next = cb->next;

		while( cb ) {
			WFREE( cb );
			cb = next;
			if( cb )
				next = cb->next;
		}
	}

	// remove from list
	if (http_requests_hnode == req) http_requests_hnode = req->prev;
	if (http_requests == req) http_requests = req->next;
	if (req->prev) req->prev->next = req->next;
	if (req->next) req->next->prev = req->prev;
	WFREE(req);
}

int wswcurl_isvalidhandle(wswcurl_req *req)
{
	wswcurl_req *r = http_requests;
	while (r != NULL )
	{
		if (r == req)
		{
			return 1;
		}
		r = r->next;
	}

	return 0;
}

const char *wswcurl_get_content_type( wswcurl_req *req )
{
	char *content_type = NULL;
	curl_easy_getinfo( req->curl, CURLINFO_CONTENT_TYPE, &content_type );
	return content_type;
}

const char *wswcurl_getip(wswcurl_req *req)
{
	char *ipstr = NULL;
	curl_easy_getinfo( req->curl, CURLINFO_PRIMARY_IP, &ipstr );
	return ipstr;
}

const char *wswcurl_errorstr(int status)
{
	return curl_easy_strerror( ( CURLcode )-status );
}

const char *wswcurl_get_url(const wswcurl_req *req)
{
	return req->url;
}

const char *wswcurl_get_effective_url(wswcurl_req *req)
{
	char *last_url = NULL;
	curl_easy_getinfo( req->curl, CURLINFO_EFFECTIVE_URL, &last_url );
	return last_url;
}

///////////////////////
// static functions

// Some versions of CURL don't report the correct exepected size when following redirects
// This manual interpretation of the expected size fixes this.
static size_t wswcurl_readheader(void *ptr, size_t size, size_t nmemb, void *stream)
{
	char buf[1024], *str;
	int slen;
	wswcurl_req *req = (wswcurl_req*)stream;

	memset(buf, 0, sizeof(buf));
	memcpy(buf, ptr, (size * nmemb) > (sizeof(buf) - 1) ? (sizeof(buf) - 1): (size * nmemb));
	str = buf;
	while (*str)
	{
		if ( (*str >= 'a') && (*str <= 'z') ) *str -= 'a' - 'A';
		str++;
	}

	if ( (str = (char*)strstr(buf, "CONTENT-LENGTH:")) )
	{
		int size;

		while (*str && (*str != ':')) { str++; }
		str++;

		while (*str && (*str == ' ')) { str++; }
		slen = strlen(str) - 1;

		while ( (slen > 0) && ( ( str[slen] == '\n') || ( str[slen] == '\r') || ( str[slen] == ' ') ) ) {
			str[slen] = '\0';
			slen = strlen(str) - 1;
		}

		size = atoi(str);
		if( size >= 0 ) {
			req->rx_expsize = size;
		}
	}
	else if ( (str  = (char*)strstr(buf, "TRANSFER-ENCODING:")) )
	{
		req->rx_expsize = 0;
	}

	// call header callback function
	if( req->callback_header ) {
		req->callback_header( req, buf, req->customp );
	}

	req->last_action = wswcurl_now();
	return size * nmemb;
}

static size_t wswcurl_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	float progress;
	long numb;
	wswcurl_req *req = (wswcurl_req*)stream;

	if( !req->headers_done ) {
		req->headers_done = 1;
	}

	numb = size * nmemb;
	req->rxreceived += numb;
	req->last_action = wswcurl_now();

	progress = !req->rx_expsize ? 0.0 : (float)(((double)req->rxreceived / (double)req->rx_expsize) * 100.0);
	clamp( progress, 0, 100 );

	if( req->callback_read )
	{
		return req->callback_read( req, ptr, numb, progress, req->customp );
	}
	else
	{
		chained_buffer_t *cb;

		// Allocate new buffer
		cb = ( chained_buffer_t* )WMALLOC( sizeof(*cb) + numb );
		memset( cb, 0, sizeof(*cb) );

		// Stick the buffer to the end of the chain
		if( req->btail )
			req->btail->next = cb;
		req->btail = cb;
		if( !req->bhead )
			req->bhead = cb;

		memcpy( cb->data, ptr, numb );
		cb->data[numb] = '\0';
		cb->rxsize = numb;
	}

	return numb;
}

static int wswcurl_checkmsg( void )
{
	int cnt = 0;
	CURLMsg *msg;
	wswcurl_req *r;
	int ret = 0;
	char *info;

	do {
		msg = curl_multi_info_read( curlmulti, &cnt );
		if( !msg || !msg->easy_handle ) {
			continue;
		}

		// Treat received message.
		curl_easy_getinfo( msg->easy_handle, CURLINFO_PRIVATE, &info );
		r = ( wswcurl_req * )(( void * )info);

		if( !r ) {
			//Com_Printf("OOPS: Message from unknown source!\n");
			continue; // Not for us - oops :)
		}

		if( msg->msg != CURLMSG_DONE ) {
			//Com_Printf("Other message for %s: %d\n", r->url, msg->msg);
			continue;
		}

		ret++;

		if( msg->data.result == CURLE_OK ) {
			// Done!
			r->status = WSTATUS_FINISHED;
			curl_easy_getinfo( r->curl, CURLINFO_RESPONSE_CODE, &(r->respcode) );

			if( r->callback_done ) {
				r->callback_done( r, r->respcode, r->customp );
			}
		}
		else {
			// failed, store and pass to callback negative status value
			r->status = -abs( msg->data.result ); 
			r->respcode = -1;

			if( r->callback_done ) {
				r->callback_done( r, r->status, r->customp );
			}
		}
	} while( cnt && msg );

	return ret;
}

static void wswcurl_pause( wswcurl_req *req )
{
	if( req->paused ) {
		return;
	}

	curl_easy_pause( req->curl, CURLPAUSE_RECV );
	req->paused = 1;
}

static void wswcurl_unpause( wswcurl_req *req )
{
	if( !req->paused ) {
		return;
	}

	curl_easy_pause( req->curl, CURLPAUSE_CONT );
	req->paused = 0;
}

int wswcurl_tell( wswcurl_req *req )
{
	return req->rxreturned;
}

static int wswcurl_remaining( wswcurl_req *req )
{
	if( req->rx_expsize ) {
		return req->rx_expsize - req->rxreturned;
	}
	return req->rxreceived - req->rxreturned;
}

int wswcurl_eof( wswcurl_req *req )
{
	return (req->status == WSTATUS_FINISHED || req->status < 0) // request completed
		&& !wswcurl_remaining( req );
}

static time_t wswcurl_now( void )
{
	return time( NULL );
}
