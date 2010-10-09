#include "utils.h"

namespace TestObjZeroSize
{

static const char * const TESTNAME = "TestObjZeroSize";

class CObject
{
public:
	CObject() {val = 0;refCount = 1;}
	~CObject() {}
	void AddRef() {refCount++;}
	void Release() {if( --refCount == 0 ) delete this;}
	void Set(int v) {val = v;}
	int Get() {return val;}
	int val;
	int refCount;
};

CObject *Factory()
{
	return new CObject();
}

CObject obj;

CObject *CreateObject()
{
	CObject *obj = new CObject();
	
	// The constructor already initialized the reference counter with 1

	return obj;
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptArray(engine, true);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// Register an object type that cannot be instanciated by the script, but can be interacted with through object handles
	engine->RegisterObjectType("Object", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_ADDREF, "void f()", asMETHOD(CObject, AddRef), asCALL_THISCALL);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_RELEASE, "void f()", asMETHOD(CObject, Release), asCALL_THISCALL);
	engine->RegisterObjectMethod("Object", "void Set(int)", asMETHOD(CObject, Set), asCALL_THISCALL);
	engine->RegisterObjectMethod("Object", "int Get()", asMETHOD(CObject, Get), asCALL_THISCALL);
	engine->RegisterObjectProperty("Object", "int val", offsetof(CObject, val));

	engine->RegisterGlobalProperty("Object obj", &obj);
	engine->RegisterGlobalFunction("Object @CreateObject()", asFUNCTION(CreateObject), asCALL_CDECL);

	COutStream out;
	CBufferedOutStream bout;

	// Must not allow it to be declared as local variable
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "Object obj;");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 8) : Error   : Data type can't be 'Object'\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	// Must not allow it to be declared as global variable
	bout.buffer = "";
	const char *script = "Object obj2;";
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r >= 0 || bout.buffer != "script (1, 1) : Error   : Data type can't be 'Object'\n"
		  					     "script (1, 8) : Error   : No default constructor for object of type 'Object'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		fail = true;
	}
	engine->DiscardModule(0);

	// It must not be allowed as sub type of array
	bout.buffer = "";
	r = ExecuteString(engine, "Object[] obj;");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 7) : Error   : Data type can't be 'Object'\n" )
	{
		printf("%s", bout.buffer.c_str());
		fail = true;
	}

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = ExecuteString(engine, "Object @obj;");
	if( r < 0 )
	{
		printf("%s: Failed to compile\n", TESTNAME);
		fail = true;
	}

	r = ExecuteString(engine, "Object@ obj = @CreateObject();");
	if( r < 0 )
	{
		printf("%s: Failed to compile\n", TESTNAME);
		fail = true;
	}

	r = ExecuteString(engine, "CreateObject();");
	if( r < 0 )
	{
		printf("%s: Failed to compile\n", TESTNAME);
		fail = true;
	}

	r = ExecuteString(engine, "Object@ obj = @CreateObject(); @obj = @CreateObject();");
	if( r < 0 )
	{
		printf("%s: Failed to compile\n", TESTNAME);
		fail = true;
	}

	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "Object@ obj = @CreateObject(); obj = CreateObject();");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 36) : Error   : There is no copy operator for this type available.\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	bout.buffer = "";
	r = ExecuteString(engine, "@CreateObject() = @CreateObject();");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 1) : Error   : Reference is temporary\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	bout.buffer = "";
	r = ExecuteString(engine, "CreateObject() = CreateObject();");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 1) : Error   : Reference is temporary\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	// Test object with zero size as member of script class
	script = "  \n\
	 class myclass          \n\
	 {                      \n\
	   Object obj;          \n\
	 }                      \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 || bout.buffer != "script (4, 5) : Error   : Data type can't be 'Object'\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

