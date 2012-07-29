#include "utils.h"

namespace TestExceptionMemory
{

static const char * const TESTNAME = "TestExceptionMemory";

static const char *script1 =
"void Test1()                   \n"
"{                              \n"
"  Object a;                    \n"
"  RaiseException();            \n"
"}                              \n"
"void Test2()                   \n"
"{                              \n"
"  RaiseException();            \n"
"  Object a;                    \n"
"}                              \n"
"void Test3()                   \n"
"{                              \n"
"  int a;                       \n"
"  Func(Object());              \n"
"}                              \n"
"void Func(Object a)            \n"
"{                              \n"
"  Object b;                    \n"
"  RaiseException();            \n"
"}                              \n"
"void Test4()                   \n"
"{                              \n"
"  Object a = SuspendObj();     \n"
"}                              \n"
"void Test5()                   \n"
"{                              \n"
"  Object a = ExceptionObj();   \n"
"}                              \n"
"void Test6()                   \n"
"{                              \n"
"  Object a(1);                 \n"
"}                              \n";
static const char *script2 =
"void Test7()                      \n"
"{                                 \n"
"  RefObj @a = @ExceptionHandle(); \n"
"}                                 \n";

static const char *script3 =
"class Pie                             \n"
"{                                     \n"
"	void foo() {}                      \n"
"}                                     \n"
"void calc()                           \n"
"{                                     \n"
"    Pie@ thing = null;                \n"
"    thing.foo(); // Null dereference  \n"
"}                                     \n";

class CObject
{
public:
	CObject() 
	{
		val = ('C' | ('O'<<8) | ('b'<<16) | ('j'<<24)); 
		mem = new int[1]; 
		*mem = ('M' | ('e'<<8) | ('m'<<16) | (' '<<24)); 
		//printf("C: %x\n", this);
	}
	~CObject() 
	{
		delete[] mem; 
		//printf("D: %x\n", this);
	}
	int val;
	int *mem;
};

void Assign_gen(asIScriptGeneric *gen)
{
	// Don't do anything
}

class CRefObject
{
public:
	CRefObject() {refCount = 1;}
	int AddRef() {return ++refCount;}
	int Release() {int r = --refCount; if( r == 0 ) delete this; return r;}
	int refCount;
};

void AddRef_gen(asIScriptGeneric*gen)
{
	CRefObject *o = (CRefObject*)gen->GetObject();
	o->AddRef();
}

void Release_gen(asIScriptGeneric *gen)
{
	CRefObject *o = (CRefObject*)gen->GetObject();
	o->Release();
}

CRefObject *RefObjFactory()
{
	return new CRefObject();
}

void RefObjFactory_gen(asIScriptGeneric *gen)
{
	*(CRefObject**)gen->GetAddressOfReturnLocation() = new CRefObject();
}

void Construct(CObject *o)
{
	new(o) CObject();
}

void Construct_gen(asIScriptGeneric *gen)
{
	CObject *o = (CObject*)gen->GetObject();
	new(o) CObject();
}

void Construct2(asIScriptGeneric *gen)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");
}

void Destruct(CObject *o)
{
	o->~CObject();
}

void Destruct_gen(asIScriptGeneric *gen)
{
	CObject *o = (CObject*)gen->GetObject();
	o->~CObject();
}

void RaiseException()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");
}

CObject SuspendObj()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->Suspend();

	return CObject();
}

void SuspendObj_gen(asIScriptGeneric*gen)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->Suspend();

	CObject obj;
	gen->SetReturnObject(&obj);
}

CObject ExceptionObj()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	return CObject();
}

void ExceptionObj_gen(asIScriptGeneric *gen)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	CObject obj;
	gen->SetReturnObject(&obj);
}

CRefObject *ExceptionHandle()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	return 0;
}

