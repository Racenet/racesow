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

#include "qas_local.h"
#include "addon/addon_math.h"
#include "addon/addon_scriptarray.h"
#include "addon/addon_string.h"
#include "addon/addon_dictionary.h"
#include "addon/addon_time.h"
#include "addon/addon_any.h"
#include "addon/addon_vec3.h"
#include "addon/addon_cvar.h"
#include "addon/addon_stringutils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QASINVALIDHANDLE -127

typedef struct enginehandle_s 
{
	int handle;
	char *name;
	qboolean max_portability;
	asIScriptEngine *engine;
	struct enginehandle_s *next;
} enginehandle_t;

typedef struct contexthandle_s 
{
	int handle;
	int owner;
	asIScriptContext *ctx;
	asDWORD timeOut;
	struct contexthandle_s *next;
} contexthandle_t;

static enginehandle_t *engineHandlesHead = NULL;
static int numRegisteredEngines = 0;

static contexthandle_t *contextHandlesHead = NULL;
static int numRegisteredContexts = 0;

static void *qasAlloc( size_t size )
{
	return QAS_Malloc( size );
}

static void qasFree( void *mem )
{
	QAS_Free( mem );
}


static inline enginehandle_t *qasGetEngineHandle( int handle )
{
	enginehandle_t *eh;

	for( eh = engineHandlesHead; eh != NULL; eh = eh->next )
	{
		if( eh->handle == handle )
			return eh;
	}

	return NULL;
}

static inline contexthandle_t *qasGetContextHandle( int handle )
{
	contexthandle_t *ch;

	for( ch = contextHandlesHead; ch != NULL; ch = ch->next )
	{
		if( ch->handle == handle )
			return ch;
	}

	return NULL;
}

static void qasGenericLineCallback( asIScriptContext *ctx, asDWORD *timeOut )
{
	// If the time out is reached we abort the script
	asDWORD curTicks =  trap_Milliseconds();

	if( *timeOut && ( *timeOut < curTicks ) )
		ctx->Abort();

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later 
	// time, by simply calling Execute() again.
}

static void qasGenericMessageCallback( const struct asSMessageInfo *msg, void *param )
{
	const char *type = "ERROR";

	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARNING";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	QAS_Printf( "\n%s : %s (line %d, column %d) :\n %s\n", type, msg->section, msg->row, msg->col, msg->message );
}

int qasCreateScriptEngine( qboolean *as_max_portability )
{
	enginehandle_t *eh;
	asIScriptEngine *engine;

	// register the global memory allocation and deallocation functions
	asSetGlobalMemoryFunctions( qasAlloc, qasFree );

	// always new

	// ask for angelscript initialization and script engine creation
	engine = asCreateScriptEngine( ANGELSCRIPT_VERSION );
	if( !engine )
		return -1;

	eh = ( enginehandle_t * )QAS_Malloc( sizeof( enginehandle_t ) );
	eh->handle = numRegisteredEngines++;
	eh->next = engineHandlesHead;
	engineHandlesHead = eh;

	eh->engine = engine;
	eh->max_portability = qfalse;

	if( strstr( asGetLibraryOptions(), "AS_MAX_PORTABILITY" ) )
	{
		QAS_Printf( "* angelscript library with AS_MAX_PORTABILITY detected\n" );
		eh->max_portability = qtrue;
	}

	if( as_max_portability )
		*as_max_portability = eh->max_portability;

	// The script compiler will write any compiler messages to the callback.
	eh->engine->SetMessageCallback( asFUNCTION( qasGenericMessageCallback ), 0, asCALL_CDECL );

	PreRegisterMathAddon( engine );
	PreRegisterScriptArrayAddon( engine, true );
	PreRegisterStringAddon( engine );
	PreRegisterDictionaryAddon( engine );
	PreRegisterTimeAddon( engine );
	PreRegisterScriptAny( engine );
	PreRegisterVec3Addon( engine );
	PreRegisterCvarAddon( engine );
	PreRegisterStringUtilsAddon( engine );

	RegisterMathAddon( engine );
	RegisterScriptArrayAddon( engine, true );
	RegisterStringAddon( engine );
	RegisterDictionaryAddon( engine );
	RegisterTimeAddon( engine );
	RegisterScriptAny( engine );
	RegisterVec3Addon( engine );
	RegisterCvarAddon( engine );
	RegisterStringUtilsAddon( engine );

	return eh->handle;
}

