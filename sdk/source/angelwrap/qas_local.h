
#ifndef __QAS_LOCAL_H__
#define __QAS_LOCAL_H__

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_angelwrap.h"
#include "qas_public.h"
#include "qas_syscalls.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct mempool_s *angelwrappool;

#define QAS_MemAlloc( pool, size ) trap_MemAlloc( pool, size, __FILE__, __LINE__ )
#define QAS_MemFree( mem ) trap_MemFree( mem, __FILE__, __LINE__ )
#define QAS_MemAllocPool( name ) trap_MemAllocPool( name, __FILE__, __LINE__ )
#define QAS_MemFreePool( pool ) trap_MemFreePool( pool, __FILE__, __LINE__ )
#define QAS_MemEmptyPool( pool ) trap_MemEmptyPool( pool, __FILE__, __LINE__ )

#define QAS_Malloc( size ) QAS_MemAlloc( angelwrappool, size )
#define QAS_Free( data ) QAS_MemFree( data )

int QAS_API( void );
int QAS_Init( void );
void QAS_ShutDown( void );
struct angelwrap_api_s *QAS_GetAngelExport( void );

void QAS_Printf( const char *format, ... );
void QAS_Error( const char *format, ... );

/******* asIScriptEngine *******/
int qasCreateScriptEngine( qboolean *as_max_portability );
int qasReleaseScriptEngine( int engineHandle );
int qasSetEngineProperty( int engineHandle, int property, int value );
size_t qasGetEngineProperty( int engineHandle, int property );
int qasCreateContext( int engineHandle );
int qasAdquireContext( int engineHandle );

// OBJECTS --
int qasRegisterObjectType( int engineHandle, const char *objname, int byteSize, unsigned int flags );
int qasRegisterObjectProperty( int engineHandle, const char *objname, const char *declaration, int byteOffset );
int qasRegisterObjectMethod( int engineHandle, const char *objname, const char *declaration, const void *funcPointer, int callConv );
int qasRegisterObjectBehaviour( int engineHandle, const char *objname, unsigned int behavior, const char *declaration, const void *funcPointer, int callConv );

// GLOBAL --
int qasRegisterGlobalProperty( int engineHandle, const char *declaration, void *pointer );
int qasRegisterGlobalFunction( int engineHandle, const char *declaration, const void *funcPointer, int callConv );
int qasRegisterGlobalBehaviour( int engineHandle, unsigned int behavior, const char *declaration, const void *funcPointer, int callConv );

int qasRegisterInterface( int engineHandle, const char *name );
int qasRegisterInterfaceMethod( int engineHandle, const char *intfname, const char *declaration );
int qasRegisterEnum( int engineHandle, const char *type );
int qasRegisterEnumValue( int engineHandle, const char *type, const char *name, int value );
int qasRegisterTypedef( int engineHandle, const char *type, const char *decl );
int qasRegisterStringFactory( int engineHandle, const char *datatype, const void *factoryFunc, int callConv );
int qasBeginConfigGroup( int engineHandle, const char *groupName );
int qasEndConfigGroup( int engineHandle );
int qasRemoveConfigGroup( int engineHandle, const char *groupName );
int qasSetConfigGroupModuleAccess( int engineHandle, const char *groupName, const char *module, int hasAccess );

// Type identification
int qasGetTypeIdByDecl( int engineHandle, const char *decl );
const char *qasGetTypeDeclaration( int engineHandle, int typeId );
int qasGetSizeOfPrimitiveType( int engineHandle, int typeId );

// Script modules
int qasAddScriptSection( int engineHandle, const char *module, const char *name, const char *code, size_t codeLength );
int qasBuildModule( int engineHandle, const char *module );
int qasGetFunctionCount( int engineHandle, const char *module );
int qasGetFunctionIDByIndex( int engineHandle, const char *module, int index );
int qasGetFunctionIDByName( int engineHandle, const char *module, const char *name );
int qasGetFunctionIDByDecl( int engineHandle, const char *module, const char *decl );
// these two are not part of angelscript API anymore
const char *qasGetFunctionDeclaration( int engineHandle, const char *module, int funcId );
const char *qasGetFunctionSection( int engineHandle, const char *module, int funcId );
int qasGetGlobalVarCount( int engineHandle, const char *module );
int qasGetGlobalVarIndexByName( int engineHandle, const char *module, const char *name );
int qasGetGlobalVarIndexByDecl( int engineHandle, const char *module, const char *decl );
const char *qasGetGlobalVarDeclaration( int engineHandle, const char *module, int index );
const char *qasGetGlobalVarName( int engineHandle, const char *module, int index );
int qasGetGlobalVarTypeId( int engineHandle, const char *module, int index );
void *qasGetAddressOfGlobalVar( int engineHandle, const char *module, int index );