void ExceptionHandle_gen(asIScriptGeneric *gen)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	gen->SetReturnObject(0);
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Skipping this due to not supporting native calling conventions
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	bool fail = false;
	int r;
	int suspendId, exceptionId;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Object", sizeof(CObject), asOBJ_VALUE | asOBJ_APP_CLASS_CD);	
	r = engine->RegisterObjectType("RefObj", sizeof(CRefObject), asOBJ_REF); assert(r >= 0);
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Construct2), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct_gen), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Object", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct_gen), asCALL_GENERIC);
		engine->RegisterObjectMethod("Object", "Object &opAssign(const Object &in)", asFUNCTION(Assign_gen), asCALL_GENERIC);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_FACTORY, "RefObj@ f()", asFUNCTION(RefObjFactory_gen), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRef_gen), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_RELEASE, "void f()", asFUNCTION(Release_gen), asCALL_GENERIC); assert(r >= 0);
		engine->RegisterGlobalFunction("void RaiseException()", asFUNCTION(RaiseException), asCALL_GENERIC);
		suspendId   = engine->RegisterGlobalFunction("Object SuspendObj()", asFUNCTION(SuspendObj_gen), asCALL_GENERIC);
		exceptionId = engine->RegisterGlobalFunction("Object ExceptionObj()", asFUNCTION(ExceptionObj_gen), asCALL_GENERIC);
		engine->RegisterGlobalFunction("RefObj@ ExceptionHandle()", asFUNCTION(ExceptionHandle_gen), asCALL_GENERIC);
	}
	else
	{
		engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Construct2), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectBehaviour("Object", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("Object", "Object &opAssign(const Object &in)", asFUNCTION(Assign_gen), asCALL_GENERIC);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_FACTORY, "RefObj@ f()", asFUNCTION(RefObjFactory), asCALL_CDECL); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefObject, AddRef), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefObject, Release), asCALL_THISCALL); assert(r >= 0);
		engine->RegisterGlobalFunction("void RaiseException()", asFUNCTION(RaiseException), asCALL_CDECL);
		suspendId   = engine->RegisterGlobalFunction("Object SuspendObj()", asFUNCTION(SuspendObj), asCALL_CDECL);
		exceptionId = engine->RegisterGlobalFunction("Object ExceptionObj()", asFUNCTION(ExceptionObj), asCALL_CDECL);
		engine->RegisterGlobalFunction("RefObj@ ExceptionHandle()", asFUNCTION(ExceptionHandle), asCALL_CDECL);
	}



	COutStream out;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	// The object has been initialized
	r = ExecuteString(engine, "Test1()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// The object has not yet been initialized
	r = ExecuteString(engine, "Test2()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// An object has been initialized and passed by value to function that throws exception
	r = ExecuteString(engine, "Test3()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// An object has been initialized and passed by value to a function, but 
	// the function cannot be called due to the stack being full
	engine->SetEngineProperty(asEP_MAX_STACK_SIZE, 4);
	r = ExecuteString(engine, "Test3()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// An object is allocated and initialized with a call to 
	// a function that returns an object by value. The function 
	// suspends the thread. The context is then aborted.
	asIScriptContext *ctx = engine->CreateContext();
	engine->SetEngineProperty(asEP_MAX_STACK_SIZE, 0);
	r = ExecuteString(engine, "Test4()", mod, ctx);
	if( r != asEXECUTION_SUSPENDED )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}
	ctx->Abort();
	ctx->Release();

	// An object is allocated and initialized with a call to 
	// a function that returns an object by value. The function 
	// sets a script exception.
	r = ExecuteString(engine, "Test5()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// The object constructor sets the exception
	r = ExecuteString(engine, "Test6()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// A function that is supposed to return a handle sets an exception
	r = ExecuteString(engine, "Test7()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

    // Attempt to call method on null class pointer
	mod->AddScriptSection("script", script3, strlen(script3));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	r = ExecuteString(engine, "calc()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		TEST_FAILED;
	}

	// Exception happens after value object has already been destroyed
	r = ExecuteString(engine, "{\n"
		                      "  Object o;\n"
                              "}\n"
							  "RaiseException();");
	if( r != asEXECUTION_EXCEPTION )
		TEST_FAILED;

	// Exception happens after the value object has been destroyed and, 
	// the same position would also be used again after the exception
	r = ExecuteString(engine, "{ Object o; } \n"
		                      "RaiseException(); \n"
							  "Object o; \n");
	if( r != asEXECUTION_EXCEPTION )
		TEST_FAILED;

	// The code has two places where the object is destroyed, one in the if case, and 
	// and one at the end of the function. If the code doesn't go in to the if case,
	// and the exception happens afterwards, the exception handler must not think the
	// object was already destroyed.
	r = ExecuteString(engine, "Object o; bool a = false; \n"
		                      "if( a ) return; \n"
							  "RaiseException(); \n");
	if( r != asEXECUTION_EXCEPTION )
		TEST_FAILED;

	// Calling a function that returns an object directly must release the object upon releasing the context
	ctx = engine->CreateContext();
	ctx->Prepare(suspendId);
	ctx->Execute();
	ctx->Release();

	// Calling a function that returns an object but raised an exception shouldn't try to destroy the object
	ctx = engine->CreateContext();
	ctx->Prepare(exceptionId);
	ctx->Execute();
	ctx->Release();

 	engine->Release();

	// Success
	return fail;
}

} // namespace