int qasSetEngineProperty( int engineHandle, int property, int value )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->SetEngineProperty( (asEEngineProp)property, (asPWORD)value );
}

size_t qasGetEngineProperty( int engineHandle, int property )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		QAS_Error( "qasGetEngineProperty: invalid engine handle\n" );

	return (size_t)eh->engine->GetEngineProperty( (asEEngineProp)property );
}

int qasCreateContext( int engineHandle )
{
	asIScriptContext *ctx;
	contexthandle_t *ch;
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	// always new

	ctx = eh->engine->CreateContext();
	if( !ctx )
		return -1;

	ch = ( contexthandle_t * )QAS_Malloc( sizeof( contexthandle_t ) );

	// We don't want to allow the script to hang the application, e.g. with an
	// infinite loop, so we'll use the line callback function to set a timeout
	// that will abort the script after a certain time. Before executing the 
	// script the timeOut variable will be set to the time when the script must 
	// stop executing. 

	error = ctx->SetLineCallback( asFUNCTION(qasGenericLineCallback), &ch->timeOut, asCALL_CDECL );
	if( error < 0 )
	{
		QAS_Free( ch );
		return -1;
	}

	ch->ctx = ctx;
	ch->handle = numRegisteredContexts++;
	ch->owner = eh->handle;
	ch->next = contextHandlesHead;
	contextHandlesHead = ch;

	return ch->handle;
}

int qasReleaseContext( int contextHandle )
{
	contexthandle_t *prevhandle;
	contexthandle_t *ch = qasGetContextHandle( contextHandle );

	if( !ch )
		return QASINVALIDHANDLE;

	if( ch == contextHandlesHead )
	{
		contextHandlesHead = ch->next;
	}
	else
	{
		// find the handle that links our handle as next
		for( prevhandle = contextHandlesHead; prevhandle != NULL; prevhandle = prevhandle->next )
		{
			if( prevhandle->next == ch )
			{
				prevhandle->next = ch->next;
				break;
			}
		}	
	}

	ch->ctx->Release();
	QAS_Free( ch );

	return 1;
}

int qasAdquireContext( int engineHandle )
{
	enginehandle_t *eh = qasGetEngineHandle( engineHandle );
	contexthandle_t *ch;

	if( !eh )
		return QASINVALIDHANDLE;

	// try to reuse any context linked to this engine
	for( ch = contextHandlesHead; contextHandlesHead != NULL && ch != NULL; ch = ch->next )
	{
		if( ch->owner == engineHandle && ch->ctx->GetState() == asEXECUTION_FINISHED )
			return ch->handle;
	}

	// if no context was available, create a new one
	return qasCreateContext( engineHandle );
}

int qasReleaseScriptEngine( int engineHandle )
{
	enginehandle_t *prevhandle;
	enginehandle_t *eh = qasGetEngineHandle( engineHandle );
	contexthandle_t *ch, *next_ch, *prev_ch;

	if( !eh )
		return QASINVALIDHANDLE;

	// release all contexts linked to this engine
	for( ch = contextHandlesHead, prev_ch = NULL; ch != NULL; ch = next_ch )
	{
		next_ch = ch->next;
		if( ch->owner == engineHandle )
		{
			if( prev_ch )
				prev_ch->next = next_ch;
			qasReleaseContext( ch->handle );
		}
		else
		{
			prev_ch = ch;
		}
	}

	// release the engine handle
	if( eh == engineHandlesHead )
	{
		engineHandlesHead = eh->next;
	}
	else
	{
		// find the handle that links our handle as next
		for( prevhandle = engineHandlesHead; prevhandle != NULL; prevhandle = prevhandle->next )
		{
			if( prevhandle->next == eh )
			{
				prevhandle->next = eh->next;
				break;
			}
		}
	}

	eh->engine->Release();
	QAS_Free( eh );

	return 1;
}

/*
* -- OBJECTS --
*/

int qasRegisterObjectType( int engineHandle, const char *name, int byteSize, unsigned int flags )
{
	int error;
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	error = eh->engine->RegisterObjectType( name, byteSize, (asDWORD)flags );
	if( error < 0 )
		QAS_Printf( "WARNING: AScript object '%s' failed to register with error %i\n", name, error );

	return error;
}

