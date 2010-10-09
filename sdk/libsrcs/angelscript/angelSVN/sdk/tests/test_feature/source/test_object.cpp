#include "utils.h"

namespace TestObject
{

const char * const TESTNAME = "TestObject";



static const char *script1 =
"void TestObject()              \n"
"{                              \n"
"  Object a = TestReturn();     \n"
"  a.Set(1);                    \n"
"  TestArgVal(a);               \n"
"  Assert(a.Get() == 1);        \n"
"  TestArgRef(a);               \n"
"  Assert(a.Get() != 1);        \n"
"  TestProp();                  \n"
"  TestSysArgs();               \n"
"  TestSysReturn();             \n"
"  TestGlobalProperty();        \n"
"}                              \n"
"Object TestReturn()            \n"
"{                              \n"
"  return Object();             \n"
"}                              \n"
"void TestArgVal(Object a)      \n"
"{                              \n"
"}                              \n"
"void TestArgRef(Object &out a) \n"
"{                              \n"
"  a = Object();                \n"
"}                              \n"
"void TestProp()                \n"
"{                              \n"
"  Object a;                    \n"
"  a.val = 2;                   \n"
"  Assert(a.Get() == a.val);    \n"
"  Object2 b;                   \n"
"  b.obj = a;                   \n"
"  Assert(b.obj.val == 2);      \n"
"}                              \n"
"void TestSysReturn()             \n"
"{                                \n"
"  // return object               \n"
"  // by val                      \n"
"  Object a;                      \n"
"  a = TestReturnObject();        \n"
"  Assert(a.val == 12);           \n"
"  // by ref                      \n"
"  a.val = 12;                    \n"
"  TestReturnObjectRef() = a;     \n"
"  a = TestReturnObjectRef();     \n"
"  Assert(a.val == 12);           \n"
"}                                \n"
"void TestSysArgs()               \n"
"{                                \n"
"  Object a;                      \n"
"  a.val = 12;                    \n"
"  TestSysArgRef(a);              \n"
"  Assert(a.val == 2);            \n"
"  a.val = 12;                    \n"
"  TestSysArgVal(a);              \n"
"  Assert(a.val == 12);           \n"
"}                                \n"
"void TestGlobalProperty()        \n"
"{                                \n"
"  Object a;                      \n"
"  a.val = 12;                    \n"
"  TestReturnObjectRef() = a;     \n"
"  a = obj;                       \n"
"  obj = a;                       \n"
"}                                \n";

class CObject
{
public:
	CObject() {val = 0;/*printf("C:%x\n",this);*/}
	~CObject() {/*printf("D:%x\n", this);*/}
	void Set(int v) {val = v;}
	int Get() {return val;}
	int &GetRef() {return val;}
	int val;
};

void Construct(CObject *o)
{
	new(o) CObject();
}

void Destruct(CObject *o)
{
	o->~CObject();
}

class CObject2
{
public:
	CObject obj;
};

void Construct2(CObject2 *o)
{
	new(o) CObject2();
}

void Destruct2(CObject2 *o)
{
	o->~CObject2();
}

CObject TestReturnObject()
{
	CObject obj;
	obj.val = 12;
	return obj;
}

CObject obj;
CObject *TestReturnObjectRef()
{
	return &obj;
}

void TestSysArgVal(CObject obj)
{
	assert(obj.val == 12);
	obj.val = 0;
}

void TestSysArgRef(CObject &obj)
{
// We're not receiving the true object, only a reference to a place holder for the output value
	assert(obj.val == 0);
	obj.val = 2;
}

bool Test2();

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = Test2();
	int r;
	int funcId;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->RegisterObjectType("Object", sizeof(CObject), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);	
	engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);
	funcId = engine->RegisterObjectMethod("Object", "void Set(int)", asMETHOD(CObject, Set), asCALL_THISCALL);
	engine->RegisterObjectMethod("Object", "int Get()", asMETHOD(CObject, Get), asCALL_THISCALL);
	engine->RegisterObjectProperty("Object", "int val", offsetof(CObject, val));
	r = engine->RegisterObjectMethod("Object", "int &GetRef()", asMETHOD(CObject, GetRef), asCALL_THISCALL); assert( r >= 0 );

	engine->RegisterObjectType("Object2", sizeof(CObject2), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectBehaviour("Object2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Object2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("Object2", "Object obj", offsetof(CObject2, obj));

	engine->RegisterGlobalFunction("Object TestReturnObject()", asFUNCTION(TestReturnObject), asCALL_CDECL);
	engine->RegisterGlobalFunction("Object &TestReturnObjectRef()", asFUNCTION(TestReturnObjectRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestSysArgVal(Object)", asFUNCTION(TestSysArgVal), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestSysArgRef(Object &out)", asFUNCTION(TestSysArgRef), asCALL_CDECL);

	engine->RegisterGlobalProperty("Object obj", &obj);

	// Test objects with no default constructor
	engine->RegisterObjectType("ObjNoConstruct", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

	COutStream out;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestObject()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	ExecuteString(engine, "ObjNoConstruct a; a = ObjNoConstruct();");
	if( r != 0 )
	{
		fail = true;
		printf("%s: Failed\n", TESTNAME);
	}

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "Object obj; float r = 0; obj = r;");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 32) : Error   : Can't implicitly convert from 'float' to 'const Object&'.\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		printf("%s", bout.buffer.c_str());
		fail = true;
	}

	// Verify that the registered types can be enumerated
	int count = engine->GetObjectTypeCount();
	if( count != 3 )
		fail = true;
	asIObjectType *type = engine->GetObjectTypeByIndex(0);
	if( strcmp(type->GetName(), "Object") != 0 )
		fail = true;
	
	// Test calling an application registered method directly with context
	ctx = engine->CreateContext();
	ctx->Prepare(funcId);
	ctx->SetObject(&obj);
	ctx->SetArgDWord(0, 42);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( obj.val != 42 )
		fail = true;
	ctx->Release();

	// Test GetObjectTypeCount for the module
	const char *script2 = "class ScriptType {}";
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2);
	mod->Build();

	count = engine->GetObjectTypeCount();
	if( count != 3 )
		fail = true;

	count = engine->GetModule(0)->GetObjectTypeCount();
	if( count != 1 )
		fail = true;


	// Test assigning value to reference returned by class method where the reference points to a member of the class
	// This test attempts to verify that the object isn't freed before the reference goes out of scope.
	r = ExecuteString(engine, "Object o; o.GetRef() = 10;");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "Object().GetRef() = 10;");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}
	if( bout.buffer != "ExecuteString (1, 10) : Warning : A non-const method is called on temporary object. Changes to the object may be lost.\n"
		               "ExecuteString (1, 10) : Info    : int& Object::GetRef()\n" )
	{
		printf("%s", bout.buffer.c_str());
		fail = true;
	}

	// TODO: Make the same test with the index operator

	engine->Release();

	// Success
	return fail;
}

class Creep
{
};

void dummy(void*)
{
}

bool Test2()
{
	bool fail = false;

	int r;
	CBufferedOutStream bout;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("Creep", sizeof(Creep), asOBJ_VALUE | asOBJ_APP_CLASS); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Creep", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(dummy), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Creep", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(dummy), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Must not allow a value type to be declared as object handle
	const char *script = "void Test(Creep @c) {}";
	asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (1, 17) : Error   : Object handle is not supported for this type\n"
	                   "script (1, 1) : Info    : Compiling void Test(Creep)\n"
	                   "script (1, 17) : Error   : Object handle is not supported for this type\n" )
	{
		fail = true;
		printf("%s", bout.buffer.c_str());
	}

	engine->Release();

	return fail;
}

} // namespace

