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

#ifndef GAME_QARCH_H
#define GAME_QARCH_H

#ifdef __cplusplus
extern "C" {
#endif

// global preprocessor defines
#include "config.h"

// q_shared.h -- included first by ALL program modules
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//==============================================

#ifdef _WIN32

// wsw : pb : zlib 1.2.3
//# define ZLIB_WINAPI

// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning( disable : 4244 )       // MIPS
#pragma warning( disable : 4136 )       // X86
#pragma warning( disable : 4051 )       // ALPHA

//# pragma warning(disable : 4018)		// signed/unsigned mismatch
//# pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning( disable : 4514 )       // unreferenced inline function has been removed
#pragma warning( disable : 4152 )       // nonstandard extension, function/data pointer conversion in expression
#pragma warning( disable : 4201 )       // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4054 )       // 'type cast' : from function pointer to data pointer
#pragma warning( disable : 4127 )       // conditional expression is constant
#pragma warning( disable : 4100 )       // unreferenced formal parameter
#pragma warning( disable : 4706 )       // assignment within conditional expression
#pragma warning( disable : 4702 )       // unreachable code
#pragma warning( disable : 4306 )       // conversion from 'int' to 'void *' of greater size
#pragma warning( disable : 4305 )       // truncation from 'void *' to 'int'
#pragma warning( disable : 4055 )		// 'type cast' : from data pointer 'void *' to function pointer

#if defined _M_AMD64
#pragma warning( disable : 4267 )       // conversion from 'size_t' to whatever, possible loss of data
#endif

#if defined(_MSC_VER) && defined(_I64_MAX)
# define HAVE___STRTOI64
#endif

#define HAVE___INLINE

#define HAVE__SNPRINTF

#define HAVE__VSNPRINTF

#define HAVE__STRICMP

#ifdef LCC_WIN32
#ifndef C_ONLY
#define C_ONLY
#endif
#define HAVE_TCHAR
#define HAVE_MMSYSTEM
#define HAVE_DLLMAIN
#else
#define HAVE_WSIPX
#endif

#define LIB_DIRECTORY "libs"
#define LIB_SUFFIX ".dll"

#define VID_INITFIRST

#define GL_DRIVERNAME "opengl32.dll"

#define VORBISFILE_LIBNAME "libvorbisfile.dll"

#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
#else
#define BUILDSTRING "Win32 DEBUG"
#endif

#define OSNAME "Windows"

#ifdef _M_IX86
#if defined __FreeBSD__
#define CPUSTRING "i386"
#define ARCH "freebsd_i386"
#else
#define CPUSTRING "x86"
#define ARCH "x86"
#endif
#elif defined ( __x86_64__ ) || defined( _M_AMD64 )
#if defined __FreeBSD__
#define CPUSTRING "x86_64"
#define ARCH "freebsd_x86_64"
#else
#define CPUSTRING "x64"
#define ARCH "x64"
#endif
#elif defined ( _M_ALPHA )
#define CPUSTRING "axp"
#define ARCH	  "axp"
#endif

// doh, some compilers need a _ prefix for variables so they can be
// used in asm code
#ifdef __GNUC__     // mingw
#define VAR( x )    "_" # x
#else
#define VAR( x )    # x
#endif

#ifdef _MSC_VER
#define HAVE___CDECL
#endif

// wsw : aiwa : 64bit integers and integer-pointer types
#include <basetsd.h>
#include <stdint.h>
typedef int64_t qint64;
typedef uint64_t quint64;
typedef intptr_t qintptr;
typedef uintptr_t quintptr;


typedef int socklen_t;
typedef unsigned long ioctl_param_t;

// The following definition comes from WinSock2.h (typedef UINT_PTR SOCKET)
// But we don't want to include all those Windows definitions there, do we?
typedef UINT_PTR socket_handle_t;

#endif

//==============================================

#if defined ( __linux__ ) || defined ( __FreeBSD__ )

#define HAVE_INLINE

#ifndef HAVE_STRCASECMP // SDL_config.h seems to define this too...
#define HAVE_STRCASECMP
#endif

#define LIB_DIRECTORY "libs"
#define LIB_SUFFIX ".so"

#define GL_DRIVERNAME  "libGL.so.1"

#define VORBISFILE_LIBNAME "libvorbisfile.so"

#ifdef __FreeBSD__
#define BUILDSTRING "FreeBSD"
#define OSNAME "FreeBSD"
#else
#define BUILDSTRING "Linux"
#define OSNAME "Linux"
#endif

#ifdef __i386__
#if defined __FreeBSD__
#define ARCH "freebsd_i386"
#define CPUSTRING "i386"
#else
#define ARCH "i386"
#define CPUSTRING "i386"
#endif
#elif defined ( __x86_64__ )
#if defined __FreeBSD__
#define ARCH "freebsd_x86_64"
#define CPUSTRING "x86_64"
#else
#define ARCH "x86_64"
#define CPUSTRING "x86_64"
#endif
#elif defined ( __powerpc__ )
#define CPUSTRING "ppc"
#define ARCH "ppc"
#elif defined ( __alpha__ )
#define CPUSTRING "axp"
#define ARCH "axp"
#else
#define CPUSTRING "Unknown"
#define ARCH "Unknown"
#endif

#define VAR( x ) # x

// wsw : aiwa : 64bit integers and integer-pointer types
#include <stdint.h>
typedef int64_t qint64;
typedef uint64_t quint64;
typedef intptr_t qintptr;
typedef uintptr_t quintptr;

typedef int ioctl_param_t;

typedef int socket_handle_t;

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)

