/*
Copyright (C) 2012 Victor Luchtz

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

#ifndef __Q_ANGELIFACE_H__
#define __Q_ANGELIFACE_H__

// public interfaces

typedef struct asstring_s
{
	char *buffer;
	size_t len, size;
	int asRefCount;
} asstring_t;

typedef struct asvec3_s
{
	vec3_t v;
} asvec3_t;

#ifdef __cplusplus

class CScriptArrayInterface
{
protected:
	virtual ~CScriptArrayInterface() {};

public:
	virtual void AddRef() const = 0;
	virtual void Release() const = 0;

	virtual void Resize( unsigned int numElements ) = 0;
	virtual unsigned int GetSize() const = 0;

	// Get a pointer to an element. Returns 0 if out of bounds
	virtual const void *At( unsigned int index ) const = 0;

	virtual void InsertAt( unsigned int index, void *value ) = 0;
	virtual void RemoveAt( unsigned int index ) = 0;
	virtual void Sort( unsigned int index, unsigned int count, bool asc ) = 0;
	virtual void Reverse() = 0;
	virtual int  Find( unsigned int index, void *value ) const = 0;
};

class CScriptDictionaryInterface
{
protected:
	virtual ~CScriptDictionaryInterface() {};

public:
	virtual void AddRef() const = 0;
	virtual void Release() const = 0;

	// Sets/Gets a variable type value for a key
	virtual void Set(const asstring_t *key, void *value, int typeId) = 0;
	virtual bool Get(const asstring_t *key, void *value, int typeId) const = 0;

	// Sets/Gets an integer number value for a key
	virtual void Set(const asstring_t *key, qint64 &value) = 0;
	virtual bool Get(const asstring_t *key, qint64 &value) const = 0;

	// Sets/Gets a real number value for a key
	virtual void Set(const asstring_t *key, double &value) = 0;
	virtual bool Get(const asstring_t *key, double &value) const = 0;

	// Returns true if the key is set
	virtual bool Exists(const asstring_t *key) const = 0;

	// Deletes the key
	virtual void Delete(const asstring_t *key) = 0;

	// Deletes all keys
	virtual void Clear() = 0;
};

class CScriptAnyInterface
{
protected:
	virtual ~CScriptAnyInterface() {};

public:
	// Memory management
	virtual int AddRef() const = 0;
	virtual int Release() const = 0;

	// Store the value, either as variable type, integer number, or real number
	virtual void Store(void *ref, int refTypeId) = 0;
	virtual void Store(asINT64 &value) = 0;
	virtual void Store(double &value) = 0;

	// Retrieve the stored value, either as variable type, integer number, or real number
	virtual bool Retrieve(void *ref, int refTypeId) const = 0;
	virtual bool Retrieve(asINT64 &value) const = 0;
	virtual bool Retrieve(double &value) const = 0;

	// Get the type id of the stored value
	virtual int  GetTypeId() const = 0;
};

#endif

typedef	struct angelwrap_api_s
{
	int angelwrap_api_version;

	// ENGINE--
	int ( *asCreateScriptEngine )( qboolean *as_max_portability );
	int ( *asReleaseScriptEngine )( int engineHandle );
	int ( *asAdquireContext )( int engineHandle );

	// Script modules
	int ( *asAddScriptSection )( int engineHandle, const char *module, const char *name, const char *code, size_t codeLength );
	int ( *asBuildModule )( int engineHandle, const char *module );

	// Garbage collection
	int ( *asGarbageCollect )( int engineHandle );
	void ( *asGetGCStatistics )(  int engineHandle, unsigned int *currentSize, unsigned int *totalDestroyed, unsigned int *totalDetected );

	// CONTEXT--
	int ( *asPrepare )( int contextHandle, void *fptr );
	int ( *asExecute )( int contextHandle );
	int ( *asAbort )( int contextHandle );

	// Return types

	int ( *asSetArgByte )( int contextHandle, unsigned int arg, unsigned char value );
	int ( *asSetArgWord )( int contextHandle, unsigned int arg, unsigned short value );
	int ( *asSetArgDWord )( int contextHandle, unsigned int arg, unsigned int value );
	int ( *asSetArgQWord )( int contextHandle, unsigned int arg, quint64 value );
	int ( *asSetArgFloat )( int contextHandle, unsigned int arg, float value );
	int ( *asSetArgDouble )( int contextHandle, unsigned int arg, double value );
	int ( *asSetArgAddress )( int contextHandle, unsigned int arg, void *addr );
	int ( *asSetArgObject )( int contextHandle, unsigned int arg, void *obj );

	int ( *asSetObject )( int contextHandle, void *obj );

	unsigned char ( *asGetReturnByte )( int contextHandle );
	unsigned int  ( *asGetReturnBool )( int contextHandle );
	unsigned short ( *asGetReturnWord )( int contextHandle );
	unsigned int ( *asGetReturnDWord )( int contextHandle );
	quint64 ( *asGetReturnQWord )( int contextHandle );
	float ( *asGetReturnFloat )( int contextHandle );
	double ( *asGetReturnDouble )( int contextHandle );
	void *( *asGetReturnAddress )( int contextHandle );
	void *( *asGetReturnObject )( int contextHandle );
	void *( *asGetAddressOfReturnValue )( int contextHandle );

	// OBJECTS --
	int ( *asRegisterObjectType )( int engineHandle, const char *name, int byteSize, unsigned int flags );
	int ( *asRegisterObjectProperty )( int engineHandle, const char *objname, const char *declaration, int byteOffset );
	int ( *asRegisterObjectMethod )( int engineHandle, const char *objname, const char *declaration, const void *funcPointer, int callConv );
	int ( *asRegisterObjectBehaviour )( int engineHandle, const char *objname, unsigned int behavior, const char *declaration, const void *funcPointer, int callConv );

	// GLOBAL --
	int ( *asRegisterGlobalProperty )( int engineHandle, const char *declaration, void *pointer );
	int ( *asRegisterGlobalFunction )( int engineHandle, const char *declaration, const void *funcPointer, int callConv );
	void *( *asGetGlobalFunctionByDecl )( int engineHandle, const char *decl );

	int ( *asRegisterEnum )( int engineHandle, const char *type );
	int ( *asRegisterEnumValue )( int engineHandle, const char *type, const char *name, int value );

	int ( *asRegisterStringFactory )( int engineHandle, const char *datatype, const void *factoryFunc, int callConv );

	int ( *asRegisterFuncdef )( int engineHandle, const char *decl );

	// EXCEPTIONS--
	void *( *asGetExceptionFunction )( int contextHandle );
	int ( *asGetExceptionLineNumber )( int contextHandle );
	const char *( *asGetExceptionString )( int contextHandle );

	// functions tools
	const char *( *asGetFunctionSection )( void *fptr );
	void *( *asGetFunctionByDecl )( int engineHandle, const char *module, const char *decl );
	int ( *asGetFunctionType )( void *fptr );
	const char *( *asGetFunctionName )( void *fptr );
	const char *( *asGetFunctionDeclaration )( void *fptr, qboolean includeObjectName );
	unsigned int ( *asGetFunctionParamCount )( void *fptr );
	int ( *asGetFunctionReturnTypeId )( void *fptr );
	void ( *asAddFunctionReference )( void *fptr ); 
	void ( *asReleaseFunction )( void *fptr ); 

	// strings
	asstring_t *( *asStringFactoryBuffer )( const char *buffer, unsigned int length );
	void( *asStringRelease )( asstring_t *str );
	asstring_t *( *asStringAssignString )( asstring_t *self, const char *string, unsigned int strlen );

	// C++ interfaces
	void *( *asGetEngineCpp )( int engineHandle );
	void *( *asGetContextCpp )( int contextHandle );

	void *( *asGetActiveContext )( void );

	void *( *asCreateArrayCpp )( unsigned int length, void *ot );
	void ( *asReleaseArrayCpp )( void *arr );

	void *( *asCreateDictionaryCpp )( void *engine );
	void ( *asReleaseDictionaryCpp )( void *arr );

	void *( *asCreateAnyCpp )( void *engine );
	void ( *asReleaseAnyCpp )( void *any );
} angelwrap_api_t;

#endif
