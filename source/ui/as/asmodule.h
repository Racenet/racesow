#pragma once
#ifndef __UI_ASMODULE_H__
#define __UI_ASMODULE_H__

// forward declare some angelscript stuff without including angelscript
class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class asIObjectType;
class asIScriptFunction;

class CScriptArrayInterface;
class CScriptDictionaryInterface;

struct asstring_s;
typedef struct asstring_s asstring_t;

namespace ASUI {

// base class for AS
class ASInterface
{
public:
	virtual bool Init( void ) = 0;
	virtual void Shutdown( void ) = 0;

	virtual asIScriptEngine *getEngine( void ) = 0;
	virtual asIScriptContext *getContext( void ) = 0;
	virtual asIScriptModule *getModule( const char *name = NULL ) = 0;

	// only valid during execution of script functions
	virtual asIScriptContext *getActiveContext( void ) = 0;
	virtual asIScriptModule *getActiveModule( void ) = 0;

	virtual asIObjectType *getStringObjectType( void ) = 0;

	// called to start a building round
	// note that temporary name assigned to the build (module)
	// may be changed in the finishBuilding call
	virtual void startBuilding( const char *tempModuleName ) = 0;

	// compile all added scripts, set final module name
	virtual bool finishBuilding( const char *finalModuleName = NULL ) = 0;

	// are we in the middle of start/finishBuilding?
	virtual bool isBuilding( void ) = 0;

	// adds a script either to module, or the following.
	// If no name is provided, script_XXX is used
	virtual bool addScript( asIScriptModule *module, const char *name, const char *code ) = 0;

	// adds a function to module, despite if finishBuilding has been called
	virtual bool addFunction( asIScriptModule *module, const char *name, const char *code, asIScriptFunction **outFunction ) = 0;

	// testing, dumapi command
	virtual void dumpAPI( const char *path ) = 0;

	// reset all potentially referenced global vars
	// (used for releasing reference-counted Rocket objects)
	virtual void buildReset( const char *moduleName ) = 0;

	// garbage collector interfaces
	virtual void garbageCollectOneStep( void ) = 0;
	virtual void garbageCollectFullCycle( void ) = 0;

	// creates a new array object, which can be natively passed on to scripts
	virtual CScriptArrayInterface *createArray( unsigned int size, asIObjectType *ot ) = 0;

	// AS string functions
	virtual asstring_t *createString( const char *buffer, unsigned int length ) = 0;
	virtual asstring_t *assignString( asstring_t *self, const char *string, unsigned int strlen ) = 0;
	virtual void releaseString( asstring_t *str ) = 0;

	// creates a new dictionary object, which can be natively passed on to scripts
	virtual CScriptDictionaryInterface *createDictionary( void ) = 0;

	// caching, TODO!
	// void *getBytecode(size_t &size);
	// void setByteCode(void *bytecode, size_t size);

	// void saveBytecode(const char *filename);
	// void loadBytecode(const char *filename);
};

ASInterface * GetASModule( WSWUI::UI_Main *main );

}
#endif