#endif

//==============================================

#if defined ( __APPLE__ ) && defined ( __MACH__ )

#ifndef __MACOSX__
#define __MACOSX__
#endif

#define HAVE_INLINE

#ifndef HAVE_STRCASECMP // SDL_config.h seems to define this too...
#define HAVE_STRCASECMP
#endif

#define LIB_DIRECTORY "libs"
#define LIB_SUFFIX ".dylib"

#define GL_DRIVERNAME  "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#define VORBISFILE_LIBNAME "libvorbisfile.dylib"

//Mac OSX has universal binaries, no need for cpu dependency
#define BUILDSTRING "MacOSX"
#define OSNAME "MacOSX"
#define CPUSTRING "universal"
#define ARCH "mac"

#define VAR( x ) # x

#include <stdint.h>
typedef int64_t qint64;
typedef uint64_t quint64;
typedef intptr_t qintptr;
typedef uintptr_t quintptr;

typedef int ioctl_param_t;

typedef int socket_handle_t;

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)

#endif

//==============================================

#ifdef HAVE___INLINE
#ifndef inline
#define inline __inline
#endif
#elif !defined ( HAVE_INLINE )
#ifndef inline
#define inline
#endif
#endif

#ifdef HAVE__SNPRINTF
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

#ifdef HAVE__VSNPRINTF
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif

#ifdef HAVE__STRICMP
#ifndef Q_stricmp
#define Q_stricmp( s1, s2 ) _stricmp( ( s1 ), ( s2 ) )
#endif
#ifndef Q_strnicmp
#define Q_strnicmp( s1, s2, n ) _strnicmp( ( s1 ), ( s2 ), ( n ) )
#endif
#endif

#ifdef HAVE_STRCASECMP
#ifndef Q_stricmp
#define Q_stricmp( s1, s2 ) strcasecmp( ( s1 ), ( s2 ) )
#endif
#ifndef Q_strnicmp
#define Q_strnicmp( s1, s2, n ) strncasecmp( ( s1 ), ( s2 ), ( n ) )
#endif
#endif

#if ( defined ( _M_IX86 ) || defined ( __i386__ ) || defined ( __ia64__ ) ) && !defined ( C_ONLY )
#define id386
#else
#ifdef id386
#undef id386
#endif
#endif

#ifndef BUILDSTRING
#define BUILDSTRING "NON-WIN32"
#endif

#ifndef CPUSTRING
#define CPUSTRING  "NON-WIN32"
#endif

#ifdef HAVE_TCHAR
#include <tchar.h>
#endif

#ifdef HAVE___CDECL
#define qcdecl __cdecl
#else
#define qcdecl
#endif

#if defined ( __GNUC__ )
#define ALIGN( x )   __attribute__( ( aligned( x ) ) )
#define NOINLINE     __attribute__((noinline))
#define NAKED
#elif defined ( _MSC_VER )
#define ALIGN( x )   __declspec( align( x ) )
#define NOINLINE
#define NAKED        __declspec( naked )
#else
#define ALIGN( x )
#define NOINLINE
#define NAKED
#endif

#ifdef HAVE___STRTOI64
#define strtoll _strtoi64
#define strtoull _strtoi64
#endif

#ifdef _M_AMD64
#define STR_TO_POINTER(str) (void *)strtoll(str,NULL,0)
#else
#define STR_TO_POINTER(str) (void *)strtol(str,NULL,0)
#endif

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
# define QF_DLL_IMPORT __declspec(dllimport)
# define QF_DLL_EXPORT __declspec(dllexport)
# define QF_DLL_LOCAL
#else
# if __GNUC__ >= 4
#  define QF_DLL_IMPORT __attribute__ ((visibility("default")))
#  define QF_DLL_EXPORT __attribute__ ((visibility("default")))
#  define QF_DLL_LOCAL  __attribute__ ((visibility("hidden")))
# else
#  define QF_DLL_IMPORT
#  define QF_DLL_EXPORT
#  define QF_DLL_LOCAL
# endif
#endif

//==============================================

typedef unsigned char qbyte;
typedef enum { qfalse, qtrue }	  qboolean;
typedef unsigned int qwchar;	// Unicode character

#ifndef NULL
#define NULL ( (void *)0 )
#endif

#ifdef __cplusplus
};
#endif

#endif // GAME_QARCH_H

