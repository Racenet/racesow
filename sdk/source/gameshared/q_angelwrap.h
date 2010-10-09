
#ifndef __Q_ANGELWRAP_H__
#define __Q_ANGELWRAP_H__

enum {
	QAS_ARG_UINT8,
	QAS_ARG_UINT16,
	QAS_ARG_UINT,
	QAS_ARG_UINT64,
	QAS_ARG_FLOAT,
	QAS_ARG_DOUBLE,
	QAS_ARG_OBJECT,
	QAS_ARG_ADDRESS,
	QAS_ARG_POINTER
};

typedef	struct  
{
	char type;
	union {
		unsigned int integer;
		float value;
	};

	union {
		quint64 integer64;
		double dvalue;
	};

	void *ptr;
}qas_argument_t;

// temp: moveme
typedef	struct angelwrap_api_s
{
	int angelwrap_api_version;

	// ENGINE--
	int ( *asCreateScriptEngine )( qboolean *as_max_portability );
	int ( *asSetEngineProperty ) ( int engineHandle, int property, int value ); 
	int ( *asReleaseScriptEngine )( int engineHandle );
	int ( *asAdquireContext )( int engineHandle );

	// Script modules
	int ( *asAddScriptSection )( int engineHandle, const char *module, const char *name, const char *code, size_t codeLength );
	int ( *asBuildModule )( int engineHandle, const char *module );

	// Garbage collection
	int ( *asGarbageCollect )( int engineHandle );
	void ( *asGetGCStatistics )(  int engineHandle, unsigned int *currentSize, unsigned int *totalDestroyed, unsigned int *totalDetected );

	// CONTEXT--
	int ( *asPrepare )( int contextHandle, int funcId );
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

	int ( *asRegisterEnum )( int engineHandle, const char *type );
	int ( *asRegisterEnumValue )( int engineHandle, const char *type, const char *name, int value );

	int ( *asRegisterStringFactory )( int engineHandle, const char *datatype, const void *factoryFunc, int callConv );

	// EXCEPTIONS--
	int ( *asGetExceptionFunction )( int contextHandle );
	int ( *asGetExceptionLineNumber )( int contextHandle );
	const char *( *asGetExceptionString )( int contextHandle );

	// functions tools
	const char *( *asGetFunctionDeclaration )( int engineHandle, const char *module, int funcId );
	const char *( *asGetFunctionSection )( int engineHandle, const char *module, int funcId );
	int ( *asGetFunctionIDByDecl )( int engineHandle, const char *module, const char *decl );

	void *( *asIScriptGeneric_GetObject )( const void *gen );
	qas_argument_t *( *asIScriptGeneric_GetArg )( const void *gen, unsigned int arg, int type );
	int ( *asIScriptGeneric_SetReturn )( const void *gen, qas_argument_t *argument );
} angelwrap_api_t;

#endif // __Q_ANGELWRAP_H__