int qasRegisterObjectProperty( int engineHandle, const char *objname, const char *declaration, int byteOffset )
{
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	error = eh->engine->RegisterObjectProperty( objname, declaration, byteOffset );
	if( error < 0 )
		QAS_Printf( "WARNING: AScript property '%s' in object '%s' failed to register with error %i\n", declaration, objname, error );

	return error;
}

int qasRegisterObjectMethod( int engineHandle, const char *objname, const char *declaration, const void *funcPointer, int callConv )
{
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	if( eh->max_portability )
		return QASINVALIDHANDLE;
	else
		error = eh->engine->RegisterObjectMethod( objname, declaration, asFUNCTION(funcPointer), (asDWORD)callConv );
	
	if( error < 0 )
		QAS_Printf( "WARNING: AScript method '%s' in object '%s' failed to register with error %i\n", declaration, objname, error );

	return error;
}

int qasRegisterObjectBehaviour( int engineHandle, const char *objname, unsigned int behavior, const char *declaration, const void *funcPointer, int callConv )
{
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	if( eh->max_portability )
		return QASINVALIDHANDLE;
	else
		error = eh->engine->RegisterObjectBehaviour( objname, (asEBehaviours)behavior, declaration, asFUNCTION(funcPointer), (asDWORD)callConv );

	if( error < 0 )
		QAS_Printf( "WARNING: AScript Behavior '%s' in object '%s' failed to register with error %i\n", declaration, objname, error );

	return error;
}

// GLOBAL --

int qasRegisterGlobalProperty( int engineHandle, const char *declaration, void *pointer )
{
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	error = eh->engine->RegisterGlobalProperty( declaration, pointer );
	if( error < 0 )
		QAS_Printf( "WARNING: AScript GlobalProperty '%s' failed to register with error %i\n", declaration, error );

	return error;
}

int qasRegisterGlobalFunction( int engineHandle, const char *declaration, const void *funcPointer, int callConv )
{
	enginehandle_t *eh;
	int error;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	if( eh->max_portability )
		return QASINVALIDHANDLE;
	else
		error = eh->engine->RegisterGlobalFunction( declaration, asFUNCTION(funcPointer), (asDWORD)callConv );

	if( error < 0 )
		QAS_Printf( "WARNING: AScript GlobalFunction '%s' failed to register with error %i\n", declaration, error );

	return error;
}

void *qasGetGlobalFunctionByDecl( int engineHandle, const char *declaration )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptFunction *f = eh->engine->GetGlobalFunctionByDecl( declaration );
	return static_cast<void*>( f );
}

int qasRegisterInterface( int engineHandle, const char *name )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterInterface( name );
}

int qasRegisterInterfaceMethod( int engineHandle, const char *intfname, const char *declaration )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterInterfaceMethod( intfname, declaration );
}

int qasRegisterEnum( int engineHandle, const char *type )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterEnum( type );
}

int qasRegisterEnumValue( int engineHandle, const char *type, const char *name, int value )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterEnumValue( type, name, value );
}

int qasRegisterTypedef( int engineHandle, const char *type, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterTypedef( type, decl );
}

int qasRegisterStringFactory( int engineHandle, const char *datatype, const void *factoryFunc, int callConv )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	if( eh->max_portability )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterStringFactory( datatype, asFUNCTION(factoryFunc), (asDWORD)callConv );
}

int qasRegisterFuncdef( int engineHandle, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	if( eh->max_portability )
		return QASINVALIDHANDLE;

	return eh->engine->RegisterFuncdef( decl );
}

int qasBeginConfigGroup( int engineHandle, const char *groupName )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->BeginConfigGroup( groupName );
}

int qasEndConfigGroup( int engineHandle )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->EndConfigGroup();
}

int qasRemoveConfigGroup( int engineHandle, const char *groupName )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->RemoveConfigGroup( groupName );
}

int qasSetDefaultAccessMask( int engineHandle, int defaultMask )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->SetDefaultAccessMask( (asDWORD)defaultMask );
}

// Type identification (engine)

int qasGetObjectTypeCount( int engineHandle )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->GetObjectTypeCount();
}

int qasGetTypeIdByDecl( int engineHandle, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->GetTypeIdByDecl( decl );
}

const char *qasGetTypeDeclaration( int engineHandle, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	return eh->engine->GetTypeDeclaration( typeId );
}

int qasGetSizeOfPrimitiveType( int engineHandle, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->GetSizeOfPrimitiveType( typeId );
}

