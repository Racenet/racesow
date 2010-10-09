/*
* AngelWrap is a C wrapper for AngelScript. (c) German Garcia 2008 
*/

#if defined ( _WIN32 ) || ( _WIN64 )
#include <string.h>
#endif

#if defined ( __APPLE__ )
#include <angelscript/angelscript.h>	
#else
#include <angelscript.h>
#endif

#include "qas_local.h"
#include "qas_scriptarray.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QASINVALIDHANDLE -127

#define asFUNCTION_GENERIC(f) asFUNCTION( (void (qcdecl *)(asIScriptGeneric *))f )

typedef struct enginehandle_s 
{
	int handle;
	char *name;
	qboolean max_portability;
	asIScriptEngine *engine;
	struct enginehandle_s *next;
}enginehandle_t;

static enginehandle_t *engineHandlesHead = NULL;
static int numRegisteredEngines = 0;

typedef struct contexthandle_s 
{
	int handle;
	int owner;
	asIScriptContext *ctx;
	asDWORD timeOut;
	struct contexthandle_s *next;
}contexthandle_t;

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

void qasGenericLineCallback( asIScriptContext *ctx, asDWORD *timeOut )
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

void qasRegisterMathAddon( enginehandle_t *eh );

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

	qasRegisterMathAddon( eh );

	// racesow:
	// built-in array type is deprecated as of AngelScript 2.20.0
	// for wsw 0.5 script compatibility, we still register it here
	// in order to port future wsw scripts, please declare dynamic arrays
	// using this symtax: array<int> arr; see this AS changelog:
	// Remove the syntax for declaring dynamic array types, i.e. int[] arr;. Dynamic arrays 
	// should be declared as the template type, e.g. array<int> arr;. Static arrays (when 
	// implemented) should still be declared with int arr[];
	RegisterScriptArray(eh->engine, true);

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
		error = eh->engine->RegisterObjectMethod( objname, declaration, asFUNCTION_GENERIC(funcPointer), (asDWORD)callConv );
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
		error = eh->engine->RegisterObjectBehaviour( objname, (asEBehaviours)behavior, declaration, asFUNCTION_GENERIC(funcPointer), (asDWORD)callConv );
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
		error = eh->engine->RegisterGlobalFunction( declaration, asFUNCTION_GENERIC(funcPointer), (asDWORD)callConv );
	else
		error = eh->engine->RegisterGlobalFunction( declaration, asFUNCTION(funcPointer), (asDWORD)callConv );

	if( error < 0 )
		QAS_Printf( "WARNING: AScript GlobalFunction '%s' failed to register with error %i\n", declaration, error );

	return error;
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
		return eh->engine->RegisterStringFactory( datatype, asFUNCTION_GENERIC(factoryFunc), (asDWORD)callConv );

	return eh->engine->RegisterStringFactory( datatype, asFUNCTION(factoryFunc), (asDWORD)callConv );
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

int qasSetConfigGroupModuleAccess( int engineHandle, const char *groupName, const char *module, int hasAccess )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	return eh->engine->SetConfigGroupModuleAccess( groupName, module, (hasAccess != 0) );
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

int qasGetFunctionIDByIndex( int engineHandle, const char *module, int index )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	
	return mod->GetFunctionIdByIndex( index );
}

int qasGetFunctionIDByName( int engineHandle, const char *module, const char *name )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetFunctionIdByName( name );
}

int qasGetFunctionIDByDecl( int engineHandle, const char *module, const char *decl )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetFunctionIdByDecl( decl );
}

// THESE TWO ARE NOT PART OF ANGELSCRIPT API ANYMORE
const char *qasGetFunctionDeclaration( int engineHandle, const char *module, int funcId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	asIScriptFunction *desc = mod->GetFunctionDescriptorById( funcId );
	if( !desc )
		return NULL;

	return desc->GetDeclaration();
}

const char *qasGetFunctionSection( int engineHandle, const char *module, int funcId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	asIScriptFunction *desc = mod->GetFunctionDescriptorById( funcId );
	if( !desc )
		return NULL;

	return desc->GetScriptSectionName();
}
// THESE TWO ARE NOT PART OF ANGELSCRIPT API ANYMORE [end]

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

//racesow
#ifdef AS_DEPRECATED

const char *qasGetGlobalVarName( int engineHandle, const char *module, int index )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return NULL;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return NULL;

	return mod->GetGlobalVarName( index );
}

int qasGetGlobalVarTypeId( int engineHandle, const char *module, int index )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return QASINVALIDHANDLE;

	asIScriptModule *mod = eh->engine->GetModule( module, asGM_ONLY_IF_EXISTS );
	if( !mod )
		return QASINVALIDHANDLE;

	return mod->GetGlobalVarTypeId( index );
}
#endif
//!racesow

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

