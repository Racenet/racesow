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
#include "wswcurl.h"
#ifdef WSWCURL_TEST_MAIN
# define Com_Printf	printf
#else
# include "qcommon.h"
#endif

#ifdef WIN32
# define snprintf				_snprintf
# define strdup					_strdup
#endif


///////////////////////
// Misc defines that should probably be changed for using in warsow.
#define WMALLOC(x)		Mem_ZoneMalloc(x)
#define WREALLOC(x, y)	( (x) ? Mem_Realloc((x), (y) ) : WMALLOC(y) )
#define WFREE(x)		Mem_Free(x)
#define WSTRDUP(x)		ZoneCopyString(x)

// Curl setopt wrapper
#define CURLSETOPT(c,r,o,v) { if (c) { r = curl_easy_setopt(c,o,v); if (r) { printf("\nCURL ERROR: %d: %s\n", r, curl_easy_strerror(r)); curl_easy_cleanup(c) ; c = NULL; } } }
#define CURLDBG(x)		Com_DPrintf x

///////////////////////
// Function defines
static int wswcurl_checkmsg();
static size_t wswcurl_write(void *ptr, size_t size, size_t nmemb, void *stream);
static size_t wswcurl_readheader(void *ptr, size_t size, size_t nmemb, void *stream);

///////////////////////
// Local variables
static wswcurl_req *http_requests  = NULL; // Linked list of active requests
static CURLM    *curlmulti = NULL;		// Curl MULTI handle

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

void wswcurl_start(wswcurl_req *req)
{
	CURLcode res;
	if (!req) return;
	if (req->status) return; // was already started
	if (!req) return;
	if (!req->curl) {
		wswcurl_delete(req);
		return;
	}

	if (req->txhead) {
		CURLSETOPT(req->curl, res, CURLOPT_HTTPHEADER, req->txhead);
	}
	if (req->post) {
		CURLSETOPT(req->curl, res, CURLOPT_HTTPPOST, req->post);
		req->post_last = NULL;
	}
	// Initialize multi handle if needed
	if (curlmulti == NULL) {
		curlmulti = curl_multi_init();
		if (curlmulti == NULL) {
			CURLDBG(("OOPS: CURL MULTI NULL!!!"));
		}
	}
	if (curl_multi_add_handle(curlmulti, req->curl)) {
		CURLDBG(("OOPS: CURL MULTI ADD HANDLE FAIL!!!"));
	}
	req->status = 1; // Started
}


void wswcurl_cleanup()
{
	while (http_requests) {
		wswcurl_delete(http_requests);
	}
}