// Dynamic binding between modules
int qasGetImportedFunctionCount( int engineHandle, const char *module );
int qasGetImportedFunctionIndexByDecl( int engineHandle, const char *module, const char *decl );
const char *qasGetImportedFunctionDeclaration( int engineHandle, const char *module, int importIndex );
const char *qasGetImportedFunctionSourceModule( int engineHandle, const char *module, int importIndex );
int qasBindImportedFunction( int engineHandle, const char *module, int importIndex, int funcId );
int qasUnbindImportedFunction( int engineHandle, const char *module, int importIndex );
int qasBindAllImportedFunctions( int engineHandle, const char *module );
int qasUnbindAllImportedFunctions( int engineHandle, const char *module );

// Script execution
void *qasCreateScriptObject( int engineHandle, int typeId );
void *qasCreateScriptObjectCopy( int engineHandle, void *obj, int typeId );
int qasCopyScriptObject( int engineHandle, void *dstObj, void *srcObj, int typeId );
int qasReleaseScriptObject( int engineHandle, void *obj, int typeId );
int qasAddRefScriptObject( int engineHandle, void *obj, int typeId );
int qasIsHandleCompatibleWithObject( int engineHandle, void *obj, int objTypeId, int handleTypeId );
int qasCompareScriptObjects( int engineHandle, int *result, int behaviour, void *leftObj, void *rightObj, int typeId );

// Garbage collection
int qasGarbageCollect( int engineHandle/*, int flags*/ );
void qasGetGCStatistics(  int engineHandle, unsigned int *currentSize, unsigned int *totalDestroyed, unsigned int *totalDetected );
void qasNotifyGarbageCollectorOfNewObject( int engineHandle, void *obj, int typeId );
void qasGCEnumCallback( int engineHandle, void *reference );

// Bytecode Saving/Loading
// not added
//virtual int SaveByteCode(const char *module, asIBinaryStream *out) = 0;
//virtual int LoadByteCode(const char *module, asIBinaryStream *in) = 0;

/******* asIScriptContext *******/

int qasReleaseContext( int contextHandle );
int qasGetEngine( int contextHandle );
int qasGetState( int contextHandle );
int qasPrepare( int contextHandle, int funcId );
int qasUnprepare( int contextHandle );

// Return types

int qasSetArgByte( int contextHandle, unsigned int arg, unsigned char value );
int qasSetArgWord( int contextHandle, unsigned int arg, unsigned short value );
int qasSetArgDWord( int contextHandle, unsigned int arg, unsigned int value );
int qasSetArgQWord( int contextHandle, unsigned int arg, quint64 value );
int qasSetArgFloat( int contextHandle, unsigned int arg, float value );
int qasSetArgDouble( int contextHandle, unsigned int arg, double value );
int qasSetArgAddress( int contextHandle, unsigned int arg, void *addr );
int qasSetArgObject( int contextHandle, unsigned int arg, void *obj );
void *qasGetArgPointer( int contextHandle, unsigned int arg );

int qasSetObject( int contextHandle, void *obj );

unsigned char qasGetReturnByte( int contextHandle );
unsigned short qasGetReturnWord( int contextHandle );
unsigned int qasGetReturnDWord( int contextHandle );
quint64 qasGetReturnQWord( int contextHandle );
float qasGetReturnFloat( int contextHandle );
double qasGetReturnDouble( int contextHandle );
void *qasGetReturnAddress( int contextHandle );
void *qasGetReturnObject( int contextHandle );
void *qasGetAddressOfReturnValue( int contextHandle );

int qasExecute( int contextHandle );
int qasAbort( int contextHandle );
int qasSuspend( int contextHandle );

int qasGetCurrentLineNumber( int contextHandle );
int qasGetCurrentFunction( int contextHandle );

// Exception handling
int qasSetException( int contextHandle, const char *string );
int qasGetExceptionLineNumber( int contextHandle );
int qasGetExceptionFunction( int contextHandle );
const char *qasGetExceptionString( int contextHandle );
int qasGetCallstackSize( int contextHandle );
int qasGetCallstackFunction( int contextHandle, int index );
int qasGetCallstackLineNumber( int contextHandle, int index );

int qasGetVarCount( int contextHandle );
const char *qasGetVarName( int contextHandle, int varIndex );
const char *qasGetVarDeclaration( int contextHandle, int varIndex );
int qasGetVarTypeId( int contextHandle, int varIndex );
int qasGetThisTypeId( int contextHandle );
void *qasGetThisPointer( int contextHandle );
void *qasSetUserData( int contextHandle, void *data );
void *qasGetUserData( int contextHandle );

void *qasIScriptGeneric_GetObject( const void *gen );
qas_argument_t *qasIScriptGeneric_GetArg( const void *gen, unsigned int arg, int type );
int qasIScriptGeneric_SetReturn( const void *gen, qas_argument_t *argument );


#ifdef __cplusplus
}
#endif

#endif // __QAS_LOCAL_H__