const char *qasGetFunctionSection( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return NULL;
	}
	f = static_cast<asIScriptFunction *>( fptr );

	return f->GetScriptSectionName();
}

// Not added
//virtual asIObjectType *GetObjectTypeById(int typeId) = 0;
//virtual asIObjectType *GetObjectTypeByIndex(asUINT index) = 0;
//virtual int GetObjectTypeCount() = 0;

// module class
// Add the script sections that will be compiled into executable code.
// If we want to combine more than one file into the same script, then 
// we can call AddScriptSection() several times for the same module and
// the script engine will treat them all as if they were one. The script
// section name, will allow us to localize any errors in the script code.
int qasAddScriptSection( int engineHandle, const char *module, const char *name, const char *code, size_t codeLength )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_CREATE_IF_NOT_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->AddScriptSection( name, code, codeLength );
}

int qasBuildModule( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->Build();
}

int qasSetAccessMask( int engineHandle, const char *module, int accessMask )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->SetAccessMask( (asDWORD)accessMask );
}

// Script functions
int qasGetFunctionCount( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetFunctionCount();
}

void *qasGetFunctionByDecl( int engineHandle, const char *module, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	asIScriptFunction *f = mod->GetFunctionByDecl( decl );
	if( !f )
		return NULL;

	return static_cast<void*>( f );
}


// Script global variables

int qasGetGlobalVarCount( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetGlobalVarCount();
}

int qasGetGlobalVarIndexByName( int engineHandle, const char *module, const char *name )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetGlobalVarIndexByName( name );
}

int qasGetGlobalVarIndexByDecl( int engineHandle, const char *module, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetGlobalVarIndexByDecl( decl );
}

const char *qasGetGlobalVarDeclaration( int engineHandle, const char *module, int index )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	return mod->GetGlobalVarDeclaration( index );
}

void *qasGetAddressOfGlobalVar( int engineHandle, const char *module, int index )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	return mod->GetAddressOfGlobalVar( index );
}

// Dynamic binding between modules

int qasGetImportedFunctionCount( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetImportedFunctionCount();
}

int qasGetImportedFunctionIndexByDecl( int engineHandle, const char *module, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetImportedFunctionIndexByDecl( decl );
}

const char *qasGetImportedFunctionDeclaration( int engineHandle, const char *module, int importIndex )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	return mod->GetImportedFunctionDeclaration( importIndex );
}

const char *qasGetImportedFunctionSourceModule( int engineHandle, const char *module, int importIndex )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	return mod->GetImportedFunctionSourceModule( importIndex );
}

int qasBindImportedFunction( int engineHandle, const char *module, int importIndex, int funcId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->BindImportedFunction( importIndex, funcId );
}

int qasUnbindImportedFunction( int engineHandle, const char *module, int importIndex )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->UnbindImportedFunction( importIndex );
}

int qasBindAllImportedFunctions( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->BindAllImportedFunctions();
}

int qasUnbindAllImportedFunctions( int engineHandle, const char *module )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->UnbindAllImportedFunctions();
}

// Type identification (module: not added)
//	virtual int            GetObjectTypeCount() = 0;
//	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) = 0;
//	virtual int            GetTypeIdByDecl(const char *decl) = 0;

// Script execution

void *qasCreateScriptObject( int engineHandle, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	return eh->engine->CreateScriptObject( typeId );
}

void *qasCreateScriptObjectCopy( int engineHandle, void *obj, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	return eh->engine->CreateScriptObjectCopy( obj, typeId );
}

int qasCopyScriptObject( int engineHandle, void *dstObj, void *srcObj, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	eh->engine->CopyScriptObject( dstObj, srcObj, typeId );
	return 1;
}

int qasReleaseScriptObject( int engineHandle, void *obj, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	eh->engine->ReleaseScriptObject( obj, typeId );
	return 1;
}

int qasAddRefScriptObject( int engineHandle, void *obj, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	eh->engine->AddRefScriptObject( obj, typeId );
	return 1;
}

int qasIsHandleCompatibleWithObject( int engineHandle, void *obj, int objTypeId, int handleTypeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return (int)eh->engine->IsHandleCompatibleWithObject( obj, objTypeId, handleTypeId );
}

// String interpretation
// not added
//virtual asETokenClass ParseToken(const char *string, size_t stringLength = 0, int *tokenLength = 0) = 0;
//virtual int  ExecuteString(const char *module, const char *script, asIScriptContext **ctx = 0, asDWORD flags = 0) = 0;

