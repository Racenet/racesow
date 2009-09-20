/*****************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: lib544.c,v 1.2 2008-05-22 21:49:53 danf Exp $
 */

#include "test.h"


static char teststring[] =
  "This\0 is test binary data with an embedded NUL byte\n";


int test(char *URL)
{
  CURL *curl;
  CURLcode res=CURLE_OK;

  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  if ((curl = curl_easy_init()) == NULL) {
    fprintf(stderr, "curl_easy_init() failed\n");
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* First set the URL that is about to receive our POST. */
  curl_easy_setopt(curl, CURLOPT_URL, URL);

#ifdef LIB545
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) sizeof teststring - 1);
#endif

  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, teststring);

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); /* show verbose for debug */
  curl_easy_setopt(curl, CURLOPT_HEADER, 1L); /* include header */

  /* Update the original data to detect non-copy. */
  strcpy(teststring, "FAIL");

  /* Now, this is a POST request with binary 0 embedded in POST data. */
  res = curl_easy_perform(curl);

  /* always cleanup */
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return (int)res;
}
