
#ifndef __ANGELREF_H__
#define __ANGELREF_H__

#include "../gameshared/q_angelwrap.h"

/*******************************************
* WARNING: The following enums must match those of angelscript.h
*******************************************/

// Engine properties
typedef enum 
{
	asEP_ALLOW_UNSAFE_REFERENCES     = 1,
	asEP_OPTIMIZE_BYTECODE           = 2,
	asEP_COPY_SCRIPT_SECTIONS        = 3,
	asEP_MAX_STACK_SIZE              = 4,
	asEP_USE_CHARACTER_LITERALS      = 5,
	asEP_ALLOW_MULTILINE_STRINGS     = 6,
	asEP_ALLOW_IMPLICIT_HANDLE_TYPES = 7
}asEEngineProp;

// Calling conventions
typedef enum 
{
	asCALL_CDECL            = 0,
	asCALL_STDCALL          = 1,
	asCALL_THISCALL         = 2,
	asCALL_CDECL_OBJLAST    = 3,
	asCALL_CDECL_OBJFIRST   = 4,
	asCALL_GENERIC          = 5
}asECallConvTypes;

// Object type flags
typedef enum 
{
	asOBJ_REF                   = 0x01,
	asOBJ_VALUE                 = 0x02,
	asOBJ_GC                    = 0x04,
	asOBJ_POD                   = 0x08,
	asOBJ_NOHANDLE              = 0x10,
	asOBJ_SCOPED                = 0x20,
	asOBJ_APP_CLASS             = 0x100,
	asOBJ_APP_CLASS_CONSTRUCTOR = 0x200,
	asOBJ_APP_CLASS_DESTRUCTOR  = 0x400,
	asOBJ_APP_CLASS_ASSIGNMENT  = 0x800,
	asOBJ_APP_CLASS_C           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR),
	asOBJ_APP_CLASS_CD          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_CA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_CDA         = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_CONSTRUCTOR + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_D           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR),
	asOBJ_APP_CLASS_A           = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_CLASS_DA          = (asOBJ_APP_CLASS + asOBJ_APP_CLASS_DESTRUCTOR + asOBJ_APP_CLASS_ASSIGNMENT),
	asOBJ_APP_PRIMITIVE         = 0x1000,
	asOBJ_APP_FLOAT             = 0x2000,
	asOBJ_MASK_VALID_FLAGS      = 0x3F3F
}asEObjTypeFlags;

// Behaviours
typedef enum 
{
	// Value object memory management
	asBEHAVE_CONSTRUCT,
	asBEHAVE_DESTRUCT,

	// Reference object memory management
	asBEHAVE_FACTORY,
	asBEHAVE_ADDREF,
	asBEHAVE_RELEASE,

	// Object operators
	asBEHAVE_VALUE_CAST,
	asBEHAVE_IMPLICIT_VALUE_CAST,
	asBEHAVE_INDEX,
	asBEHAVE_NEGATE,

	// Assignment operators
	asBEHAVE_FIRST_ASSIGN,
	asBEHAVE_ASSIGNMENT = asBEHAVE_FIRST_ASSIGN,
	asBEHAVE_ADD_ASSIGN,
	asBEHAVE_SUB_ASSIGN,
	asBEHAVE_MUL_ASSIGN,
	asBEHAVE_DIV_ASSIGN,
	asBEHAVE_MOD_ASSIGN,
	asBEHAVE_OR_ASSIGN,
	asBEHAVE_AND_ASSIGN,
	asBEHAVE_XOR_ASSIGN,
	asBEHAVE_SLL_ASSIGN,
	asBEHAVE_SRL_ASSIGN,
	asBEHAVE_SRA_ASSIGN,
	asBEHAVE_LAST_ASSIGN = asBEHAVE_SRA_ASSIGN,

	// Global operators
	asBEHAVE_FIRST_DUAL,
	asBEHAVE_ADD = asBEHAVE_FIRST_DUAL,
	asBEHAVE_SUBTRACT,
	asBEHAVE_MULTIPLY,
	asBEHAVE_DIVIDE,
	asBEHAVE_MODULO,
	asBEHAVE_EQUAL,
	asBEHAVE_NOTEQUAL,
	asBEHAVE_LESSTHAN,
	asBEHAVE_GREATERTHAN,
	asBEHAVE_LEQUAL,
	asBEHAVE_GEQUAL,
	asBEHAVE_BIT_OR,
	asBEHAVE_BIT_AND,
	asBEHAVE_BIT_XOR,
	asBEHAVE_BIT_SLL,
	asBEHAVE_BIT_SRL,
	asBEHAVE_BIT_SRA,
	asBEHAVE_LAST_DUAL = asBEHAVE_BIT_SRA,
	asBEHAVE_REF_CAST,
	asBEHAVE_IMPLICIT_REF_CAST,

	// Garbage collection behaviours
	asBEHAVE_FIRST_GC,
	asBEHAVE_GETREFCOUNT = asBEHAVE_FIRST_GC,
	asBEHAVE_SETGCFLAG,
	asBEHAVE_GETGCFLAG,
	asBEHAVE_ENUMREFS,
	asBEHAVE_RELEASEREFS,
	asBEHAVE_LAST_GC = asBEHAVE_RELEASEREFS
}asEBehaviours;

// Return codes
enum asERetCodes
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
	asMODULE_IS_IN_USE                     = -25,
	asBUILD_IN_PROGRESS                    = -26
};

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
}asEContextState;

#endif // __ANGELREF_H__
