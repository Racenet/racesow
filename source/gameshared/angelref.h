/*
Copyright (C) 2008 German Garcia

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

#ifndef __ANGELREF_H__
#define __ANGELREF_H__

#include "../gameshared/q_angeliface.h"

/*******************************************
* WARNING: The following enums must match those of angelscript.h
*******************************************/

// Enumerations and constants

// Engine properties
typedef enum
{
	asEP_ALLOW_UNSAFE_REFERENCES       = 1,
	asEP_OPTIMIZE_BYTECODE             = 2,
	asEP_COPY_SCRIPT_SECTIONS          = 3,
	asEP_MAX_STACK_SIZE                = 4,
	asEP_USE_CHARACTER_LITERALS        = 5,
	asEP_ALLOW_MULTILINE_STRINGS       = 6,
	asEP_ALLOW_IMPLICIT_HANDLE_TYPES   = 7,
	asEP_BUILD_WITHOUT_LINE_CUES       = 8,
	asEP_INIT_GLOBAL_VARS_AFTER_BUILD  = 9,
	asEP_REQUIRE_ENUM_SCOPE            = 10,
	asEP_SCRIPT_SCANNER                = 11,
	asEP_INCLUDE_JIT_INSTRUCTIONS      = 12,
	asEP_STRING_ENCODING               = 13,
	asEP_PROPERTY_ACCESSOR_MODE        = 14,
	asEP_EXPAND_DEF_ARRAY_TO_TMPL      = 15,
	asEP_AUTO_GARBAGE_COLLECT          = 16,
	asEP_DISALLOW_GLOBAL_VARS          = 17,
	asEP_ALWAYS_IMPL_DEFAULT_CONSTRUCT = 18
} asEEngineProp;

// Calling conventions
typedef enum 
{
	asCALL_CDECL            = 0,
	asCALL_STDCALL          = 1,
	asCALL_THISCALL         = 2,
	asCALL_CDECL_OBJLAST    = 3,
	asCALL_CDECL_OBJFIRST   = 4,
	asCALL_GENERIC          = 5
} asECallConvTypes;

// Object type flags
typedef enum
{
	asOBJ_REF                        = 0x01,
	asOBJ_VALUE                      = 0x02,
	asOBJ_GC                         = 0x04,
	asOBJ_POD                        = 0x08,
	asOBJ_NOHANDLE                   = 0x10,
	asOBJ_SCOPED                     = 0x20,
	asOBJ_TEMPLATE                   = 0x40,
	asOBJ_ASHANDLE                   = 0x80,
	asOBJ_APP_CLASS                  = 0x100,
	asOBJ_APP_CLASS_CONSTRUCTOR      = 0x200,
	asOBJ_APP_CLASS_DESTRUCTOR       = 0x400,
	asOBJ_APP_CLASS_ASSIGNMENT       = 0x800,
	asOBJ_APP_CLASS_COPY_CONSTRUCTOR = 0x1000,
	asOBJ_APP_CLASS_C                = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR),
	asOBJ_APP_CLASS_CD               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_CA               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_CK               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_CDA              = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_CDK              = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_CAK              = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_CDAK             = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_D                = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_DA               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_DK               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_DAK              = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_A                = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_AK               = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_ASSIGNMENT + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_CLASS_K                = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_COPY_CONSTRUCTOR),
	asOBJ_APP_PRIMITIVE              = 0x2000,
	asOBJ_APP_FLOAT                  = 0x4000,
	asOBJ_APP_CLASS_ALLINTS          = 0x8000,
	asOBJ_APP_CLASS_ALLFLOATS        = 0x10000,
	asOBJ_NOCOUNT                    = 0x20000,
	asOBJ_MASK_VALID_FLAGS           = 0x3FFFF,
	asOBJ_SCRIPT_OBJECT              = 0x80000,
	asOBJ_SHARED                     = 0x100000,
	asOBJ_NOINHERIT                  = 0x200000
} asEObjTypeFlags;

// Behaviours
typedef enum
{
	// Value object memory management
	asBEHAVE_CONSTRUCT,
	asBEHAVE_DESTRUCT,

	// Reference object memory management
	asBEHAVE_FACTORY,
	asBEHAVE_LIST_FACTORY,
	asBEHAVE_ADDREF,
	asBEHAVE_RELEASE,

	// Object operators
	asBEHAVE_VALUE_CAST,
	asBEHAVE_IMPLICIT_VALUE_CAST,
	asBEHAVE_REF_CAST,
	asBEHAVE_IMPLICIT_REF_CAST,
	asBEHAVE_TEMPLATE_CALLBACK,

	// Garbage collection behaviours
	asBEHAVE_FIRST_GC,
	 asBEHAVE_GETREFCOUNT = asBEHAVE_FIRST_GC,
	 asBEHAVE_SETGCFLAG,
	 asBEHAVE_GETGCFLAG,
	 asBEHAVE_ENUMREFS,
	 asBEHAVE_RELEASEREFS,
	asBEHAVE_LAST_GC = asBEHAVE_RELEASEREFS,

	asBEHAVE_MAX
} asEBehaviours;

