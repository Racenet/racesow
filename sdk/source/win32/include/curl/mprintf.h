#ifndef __CURL_MPRINTF_H
#define __CURL_MPRINTF_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2005, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: mprintf.h,v 1.12 2006/01/09 13:17:14 bagder Exp $
 ***************************************************************************/

#include <stdarg.h>
#include <stdio.h> /* needed for FILE */

#include "curl.h"

CURL_EXTERN int curl_mprintf(const char *format, ...);
CURL_EXTERN int curl_mfprintf(FILE *fd, const char *format, ...);
CURL_EXTERN int curl_msprintf(char *buffer, const char *format, ...);
CURL_EXTERN int curl_msnprintf(char *buffer, size_t maxlength, const char *format, ...);
CURL_EXTERN int curl_mvprintf(const char *format, va_list args);
CURL_EXTERN int curl_mvfprintf(FILE *fd, const char *format, va_list args);
CURL_EXTERN int curl_mvsprintf(char *buffer, const char *format, va_list args);
CURL_EXTERN int curl_mvsnprintf(char *buffer, size_t maxlength, const char *format, va_list args);
CURL_EXTERN char *curl_maprintf(const char *format, ...);
CURL_EXTERN char *curl_mvaprintf(const char *format, va_list args);

#ifdef _MPRINTF_REPLACE
# define printf curl_mprintf
# define fprintf curl_mfprintf
# define sprintf curl_msprintf
# define snprintf curl_msnprintf
# define vprintf curl_mvprintf
# define vfprintf curl_mvfprintf
# define vsprintf curl_mvsprintf
# define vsnprintf curl_mvsnprintf
# define aprintf curl_maprintf
# define vaprintf curl_mvaprintf
#endif

#endif /* __CURL_MPRINTF_H */
