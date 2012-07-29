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

#ifndef __QAS_LOCAL_H__
#define __QAS_LOCAL_H__

#include <new>
#include <string>

#if defined ( _WIN32 ) || ( _WIN64 )
#include <string.h>
#endif

#define AS_USE_STLNAMES 1

#if defined ( __APPLE__ )
#include <angelscript/angelscript.h>
#else
#include <angelscript.h>
#endif

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_angeliface.h"
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

#define QAS_NEW(x)        new(QAS_Malloc(sizeof(x))) (x)
#define QAS_DELETE(ptr,x) {void *tmp = ptr; (ptr)->~x(); QAS_Free(tmp);}

#define QAS_NEWARRAY(x,cnt)  (x*)QAS_Malloc(sizeof(x)*cnt)
#define QAS_DELETEARRAY(ptr) QAS_Free(ptr)

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
void *qasGetGlobalFunctionByDecl( int engineHandle, const char *declaration );

int qasRegisterInterface( int engineHandle, const char *name );
int qasRegisterInterfaceMethod( int engineHandle, const char *intfname, const char *declaration );
int qasRegisterEnum( int engineHandle, const char *type );
int qasRegisterEnumValue( int engineHandle, const char *type, const char *name, int value );
int qasRegisterTypedef( int engineHandle, const char *type, const char *decl );
int qasRegisterStringFactory( int engineHandle, const char *datatype, const void *factoryFunc, int callConv );
int qasRegisterFuncdef( int engineHandle, const char *decl );
int qasBeginConfigGroup( int engineHandle, const char *groupName );
int qasEndConfigGroup( int engineHandle );
int qasRemoveConfigGroup( int engineHandle, const char *groupName );
int qasSetDefaultAccessMask( int engineHandle, int defaultMask );

// Type identification
int qasGetTypeIdByDecl( int engineHandle, const char *decl );
const char *qasGetTypeDeclaration( int engineHandle, int typeId );
int qasGetSizeOfPrimitiveType( int engineHandle, int typeId );

const char *qasGetFunctionSection( void *fptr );
int qasGetFunctionType( void *fptr );
const char *qasGetFunctionName( void *fptr );
const char *qasGetFunctionDeclaration( void *fptr, qboolean includeObjectName );
unsigned int qasGetFunctionParamCount( void *fptr );
int qasGetFunctionReturnTypeId( void *fptr );
void qasAddFunctionReference( void *fptr );
void qasReleaseFunction( void *fptr );

// Script modules
int qasAddScriptSection( int engineHandle, const char *module, const char *name, const char *code, size_t codeLength );
int qasBuildModule( int engineHandle, const char *module );
int qasSetAccessMask( int engineHandle, const char *module, int accessMask );
int qasGetFunctionCount( int engineHandle, const char *module );
void *qasGetFunctionByDecl( int engineHandle, const char *module, const char *decl );
int qasGetGlobalVarCount( int engineHandle, const char *module );
int qasGetGlobalVarIndexByName( int engineHandle, const char *module, const char *name );
int qasGetGlobalVarIndexByDecl( int engineHandle, const char *module, const char *decl );
const char *qasGetGlobalVarDeclaration( int engineHandle, const char *module, int index );
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

// Garbage collection
int qasGarbageCollect( int engineHandle/*, int flags*/ );
void qasGetGCStatistics(  int engineHandle, unsigned int *currentSize, unsigned int *totalDestroyed, unsigned int *totalDetected );
void qasGCEnumCallback( int engineHandle, void *reference );

// Bytecode Saving/Loading
// not added
//virtual int SaveByteCode(const char *module, asIBinaryStream *out) = 0;
//virtual int LoadByteCode(const char *module, asIBinaryStream *in) = 0;

/******* asIScriptContext *******/

int qasReleaseContext( int contextHandle );
int qasGetEngine( int contextHandle );
int qasGetState( int contextHandle );
int qasPrepare( int contextHandle, void *fptr );
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
int qasSetObject( int contextHandle, void *obj );

unsigned char qasGetReturnByte( int contextHandle );
unsigned int qasGetReturnBool( int contextHandle );
unsigned short qasGetReturnWord( int contextHandle );
unsigned int qasGetReturnDWord( int contextHandle );
quint64 qasGetReturnQWord( int contextHandle );
float qasGetReturnFloat( int contextHandle );
double qasGetReturnDouble( int contextHandle );
void *qasGetReturnAddress( int contextHandle );
void *qasGetReturnObject( int contextHandle );
void *qasGetAddressOfArg( int contextHandle, unsigned int arg );
void *qasGetAddressOfReturnValue( int contextHandle );

int qasExecute( int contextHandle );
int qasAbort( int contextHandle );
int qasSuspend( int contextHandle );

// Exception handling
int qasSetException( int contextHandle, const char *string );
int qasGetExceptionLineNumber( int contextHandle );
void *qasGetExceptionFunction( int contextHandle );
const char *qasGetExceptionString( int contextHandle );

int qasGetVarCount( int contextHandle );
const char *qasGetVarName( int contextHandle, int varIndex );
const char *qasGetVarDeclaration( int contextHandle, int varIndex );
int qasGetVarTypeId( int contextHandle, int varIndex );
int qasGetThisTypeId( int contextHandle );
void *qasGetThisPointer( int contextHandle );
void *qasSetUserData( int contextHandle, void *data );
void *qasGetUserData( int contextHandle );

// C++ objects
void *qasGetEngineCpp( int engineHandle );
void *qasGetContextCpp( int contextHandle );

void *qasGetActiveContext( void );

// array tools
void *qasCreateArrayCpp( unsigned int length, void *ot );
void qasReleaseArrayCpp( void *arr );

// string tools
asstring_t *qasStringFactoryBuffer( const char *buffer, unsigned int length );
void qasStringRelease( asstring_t *str );
asstring_t *qasStringAssignString( asstring_t *self, const char *string, unsigned int strlen );

// dictionary tools
void *qasCreateDictionaryCpp( void *engine );
void qasReleaseDictionaryCpp( void *dict );

// any tools
void *qasCreateAnyCpp( void *engine );
void qasReleaseAnyCpp( void *any );

#ifdef __cplusplus
}
#endif

#endif // __QAS_LOCAL_H__