// Return codes
typedef enum
{
	asSUCCESS                              =  0,
	asERROR                                = -1,
	asCONTEXT_ACTIVE                       = -2,
	asCONTEXT_NOT_FINISHED                 = -3,
	asCONTEXT_NOT_PREPARED                 = -4,
	asINVALID_ARG                          = -5,
	asNO_FUNCTION                          = -6,
	asNOT_SUPPORTED                        = -7,
	asINVALID_NAME                         = -8,
	asNAME_TAKEN                           = -9,
	asINVALID_DECLARATION                  = -10,
	asINVALID_OBJECT                       = -11,
	asINVALID_TYPE                         = -12,
	asALREADY_REGISTERED                   = -13,
	asMULTIPLE_FUNCTIONS                   = -14,
	asNO_MODULE                            = -15,
	asNO_GLOBAL_VAR                        = -16,
	asINVALID_CONFIGURATION                = -17,
	asINVALID_INTERFACE                    = -18,
	asCANT_BIND_ALL_FUNCTIONS              = -19,
	asLOWER_ARRAY_DIMENSION_NOT_REGISTERED = -20,
	asWRONG_CONFIG_GROUP                   = -21,
	asCONFIG_GROUP_IS_IN_USE               = -22,
	asILLEGAL_BEHAVIOUR_FOR_TYPE           = -23,
	asWRONG_CALLING_CONV                   = -24,
	asBUILD_IN_PROGRESS                    = -25,
	asINIT_GLOBAL_VARS_FAILED              = -26
} asERetCodes;

// Context states
typedef enum
{
	asEXECUTION_FINISHED      = 0,
	asEXECUTION_SUSPENDED     = 1,
	asEXECUTION_ABORTED       = 2,
	asEXECUTION_EXCEPTION     = 3,
	asEXECUTION_PREPARED      = 4,
	asEXECUTION_UNINITIALIZED = 5,
	asEXECUTION_ACTIVE        = 6,
	asEXECUTION_ERROR         = 7
} asEContextState;

// Message types
typedef enum
{
	asMSGTYPE_ERROR       = 0,
	asMSGTYPE_WARNING     = 1,
	asMSGTYPE_INFORMATION = 2
} asEMsgType;

// Garbage collector flags
typedef enum
{
	asGC_FULL_CYCLE      = 1,
	asGC_ONE_STEP        = 2,
	asGC_DESTROY_GARBAGE = 4,
	asGC_DETECT_GARBAGE  = 8
} asEGCFlags;

// Token classes
typedef enum
{
	asTC_UNKNOWN    = 0,
	asTC_KEYWORD    = 1,
	asTC_VALUE      = 2,
	asTC_IDENTIFIER = 3,
	asTC_COMMENT    = 4,
	asTC_WHITESPACE = 5
} asETokenClass;

// Prepare flags
#define asPREPARE_PREVIOUS -1

// Type id flags
typedef enum
{
	asTYPEID_VOID           = 0,
	asTYPEID_BOOL           = 1,
	asTYPEID_INT8           = 2,
	asTYPEID_INT16          = 3,
	asTYPEID_INT32          = 4,
	asTYPEID_INT64          = 5,
	asTYPEID_UINT8          = 6,
	asTYPEID_UINT16         = 7,
	asTYPEID_UINT32         = 8,
	asTYPEID_UINT64         = 9,
	asTYPEID_FLOAT          = 10,
	asTYPEID_DOUBLE         = 11,
	asTYPEID_OBJHANDLE      = 0x40000000,
	asTYPEID_HANDLETOCONST  = 0x20000000,
	asTYPEID_MASK_OBJECT    = 0x1C000000,
	asTYPEID_APPOBJECT      = 0x04000000,
	asTYPEID_SCRIPTOBJECT   = 0x08000000,
	asTYPEID_TEMPLATE       = 0x10000000,
	asTYPEID_MASK_SEQNBR    = 0x03FFFFFF
} asETypeIdFlags;

// Type modifiers
typedef enum
{
	asTM_NONE     = 0,
	asTM_INREF    = 1,
	asTM_OUTREF   = 2,
	asTM_INOUTREF = 3
} asETypeModifiers;

// GetModule flags
typedef enum
{
	asGM_ONLY_IF_EXISTS       = 0,
	asGM_CREATE_IF_NOT_EXISTS = 1,
	asGM_ALWAYS_CREATE        = 2
} asEGMFlags;

// Compile flags
typedef enum
{
	asCOMP_ADD_TO_MODULE = 1
} asECompileFlags;

// Function types
typedef enum
{
	asFUNC_DUMMY     =-1,
	asFUNC_SYSTEM    = 0,
	asFUNC_SCRIPT    = 1,
	asFUNC_INTERFACE = 2,
	asFUNC_VIRTUAL   = 3,
	asFUNC_FUNCDEF   = 4,
	asFUNC_IMPORTED  = 5
} asEFuncType;

//
// asBYTE  =  8 bits
// asWORD  = 16 bits
// asDWORD = 32 bits
// asQWORD = 64 bits
// asPWORD = size of pointer
//
typedef unsigned char  asBYTE;
typedef unsigned short asWORD;
typedef unsigned int   asUINT;
#if defined(_MSC_VER) && _MSC_VER <= 1200 // MSVC6 
	// size_t is not really correct, since it only guaranteed to be large enough to hold the segment size.
	// For example, on 16bit systems the size_t may be 16bits only even if pointers are 32bit. But nobody
	// is likely to use MSVC6 to compile for 16bit systems anymore, so this should be ok.
	typedef size_t	       asPWORD;
#else
	typedef uintptr_t      asPWORD;
#endif
#ifdef __LP64__
    typedef unsigned int  asDWORD;
    typedef unsigned long asQWORD;
    typedef long asINT64;
#else
    typedef unsigned long asDWORD;
  #if defined(__GNUC__) || defined(__MWERKS__)
    typedef uint64_t asQWORD;
    typedef int64_t asINT64;
  #else
    typedef unsigned __int64 asQWORD;
    typedef __int64 asINT64;
  #endif
#endif

#endif // __ANGELREF_H__