void qasNotifyGarbageCollectorOfNewObject( int engineHandle, void *obj, int typeId )
{
	enginehandle_t *eh;

	eh = qasGetEngineHandle( engineHandle );
	if( !eh )
		return;

	eh->engine->NotifyGarbageCollectorOfNewObject( obj, typeId );
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

int qasPrepare( int contextHandle, int funcId )
{
	int error;
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	error = ch->ctx->Prepare( funcId );
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

void *qasGetArgPointer( int contextHandle, unsigned int arg )
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

int qasGetCurrentLineNumber( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetCurrentLineNumber();
}

int qasGetCurrentFunction( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetCurrentFunction();
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

int qasGetExceptionFunction( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetExceptionFunction();
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

int qasGetCallstackSize( int contextHandle )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetCallstackSize();
}

int qasGetCallstackFunction( int contextHandle, int index )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetCallstackFunction( index );
}

int qasGetCallstackLineNumber( int contextHandle, int index )
{
	contexthandle_t *ch = qasGetContextHandle( contextHandle );
	if( !ch )
		return QASINVALIDHANDLE;

	return ch->ctx->GetCallstackLineNumber( index );
}

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

void *qasIScriptGeneric_GetObject( const void *gen )
{
	return (gen ? ((asIScriptGeneric *)gen)->GetObject() : NULL);
}

qas_argument_t *qasIScriptGeneric_GetArg( const void *gen, unsigned int arg, int type )
{
	static qas_argument_t argument;

	argument.type = -1;

	if( gen )
	{
		switch( type )
		{
		case QAS_ARG_UINT8:
			argument.type = type;
			argument.integer = (unsigned char)((asIScriptGeneric *)gen)->GetArgByte( arg );
			break;

		case QAS_ARG_UINT16:
			argument.type = type;
			argument.integer = (unsigned short)((asIScriptGeneric *)gen)->GetArgWord( arg );
			break;

		case QAS_ARG_UINT:
			argument.type = type;
			argument.integer = (unsigned int)((asIScriptGeneric *)gen)->GetArgDWord( arg );
			break;

		case QAS_ARG_UINT64:
			argument.type = type;
			argument.integer64 = (quint64)((asIScriptGeneric *)gen)->GetArgQWord( arg );
			break;

		case QAS_ARG_FLOAT:
			argument.type = type;
			argument.value = (float)((asIScriptGeneric *)gen)->GetArgFloat( arg );
			break;

		case QAS_ARG_DOUBLE:
			argument.type = type;
			argument.dvalue = (double)((asIScriptGeneric *)gen)->GetArgDouble( arg );
			break;

		case QAS_ARG_OBJECT:
			argument.type = type;
			argument.ptr = ((asIScriptGeneric *)gen)->GetArgObject( arg );
			break;

		case QAS_ARG_ADDRESS:
			argument.type = type;
			argument.ptr = ((asIScriptGeneric *)gen)->GetArgAddress( arg );
			break;

		case QAS_ARG_POINTER:
			argument.type = type;
			argument.ptr = ((asIScriptGeneric *)gen)->GetAddressOfArg( arg );
			break;

		default:
			QAS_Error( "qasIScriptGeneric_GetArg: Invalid argument type\n" );
			break;
		}
	}

	return &argument;
}

int qasIScriptGeneric_SetReturn( const void *gen, qas_argument_t *argument )
{
	int error = -1;

	if( gen && argument )
	{
		switch( argument->type )
		{
		case QAS_ARG_UINT8:
			error = ((asIScriptGeneric *)gen)->SetReturnByte( (asBYTE)argument->integer );
			assert( error >= 0 );
			break;

		case QAS_ARG_UINT16:
			error = ((asIScriptGeneric *)gen)->SetReturnWord( (asWORD)argument->integer );
			assert( error >= 0 );
			break;

		case QAS_ARG_UINT:
			error = ((asIScriptGeneric *)gen)->SetReturnDWord( (asDWORD)argument->integer );
			assert( error >= 0 );
			break;

		case QAS_ARG_UINT64:
			error = ((asIScriptGeneric *)gen)->SetReturnQWord( (asQWORD)argument->integer64 );
			assert( error >= 0 );
			break;

		case QAS_ARG_FLOAT:
			error = ((asIScriptGeneric *)gen)->SetReturnFloat( argument->value );
			assert( error >= 0 );
			break;

		case QAS_ARG_DOUBLE:
			error = ((asIScriptGeneric *)gen)->SetReturnDouble( argument->dvalue );
			assert( error >= 0 );
			break;

		case QAS_ARG_OBJECT:
			error = ((asIScriptGeneric *)gen)->SetReturnObject( argument->ptr );
			assert( error >= 0 );
			break;

		case QAS_ARG_ADDRESS:
			error = ((asIScriptGeneric *)gen)->SetReturnAddress( argument->ptr );
			assert( error >= 0 );
			break;

		case QAS_ARG_POINTER:
			*(void**)((asIScriptGeneric *)gen)->GetAddressOfReturnLocation() = argument->ptr;
			assert( error >= 0 );
			break;

		default:
			error = -1;
			break;
		}
	}

	return error;
}

/*************************************
* MATHS ADDON
**************************************/

static int asFunc_abs( int x )
{
	return abs( x );
}

static void asFunc_asGeneric_abs( asIScriptGeneric *gen )
{
	gen->SetReturnDWord( (unsigned int)asFunc_abs( (int)gen->GetArgDWord( 0 ) ) );
}

static float asFunc_fabs( float x )
{
	return fabs( x );
}

static void asFunc_asGeneric_fabs( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_fabs( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_log( float x )
{
	return (float)log( x );
}

static void asFunc_asGeneric_log( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_log( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_pow( float x, float y )
{
	return (float)pow( x, y );
}

static void asFunc_asGeneric_pow( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_pow( gen->GetArgFloat( 0 ), gen->GetArgFloat( 1 ) ) );
}

static float asFunc_cos( float x )
{
	return (float)cos( x );
}

static void asFunc_asGeneric_cos( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_cos( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_sin( float x )
{
	return (float)sin( x );
}

static void asFunc_asGeneric_sin( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_sin( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_tan( float x )
{
	return (float)tan( x );
}

static void asFunc_asGeneric_tan( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_tan( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_acos( float x )
{
	return (float)acos( x );
}

static void asFunc_asGeneric_acos( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_acos( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_asin( float x )
{
	return (float)asin( x );
}

static void asFunc_asGeneric_asin( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_asin( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_atan( float x )
{
	return (float)atan( x );
}

static void asFunc_asGeneric_atan( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_atan( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_atan2( float x, float y )
{
	return (float)atan2( x, y );
}

static void asFunc_asGeneric_atan2( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_atan2( gen->GetArgFloat( 0 ), gen->GetArgFloat( 1 ) ) );
}

static float asFunc_sqrt( float x )
{
	return (float)sqrt( x );
}

static void asFunc_asGeneric_sqrt( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_sqrt( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_ceil( float x )
{
	return (float)ceil( x );
}

static void asFunc_asGeneric_ceil( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_ceil( gen->GetArgFloat( 0 ) ) );
}

static float asFunc_floor( float x )
{
	return (float)floor( x );
}

static void asFunc_asGeneric_floor( asIScriptGeneric *gen )
{
	gen->SetReturnFloat( asFunc_floor( gen->GetArgFloat( 0 ) ) );
}

void qasRegisterMathAddon( enginehandle_t *eh )
{
	int error;
	int count = 0, failedcount = 0;

	typedef struct
	{
		const char *declaration;
		void *pointer;
		void *pointer_asGeneric;
	} asglobfuncs_t;

	static asglobfuncs_t math_asGlobFuncs[] =
	{
		{ "int abs( int x )", (void *)asFunc_abs, (void *)asFunc_asGeneric_abs },
		{ "float abs( float x )", (void *)asFunc_fabs, (void *)asFunc_asGeneric_fabs },
		{ "float log( float x )", (void *)asFunc_log, (void *)asFunc_asGeneric_log },
		{ "float pow( float x, float y )", (void *)asFunc_pow, (void *)asFunc_asGeneric_pow },
		{ "float cos( float x )", (void *)asFunc_cos, (void *)asFunc_asGeneric_cos },
		{ "float sin( float x )", (void *)asFunc_sin, (void *)asFunc_asGeneric_sin },
		{ "float tan( float x )", (void *)asFunc_tan, (void *)asFunc_asGeneric_tan },
		{ "float acos( float x )", (void *)asFunc_acos, (void *)asFunc_asGeneric_acos },
		{ "float asin( float x )", (void *)asFunc_asin, (void *)asFunc_asGeneric_asin },
		{ "float atan( float x )", (void *)asFunc_atan, (void *)asFunc_asGeneric_atan },
		{ "float atan2( float x, float y )", (void *)asFunc_atan2, (void *)asFunc_asGeneric_atan2 },
		{ "float sqrt( float x )", (void *)asFunc_sqrt, (void *)asFunc_asGeneric_sqrt },
		{ "float ceil( float x )", (void *)asFunc_ceil, (void *)asFunc_asGeneric_ceil },
		{ "float floor( float x )", (void *)asFunc_floor, (void *)asFunc_asGeneric_floor },

		{ NULL, NULL, NULL }
	};

	{
		asglobfuncs_t *func;

		for( func = math_asGlobFuncs; func->declaration; func++ )
		{
			if( eh->max_portability )
				error = qasRegisterGlobalFunction( eh->handle, func->declaration, func->pointer_asGeneric, asCALL_GENERIC );
			else
				error = qasRegisterGlobalFunction( eh->handle, func->declaration, func->pointer, asCALL_CDECL );

			if( error < 0 )
			{
				failedcount++;
				continue;
			}

			count++;
		}
	}
}

#ifdef __cplusplus
};
#endif
