#ifndef __WSWCURL_H
# define __WSWCURL_H

#include <curl/curl.h>

/**
 * Structure containing all internal data.
 */
typedef struct wswcurl_req_s {
	int status; // 0   : not started
				// 1   : started
				// 2 : finished
				// < 0 : error
	char *url;		// Url of the request
	char *filename; // File to save to. If NULL, allocate memory
	int  filenum;

	// Received data if "filename" is NULL
	char *rxdata;		// Pointer to data buffer
	long rxsize;		// size of data buffer
	long rx_expsize;	// Expected size of the file, -1 if unknown (this is possible!)

	long respcode;	// HTTP response code when request was handled completely.

	// Activity indicator. Set to 0 on each wsw_curlperform and set to 1
	// when there was activity for this request so you can check for this
	int activity;

	// Callback when done
	void (*callback_done)(struct wswcurl_req_s *req, int status);
	void (*callback_progress)(struct wswcurl_req_s *req, double percentage);

	///////////////////////
	// Internal stuff
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
} wswcurl_req;

typedef void (*wswcurl_cb_done)(struct wswcurl_req_s *req, int status);
typedef void (*wswcurl_cb_progress)(struct wswcurl_req_s *req, double percentage);

/**
 * Creates a new curl environment for a specific url.
 * Note that no request is made, this just prepares the request.
 * This request can be modified by various functions available here.
 * The function can dynamicly format the url like printf. Note that the maximum URL size is 4kb
 */
wswcurl_req *wswcurl_create(wswcurl_cb_done callback, char *furl, ...);
/**
 * Starts previously created wswcurl_req request
 */
void wswcurl_start(wswcurl_req *req);
/**
 * Lets curl handle all connection, treats all messages and returns how many connections are still active.
 */
int wswcurl_perform();
/**
 * Cancels and removes a http request
 */
void wswcurl_delete(wswcurl_req *req);
/**
 * Cleans up the warsow curl environment, canceling any existing connections
 */
void wswcurl_cleanup();

/**
 * Assigns a progress callback to a request object
 */
void wswcurl_cbsetprogress(wswcurl_req *req, wswcurl_cb_progress prg);

/**
 * Sets the filename of the file to download to. If this is not used, the file is received to memory.
 */
void wswcurl_setfilename(wswcurl_req *req, char *filename);


/**
 * Add a header field in the format of "key: value" to the header
 * The value field acts like printf
 */
int wswcurl_header( wswcurl_req *req, const char *key, const char *value, ...);
/**
 * Add a form field to the request. Adding this makes the request a POST request.
 * The value field acts like printf.
 */
int wswcurl_formadd(wswcurl_req *req, const char *field, const char *value, ...);

/**
 * Checks if a handle is still known in the wswcurl pool.
 */
int wswcurl_isvalidhandle(wswcurl_req *req);

#endif
