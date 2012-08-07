#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"

#include "as/asui.h"
#include "as/asui_local.h"

#include <list>

#define UI_AS_MODULE "UI_AS_MODULE"

namespace ASUI {

typedef WSWUI::UI_Main UI_Main;
typedef std::list<asIScriptContext *> ContextList;

//=======================================

class BinaryBufferStream : public asIBinaryStream
{
	// TODO: use vector!
	unsigned char *data;
	size_t size;		// size of data stored
	size_t allocated;	// actual allocated size
	size_t offset;		// read-head

public:
	BinaryBufferStream() : data(0), size(0), allocated(0), offset(0)
	{
	}

	BinaryBufferStream(void *_data, size_t _size)
		: data(0), size(_size), allocated(_size), offset(0)
	{
		data = __newa__(unsigned char, _size);
		memcpy(data, _data, _size);
	}

	~BinaryBufferStream()
	{
		if(data)
			__delete__(data);
	}

	size_t getDataSize() { return size; }
	void *getData() { return data; }

	// asIBinaryStream implementation
	void Read(void *ptr, asUINT _size)
	{
		if(!data || !ptr)
			trap::Error("BinaryBuffer::Read null pointer");
		if((offset+_size)>allocated)
			trap::Error("BinaryBuffer::Read tried to read more bytes than available");

		memcpy(ptr, data+offset, _size);
		offset+= _size;
	}

	void Write(const void *ptr, asUINT _size)
	{
		if(!data || !ptr)
			trap::Error("BinaryBuffer::Write null pointer");
		if((size+_size)>allocated) {
			// reallocate
			allocated = size+_size;
			unsigned char *tmp = __newa__(unsigned char, allocated);	// allocate little more?
			memcpy(tmp, data, size);
			__delete__(data);	// note! regular delete works for unsigned char*
			data = tmp;
		}
		memcpy(data+size, ptr, _size);
		size += _size;
	}
};

//=======================================

class BinaryFileStream : public asIBinaryStream
{
	int fh;

public:
	BinaryFileStream() : fh(0)
	{
	}

	BinaryFileStream(const char *filename, int mode)
	{
		if(trap::FS_FOpenFile(filename, &fh, mode) == -1)
			trap::Error("BinaryFileStream: failed to open file");
	}

	~BinaryFileStream()
	{
		Close();
	}

	bool Open(const char *filename, int mode)
	{
		Close();
		return trap::FS_FOpenFile(filename, &fh, mode) == -1;
	}

	void Close()
	{
		if( fh )
			trap::FS_FCloseFile( fh );
	}

	// asIBinaryStream implementation
	void Read(void *ptr, asUINT size)
	{
		if(!fh)
			trap::Error("BinaryFileStream::Read tried to read from closed file");

		trap::FS_Read(ptr, size, fh);
	}

	void Write(const void *ptr, asUINT size)
	{
		if(!fh)
			trap::Error("BinaryFileStream::Write tried to write to closed file");

		trap::FS_Write(ptr, size, fh);
	}
};

//=======================================

class ASModule : public ASInterface
{
	UI_Main *ui_main;

	int engineHandle;
	asIScriptEngine *engine;
	ContextList contexts;
	asIScriptModule *module;
	struct angelwrap_api_s *as_api;
	asIObjectType *stringObjectType;
	int scriptCount;
	bool _isBuilding;

// private class, its ok to have everything as public :)
public:

	ASModule()
		: ui_main(0), engineHandle(0), engine(0),
		module(0), as_api(0),
		stringObjectType(0),
		scriptCount(0), _isBuilding( false )
	{
	}

	virtual ~ASModule( void )
	{
	}

	void MessageCallback(const asSMessageInfo *msg)
	{
		const char *msg_type;
		switch( msg->type )
		{
			case asMSGTYPE_ERROR:
				msg_type = S_COLOR_RED "ERROR: ";
				break;
			case asMSGTYPE_WARNING:
				msg_type = S_COLOR_YELLOW "WARNING: ";
			case asMSGTYPE_INFORMATION:
			default:
				msg_type = S_COLOR_CYAN "ANGELSCRIPT: ";
				break;
		}

		Com_Printf( "%s%s %d:%d: %s\n", msg_type, msg->section, msg->row, msg->col, msg->message );
	}