int wswcurl_perform()
{
	int ret = 0;
	wswcurl_req *r;
	if (!curlmulti) return 0;
	r = http_requests;
	while (r) {
		r->activity = 0;
		r = r->next;
	}
	//CURLDBG(("CURL BEFORE MULTI_PERFORM\n"));
	while ( curl_multi_perform(curlmulti, &ret) == CURLM_CALL_MULTI_PERFORM) {
		CURLDBG(("   CURL MULTI LOOP\n"));
	}
	ret += wswcurl_checkmsg();
	//CURLDBG(("CURL after checkmsg\n"));
	return ret;
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

wswcurl_req *wswcurl_create(wswcurl_cb_done callback, char *furl, ...)
{
	wswcurl_req req, *retreq = NULL;
	CURLcode res;
	char url[4 * 1024]; // 4kb url buffer?
	va_list arg;

	// Prepare url formatting with variable arguments
	va_start(arg, furl);
	vsnprintf(url, sizeof(url), furl, arg);
	va_end(arg);

	// Initialize structure
	memset(&req, 0, sizeof(req));

	if (! (req.curl = curl_easy_init())) {
		return NULL;
	}
	CURLSETOPT(req.curl, res, CURLOPT_URL, url);
	CURLSETOPT(req.curl, res, CURLOPT_WRITEFUNCTION, wswcurl_write);
	CURLSETOPT(req.curl, res, CURLOPT_NOPROGRESS, 1);
	CURLSETOPT(req.curl, res, CURLOPT_FOLLOWLOCATION, 1);
	CURLSETOPT(req.curl, res, CURLOPT_HEADERFUNCTION, wswcurl_readheader);

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO: Warsow specific stuff? Modify!!
	//CURLSETOPT(req.curl, res, CURLOPT_USERAGENT, "Warsow 0.5");
	//wswcurl_header(&req, "X-Warsow-Version", "0.5");
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (!req.curl) {
		return NULL;
	}
	req.url = WSTRDUP(url);
	req.callback_done = callback;
	req.filenum = -1;
	// Everything OK, allocate, copy, prepend to list and return
	retreq = WMALLOC(sizeof(req));
	memcpy(retreq, &req, sizeof(req));
	retreq->prev = NULL;
	retreq->next = http_requests;
	if (retreq->next) retreq->next->prev = retreq;
	http_requests = retreq;
	// Set the wswcurl_req as data
	CURLSETOPT(retreq->curl, res, CURLOPT_WRITEDATA, (void*)retreq);
	CURLSETOPT(retreq->curl, res, CURLOPT_WRITEHEADER, (void*)retreq);

	return retreq;
}

void wswcurl_cbsetprogress(wswcurl_req *req, wswcurl_cb_progress prg)
{
	req->callback_progress = prg;
}

void wswcurl_setfilename(wswcurl_req *req, char *filename)
{
	if (req->filename) WFREE(req->filename);
	if (filename) req->filename = WSTRDUP(filename);
}

void wswcurl_delete(wswcurl_req *req)
{
	if ( (!req) || (!wswcurl_isvalidhandle(req)) ) {
		return;
	}

	if (req->txhead) {
		curl_slist_free_all(req->txhead);
		req->txhead = NULL;
	}

	if (req->post) {
		curl_formfree(req->post);
		req->post      = NULL;
		req->post_last = NULL;
	}

	if (req->rxdata) {
		WFREE(req->rxdata);
		req->rxdata = NULL;
	}
	if (req->filename) {
		WFREE(req->filename);
		req->filename = NULL;
	}
#ifndef WSWCURL_TEST_MAIN
	if (req->filenum >= 0) {
		FS_FCloseFile(req->filenum);
		req->filenum = -1;
	}
#else
	// TODO: IMPLEMENT TEST
#endif

	if (req->curl) {
		if ( (curlmulti) && (req->status)) curl_multi_remove_handle(curlmulti, req->curl);
		curl_easy_cleanup(req->curl);
		req->curl = NULL;
	}

	if ( (req->next == NULL) && (req == http_requests) ) {
		// Last item in list
		curl_multi_cleanup(curlmulti);
		curlmulti = NULL;
	}

	if (req->url) {
		WFREE(req->url);
	}
	// remove from list
	if (http_requests == req) http_requests = req->next;
	if (req->prev) req->prev->next = req->next;
	if (req->next) req->next->prev = req->prev;
	WFREE(req);
}

int wswcurl_isvalidhandle(wswcurl_req *req)
{
	wswcurl_req *r = http_requests;
	while (r != NULL ) {
		if (r == req) {
			return 1;
		}
		r = r->next;
	}
	return 0;
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
	while (*str) {
		if ( (*str >= 'a') && (*str <= 'z') ) *str -= 'a' - 'A';
		str++;
	}
	if ( (str = (char*)strstr(buf, "CONTENT-LENGTH:")) ) {
		while (*str && (*str != ':')) { str++; }
		str++;
		while (*str && (*str == ' ')) { str++; }
		slen = strlen(str) - 1;
		while ( (slen > 0) && ( ( str[slen] == '\n') || ( str[slen] == '\r') || ( str[slen] == ' ') ) ) {
			str[slen] = '\0';
			slen = strlen(str) - 1;
		}
		req->rx_expsize = atoi(str);
	} else if ( (str  = (char*)strstr(buf, "TRANSFER-ENCODING:")) ) {
		req->rx_expsize = -1;
	}
	req->activity = 1;
	return size * nmemb;
}

static size_t wswcurl_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	wswcurl_req *req = (wswcurl_req*)stream;
	if (!req->filename) {
		// Reallocate buffer
		req->rxdata = WREALLOC(req->rxdata, req->rxsize + (size * nmemb) + 1);
		// Set remaining data to 0
		memset(&(req->rxdata[req->rxsize]), 0, (size * nmemb));
		// Copy received data
		memcpy(&(req->rxdata[req->rxsize]), ptr, size*nmemb);
		// Recalculate size
		req->rxsize += size*nmemb;
		// Add trailing NULL byte at the end to prevent string overflow
		req->rxdata[req->rxsize] = '\0';
	} else {
		// Receive to the filename.
		// TODO: FILE RESUMING??
#ifndef WSWCURL_TEST_MAIN
		if ( (req->filenum < 0) && ( FS_FOpenFile( req->filename, &(req->filenum), FS_WRITE ) < 0 ) ) {
			return 0; // RETURN ERROR
		}
		FS_Write(ptr, size * nmemb, req->filenum);
#else
		// TODO: IMPLEMENT TEST
#endif
	}
	// Mark connection as having had activity.
	req->activity = 1;
	if (req->callback_progress) {
		// Execute progress callback
		// ??? What to return when expected size is unknown? - currently, return 0.0
		req->callback_progress(req, ( req->rx_expsize < 0 ) ? 0.0 : (req->rxsize / req->rx_expsize) * 100.0);
	}
	return size*nmemb;
}

static int wswcurl_checkmsg()
{
	int cnt = 0;
	CURLMsg *msg;
	wswcurl_req *r;
	int ret = 0;
	do {
		msg = curl_multi_info_read(curlmulti, &cnt);
		if (!msg) continue;
		if (!msg->easy_handle) continue;
		// Treat received message.
		r = http_requests;
		while (r != NULL ) {
			if (r->curl == msg->easy_handle) {
				break;
			}
			r = r->next;
		}
		if (!r) {
			//Com_Printf("OOPS: Message from unknown source!\n");
			continue; // Not for us - oops :)
		}
		if (msg->msg != CURLMSG_DONE) {
			//Com_Printf("Other message for %s: %d\n", r->url, msg->msg);
			continue;
		}
#ifndef WSWCURL_TEST_MAIN
		if (r->filenum >= 0) {
			FS_FCloseFile(r->filenum);
			r->filenum = -1;
		}
#else
#endif
		ret++;
		if (msg->data.result == CURLE_OK) {
			//Com_Printf("MSG STATUS DONE OK: %s\n", r->url);
			r->status = 2; // Done!
			curl_easy_getinfo(r->curl, CURLINFO_RESPONSE_CODE, &(r->respcode));
			if (r->callback_done) {
				r->callback_done(r, r->respcode);
			}
		} else {
			//Com_Printf("MSG STATUS DONE FAIL: %d, %s\n", msg->data.result, r->url);
			r->respcode = -1;
			r->status = ( msg->data.result >= 0) ? msg->data.result * -1 : msg->data.result;
			if (r->callback_done) {
				r->callback_done(r, r->status);
			}
		}
	} while (cnt && msg);
	return ret;
}