// Garbage collection

int qasGarbageCollect( int engineHandle/*, int flags*/ )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle);
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->GarbageCollect( (asEGCFlags)(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE) );
}

void qasGetGCStatistics( int engineHandle, unsigned int *currentSize, unsigned int *totalDestroyed, unsigned int *totalDetected )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle);
	if( !eh )
		return;

	eh->engine->GetGCStatistics( (asUINT *)currentSize, (asUINT *)totalDestroyed, (asUINT *)totalDetected );
}

void qasGCEnumCallback( int engineHandle, void *reference )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return;

	eh->engine->GCEnumCallback( reference );
}

// Bytecode Saving/Loading
// not added
//virtual int SaveByteCode(const char *module, asIBinaryStream *out) = 0;
//virtual int LoadByteCode(const char *module, asIBinaryStream *in) = 0;

// User data
/*
virtual void *SetUserData(void *data) = 0;
virtual void *GetUserData() = 0;
*/

// User data
void *qasSetUserData( int engineHandle, void *data )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	return eh->engine->SetUserData( data );
}

void *qasGetUserData( int engineHandle )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	return eh->engine->GetUserData();
}


/******* asIScriptContext *******/

// Script context

int qasGetEngine( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->owner;
}

int qasGetState( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return (int)ch->ctx->GetState();
}

int qasPrepare( int contextHandle, void *fptr )
{
	asIScriptFunction *f;

	int error;
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;
	if( !fptr ) {
		return QASINVALIDHANDLE;
	}

	f = static_cast<asIScriptFunction *>( fptr );
	error = ch->ctx->Prepare( f );
	//if( error < 0 )
	//	QAS_Printf( "WARNING: AScript failed to prepare a context for '%s' with error %i\n", qasGetFunctionDeclaration( ch->owner, funcId ), error );

	return error;
}

int qasUnprepare( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->Unprepare();
}

// return types

int qasSetArgByte( int contextHandle, unsigned int arg, unsigned char value )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgByte( (asUINT)arg, (asBYTE)value );
}

int qasSetArgWord( int contextHandle, unsigned int arg, unsigned short value )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgWord( (asUINT)arg, (asWORD)value );
}

int qasSetArgDWord( int contextHandle, unsigned int arg, unsigned int value )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgDWord( (asUINT)arg, (asDWORD)value );
}

int qasSetArgQWord( int contextHandle, unsigned int arg, asQWORD value ) 
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgQWord( (asUINT)arg, value );
}

int qasSetArgFloat( int contextHandle, unsigned int arg, float value )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgFloat( (asUINT)arg, value );
}

int qasSetArgDouble( int contextHandle, unsigned int arg, double value )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgDouble( (asUINT)arg, value );
}

int qasSetArgAddress( int contextHandle, unsigned int arg, void *addr )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgAddress( (asUINT)arg, addr );
}

int qasSetArgObject( int contextHandle, unsigned int arg, void *obj )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetArgObject( (asUINT)arg, obj );
}

void *qasGetAddressOfArg( int contextHandle, unsigned int arg )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetAddressOfArg( (asUINT)arg );
}

int qasSetObject( int contextHandle, void *obj )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetObject( obj );
}

unsigned char qasGetReturnByte( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		QAS_Error( "qasGetReturnByte: invalid context\n" );

	return (unsigned char)ch->ctx->GetReturnByte();
}

unsigned int qasGetReturnBool( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		QAS_Error( "qasGetReturnBool: invalid context\n" );

	return (unsigned char)ch->ctx->GetReturnByte();
}

unsigned short qasGetReturnWord( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		QAS_Error( "qasGetReturnWord: invalid context\n" );

	return (unsigned short)ch->ctx->GetReturnWord();
}

unsigned int qasGetReturnDWord( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		QAS_Error( "qasGetReturnDWord: invalid context\n" );

	return (unsigned int)ch->ctx->GetReturnDWord();
}

asQWORD qasGetReturnQWord( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		QAS_Error( "qasGetReturnQWord: invalid context\n" );

	return ch->ctx->GetReturnQWord();
}

float qasGetReturnFloat( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetReturnFloat();
}

double qasGetReturnDouble( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetReturnDouble();
}

void *qasGetReturnAddress( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetReturnAddress();
}

void *qasGetReturnObject( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetReturnObject();
}