	void ExceptionCallback( asIScriptContext *ctx )
	{
		int line, col;
		asIScriptFunction *func;
		const char *sectionName, *exceptionString, *funcDecl;

		line = ctx->GetExceptionLineNumber( &col, &sectionName );
		func = ctx->GetExceptionFunction();
		exceptionString = ctx->GetExceptionString();
		funcDecl = ( func ? func->GetDeclaration( true ) : "" );

		Com_Printf( S_COLOR_RED "ASModule::ExceptionCallback:\n%s %d:%d %s: %s\n", sectionName, line, col, funcDecl, exceptionString );
	}

	virtual bool Init( void )
	{
		qboolean as_max_portability = qfalse;

		as_api = trap::asGetAngelExport();
		if( !as_api )
			return false;

		engineHandle = as_api->asCreateScriptEngine( &as_max_portability );
		if( engineHandle == -1 ){
			return false;
		}

		if( as_max_portability != qfalse ){
			return false;
		}

		engine = static_cast<asIScriptEngine*>( as_api->asGetEngineCpp( engineHandle ) );
		if( !engine ){
			return false;
		}

		engine->SetMessageCallback( asMETHOD(ASModule, MessageCallback), (void*)this, asCALL_THISCALL );

		// koochi: always enable default constructor
		engine->SetEngineProperty( asEP_ALWAYS_IMPL_DEFAULT_CONSTRUCT, 1 );

		stringObjectType = engine->GetObjectTypeById(engine->GetTypeIdByDecl("String"));

		/*
		module = engine->GetModule( UI_AS_MODULE, asGM_ALWAYS_CREATE );
		if( !module )
			return false;
		*/

		return true;
	}

	virtual void Shutdown( void )
	{
		struct angelwrap_api_s *as_api;

		as_api = trap::asGetAngelExport();

		module = 0;

		for( ContextList::iterator it = contexts.begin(); it != contexts.end(); it++ )
		{
			(*it)->Release();
		}
		contexts.clear();

		if( as_api && engineHandle != -1 )
			as_api->asReleaseScriptEngine( engineHandle );

		engine = 0;
		engineHandle = -1;

		as_api = NULL;

		stringObjectType = 0;
	}

	virtual void setUI( UI_Main *ui ) { ui_main = ui; }

	virtual asIScriptEngine *getEngine( void ) { return engine; }

	virtual asIScriptModule *getModule( const char *name )
	{
		if( !name ) {
			// return current module
			return module;
		}
		return engine->GetModule( name, asGM_ONLY_IF_EXISTS );
	}

	virtual asIScriptContext *getContext( void )
	{ 
		// try to reuse a free context
		for( ContextList::iterator it = contexts.begin(); it != contexts.end(); it++ )
		{
			if( (*it)->GetState() == asEXECUTION_FINISHED ) {
				return *it;
			}
		}

		if( !engine ) {
			return NULL;
		}

		asIScriptContext *context = engine->CreateContext();
		if( !context ) {
			return NULL;
		}

		context->SetExceptionCallback( asMETHOD(ASModule, ExceptionCallback), (void*)this, asCALL_THISCALL );
		contexts.push_back( context );
		return context;
	}

	virtual asIScriptContext *getActiveContext( void )
	{ 
		return as_api ? static_cast<asIScriptContext*>( as_api->asGetActiveContext() ) : NULL;
	}

	virtual asIScriptModule *getActiveModule( void )
	{
		if( !engine ) {
			return NULL;
		}

		asIScriptContext *activeContext = UI_Main::Get()->getAS()->getActiveContext();
		asIScriptFunction *currentFunction = activeContext ? activeContext->GetFunction( 0 ) : NULL;
		if( currentFunction ) {
			return engine->GetModule( currentFunction->GetModuleName(), asGM_CREATE_IF_NOT_EXISTS );
		}
		return NULL;
	}

	virtual asIObjectType *getStringObjectType( void ) 
	{
		return stringObjectType;
	}

	virtual void startBuilding( const char *tempModuleName )
	{
		_isBuilding = true;
		scriptCount = 0;
		module = engine->GetModule( tempModuleName, asGM_CREATE_IF_NOT_EXISTS );
	}

	virtual bool finishBuilding( const char *finalModuleName )
	{
		_isBuilding = false;
		scriptCount = 0;
		if( !module ) {
			return false;
		}
		module->SetName( finalModuleName );
		return module->Build() >= 0;
	}

	virtual bool isBuilding( void )
	{
		return _isBuilding;
	}