void *qasGetAddressOfReturnValue( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetAddressOfReturnValue();
}

int qasExecute( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	ch->timeOut = trap_Milliseconds() + 500;
	ch->timeOut = 0;
	return ch->ctx->Execute();
}

int qasAbort( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->Abort();
}

int qasSuspend( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->Suspend();
}

// Exception handling

int qasSetException( int contextHandle, const char *string )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->SetException( string );
}

int qasGetExceptionLineNumber( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetExceptionLineNumber();
}

void *qasGetExceptionFunction( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return static_cast<void*>( ch->ctx->GetExceptionFunction() );
}

const char *qasGetExceptionString( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetExceptionString();
}

// missing
//virtual int  SetExceptionCallback(asSFuncPtr callback, void *obj, int callConv) = 0;
//virtual void ClearExceptionCallback() = 0;

int qasGetVarCount( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetVarCount();
}

const char *qasGetVarName( int contextHandle, int varIndex )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetVarName( varIndex );
}

const char *qasGetVarDeclaration( int contextHandle, int varIndex )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetVarDeclaration( varIndex );
}

int qasGetVarTypeId( int contextHandle, int varIndex )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetVarTypeId( varIndex );
}

void *qasGetAddressOfVar( int contextHandle, int varIndex )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetAddressOfVar( varIndex );
}

int qasGetThisTypeId( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetThisTypeId();
}

void *qasGetThisPointer( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return NULL;

	return ch->ctx->GetThisPointer();
}

/*************************************
* C++ Objects
**************************************/

void *qasGetEngineCpp( int engineHandle )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return 0;

	return static_cast<void*>( eh->engine );
}

void *qasGetContextCpp( int contextHandle )
{
	contexthandle_t *ch;

	ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return 0;

	return static_cast<void*>( ch->ctx );
}

void *qasGetActiveContext( void )
{
	return static_cast<void*>( asGetActiveContext() );
}

/*************************************
* Function pointers
**************************************/

int qasGetFunctionType( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return (int)asFUNC_DUMMY;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	return (int)f->GetFuncType();
}

const char *qasGetFunctionName( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return NULL;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	return f->GetName();
}

const char *qasGetFunctionDeclaration( void *fptr, qboolean includeObjectName )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return NULL;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	return f->GetDeclaration( includeObjectName == qtrue );
}

unsigned int qasGetFunctionParamCount( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return 0;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	return f->GetParamCount();
}

int qasGetFunctionReturnTypeId( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return 0;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	return f->GetReturnTypeId();
}

void qasAddFunctionReference( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	f->AddRef();
}

void qasReleaseFunction( void *fptr )
{
	asIScriptFunction *f;

	if( !fptr ) {
		return;
	}
	f = static_cast<asIScriptFunction *>( fptr );
	f->Release();
}

/*************************************
* Array tools
**************************************/

void *qasCreateArrayCpp( unsigned int length, void *ot )
{
	return static_cast<void*>( QAS_NEW(CScriptArray)( length, static_cast<asIObjectType *>( ot ) ) );
}

void qasReleaseArrayCpp( void *arr )
{
	(static_cast<CScriptArray*>(arr))->Release();
}

/*************************************
* Strings
**************************************/

asstring_t *qasStringFactoryBuffer( const char *buffer, unsigned int length )
{
	return objectString_FactoryBuffer( buffer, length );
}

void qasStringRelease( asstring_t *str )
{
	objectString_Release( str );
}

asstring_t *qasStringAssignString( asstring_t *self, const char *string, unsigned int strlen )
{
	return objectString_AssignString( self, string, strlen );
}

/*************************************
* Dictionary
**************************************/

void *qasCreateDictionaryCpp( void *engine )
{
	return static_cast<void*>( QAS_NEW(CScriptDictionary)( static_cast<asIScriptEngine *>( engine ) ) );
}

void qasReleaseDictionaryCpp( void *dict )
{
	(static_cast<CScriptDictionary*>(dict))->Release();
}

/*************************************
* Any
**************************************/

void *qasCreateAnyCpp( void *engine )
{
	return static_cast<void*>( QAS_NEW(CScriptAny)( static_cast<asIScriptEngine *>( engine ) ) );
}

void qasReleaseAnyCpp( void *any )
{
	(static_cast<CScriptAny*>(any))->Release();
}

#ifdef __cplusplus
};
#endif