	virtual bool addScript( asIScriptModule *module, const char *name, const char *code )
	{
		// TODO: precache SaveByteCode/LoadByteCode
		// (this can be done on a global .rml level too)
		// TODO: figure out if name can be NULL, or otherwise create
		// temp name from NULL argument to differentiate <script> tags
		// without source
		if( !module )
			return false;

		std::string actualName;

		// if theres no name given for this script-section,
		// use MODULENAME_script_COUNT
		if( !name || name[0] == '\0' )
		{
			std::stringstream ss( module->GetName() );
			ss << "_script_" << scriptCount++;
			actualName = ss.str();
		}
		else
			actualName = name;

		return module->AddScriptSection( actualName.c_str(), code ) >= 0;
	}

	virtual bool addFunction( asIScriptModule *module, const char *name, const char *code, asIScriptFunction **outFunction )
	{
		if( _isBuilding )
		{
			// just add it as a script section
			return addScript( module, name, code );
		}

		// add to currently active module. note that this->module may actually point
		// to other module which was compiled last

		// note that reference count for outFunction is increased here!
		return module ? (module->CompileFunction( name, code, 0, asCOMP_ADD_TO_MODULE, outFunction ) >= 0) : false;
	}

	// TODO: disk/mem-cache fully compiled set (use Binary*Stream)

	// testing, dumpapi, note that path has to end with '/'
	virtual void dumpAPI( const char *path )
	{
		int i, j, filenum;
		const char *str = 0;	// for temporary strings
		std::string spath( path );

		if( spath[spath.size()-1] != '/' )
			spath += '/';

		// global file
		std::string global_file( spath + "globals.h" );
		if( trap::FS_FOpenFile( global_file.c_str(), &filenum, FS_WRITE ) == -1 )
		{
			Com_Printf( "ASModule::dumpAPI: Couldn't write %s.\n", global_file.c_str() );
			return;
		}

		// global enums
		str = "/**\r\n * Enums\r\n */\r\n";
		trap::FS_Write( str, strlen( str ), filenum );

		int enumCount = engine->GetEnumCount();
		for( i = 0; i < enumCount; i++ )
		{
			str = "typedef enum\r\n{\r\n";
			trap::FS_Write( str, strlen( str ), filenum );

			int enumTypeId;
			const char *enumName = engine->GetEnumByIndex( i, &enumTypeId );

			int enumValueCount = engine->GetEnumValueCount( enumTypeId );
			for( j = 0; j < enumValueCount; j++ )
			{
				int outValue;
				const char *valueName = engine->GetEnumValueByIndex( enumTypeId, j, &outValue );
				str = va( "\t%s = 0x%x,\r\n", valueName, outValue );
				trap::FS_Write( str, strlen( str ), filenum );
			}

			str = va( "} %s;\r\n\r\n", enumName );
			trap::FS_Write( str, strlen( str ), filenum );
		}

		// global properties
		str = "/**\r\n * Global properties\r\n */\r\n";
		trap::FS_Write( str, strlen( str ), filenum );

		int propertyCount = engine->GetGlobalPropertyCount();
		for( i = 0; i < propertyCount; i++ )
		{
			const char *propertyName;
			const char *propertyNamespace;
			int propertyTypeId;
			bool propertyIsConst;

			if( engine->GetGlobalPropertyByIndex( i, &propertyName, &propertyNamespace, &propertyTypeId, &propertyIsConst ) > 0 )
			{
				const char *decl = va( "%s%s %s::%s;\r\n", propertyIsConst ? "const " : "",
							engine->GetTypeDeclaration( propertyTypeId ), propertyNamespace, propertyName );
				trap::FS_Write( decl, strlen( decl ), filenum );
			}
		}

		// global functions
		str = "/**\r\n * Global functions\r\n */\r\n";
		trap::FS_Write( str, strlen( str ), filenum );

		int functionCount = engine->GetGlobalFunctionCount();
		for( i = 0; i < functionCount; i++ )
		{
			int funcId = engine->GetGlobalFunctionIdByIndex( i );
			asIScriptFunction *func = engine->GetFunctionById( funcId );
			if( func )
			{
				const char *decl = va( "%s;\r\n", func->GetDeclaration( false ) );
				trap::FS_Write( decl, strlen( decl ), filenum );
			}
		}

		trap::FS_FCloseFile( filenum );
		Com_Printf( "Wrote %s\n", global_file.c_str() );

		// classes
		int objectCount = engine->GetObjectTypeCount();
		for( i = 0; i < objectCount; i++ )
		{
			asIObjectType *objectType = engine->GetObjectTypeByIndex( i );
			if( objectType )
			{
				// class file
				std::string class_file( spath + objectType->GetName() + ".h" );
				if( trap::FS_FOpenFile( class_file.c_str(), &filenum, FS_WRITE ) == -1 )
				{
					Com_Printf( "ASModule::dumpAPI: Couldn't write %s.\n", class_file.c_str() );
					continue;
				}

				str = va( "/**\r\n * %s\r\n */\r\n", objectType->GetName() );
				trap::FS_Write( str, strlen( str ), filenum );
				str = va( "class %s\r\n{\r\npublic:", objectType->GetName() );
				trap::FS_Write( str, strlen( str ), filenum );

				// properties
				str = "\r\n\t/* object properties */\r\n";
				trap::FS_Write( str, strlen( str ), filenum );

				int memberCount = objectType->GetPropertyCount();
				for( j = 0; j < memberCount; j++ )
				{
					const char *decl = va( "\t%s;\r\n", objectType->GetPropertyDeclaration( j ) );
					trap::FS_Write( decl, strlen( decl ), filenum );
				}

				// behaviours
				str = "\r\n\t/* object behaviors */\r\n";
				trap::FS_Write( str, strlen( str ), filenum );

				int behaviourCount = objectType->GetBehaviourCount();
				for( j = 0; j < behaviourCount; j++ )
				{
					// ch : FIXME: obscure function names in behaviours
					asEBehaviours behaviourType;
					int funcId = objectType->GetBehaviourByIndex( j, &behaviourType );
					if( behaviourType == asBEHAVE_ADDREF || behaviourType == asBEHAVE_RELEASE )
						continue;
					asIScriptFunction *function = engine->GetFunctionById( funcId );
					const char *decl = va( "\t%s;&s\r\n", function->GetDeclaration( false ),
							( behaviourType == asBEHAVE_FACTORY ? " /* factory */ " : "" ) );
					trap::FS_Write( decl, strlen( decl ), filenum );
				}

				// methods
				str = "\r\n\t/* object methods */\r\n";
				trap::FS_Write( str, strlen( str ), filenum );

				int methodCount = objectType->GetMethodCount();
				for( j = 0; j < methodCount; j++ )
				{
					asIScriptFunction *method = objectType->GetMethodByIndex( j );
					const char *decl = va( "\t%s;\r\n", method->GetDeclaration( false ) );
					trap::FS_Write( decl, strlen( decl ), filenum );
				}

				str = "};\r\n\r\n";
				trap::FS_Write( str, strlen( str ), filenum );
				trap::FS_FCloseFile( filenum );

				Com_Printf( "Wrote %s\n", class_file.c_str() );
			}
		}
	}

	virtual void buildReset( const char *moduleName )
	{
		if( !engine ) {
			return;
		}

		asIScriptModule *module = engine->GetModule( moduleName, asGM_ONLY_IF_EXISTS );
		if( module ) {
			engine->DiscardModule( moduleName );
		}

		garbageCollectFullCycle();
	}

	virtual void garbageCollectOneStep( void )
	{
		if( engine ) {
			engine->GarbageCollect( asGC_ONE_STEP );
		}
	}

	virtual void garbageCollectFullCycle( void )
	{
		if( engine ) {
			engine->GarbageCollect( asGC_FULL_CYCLE );
		}
	}

	// array factory
	virtual CScriptArrayInterface *createArray( unsigned int size, asIObjectType *ot )
	{
		if( as_api ) {
			return static_cast<CScriptArrayInterface *>( as_api->asCreateArrayCpp( size, static_cast<void *>( ot ) ) );
		}
		return NULL;
	}

	// AS string functions
	virtual asstring_t *createString( const char *buffer, unsigned int length )
	{
		if( as_api ) {
			return as_api->asStringFactoryBuffer( buffer, length );
		}
		return NULL;
	}

	void releaseString( asstring_t *str )
	{
		if( as_api ) {
			as_api->asStringRelease( str );
		}
	}

	virtual asstring_t *assignString( asstring_t *self, const char *string, unsigned int strlen )
	{
		if( as_api ) {
			return as_api->asStringAssignString( self, string, strlen );
		}
		return NULL;
	}

	// dictionary factory
	virtual CScriptDictionaryInterface *createDictionary( void )
	{
		if( as_api ) {
			return static_cast<CScriptDictionaryInterface *>( as_api->asCreateDictionaryCpp( static_cast<void *>( engine ) ) );
		}
		return NULL;
	}
};

//=======================================

// TODO: __new__ this one out
ASModule asmodule;

ASInterface * GetASModule( UI_Main *ui ) {
	asmodule.setUI( ui );
	return &asmodule;
}

}
