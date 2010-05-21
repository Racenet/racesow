#include "utils.h"
using namespace std;

#define TESTNAME "TestException"

// This script will cause an exception inside a class method
const char *script1 =
"class A               \n"
"{                     \n"
"  void Test(string c) \n"
"  {                   \n"
"    int a = 0, b = 0; \n"
"    a = a/b;          \n"
"  }                   \n"
"}                     \n";

static void print(asIScriptGeneric *gen)
{
	std::string *s = (std::string*)gen->GetArgAddress(0);
}

bool TestException()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);


	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "int a = 0;\na = 10/a;", &ctx); // Throws an exception
	if( r == asEXECUTION_EXCEPTION )
	{
		int func = ctx->GetExceptionFunction();
		int line = ctx->GetExceptionLineNumber();
		const char *desc = ctx->GetExceptionString();

		if( func != 0xFFFE )
		{
			printf("%s: Exception function ID is wrong\n", TESTNAME);
			fail = true;
		}
/*
		// TODO: This is temporarily not working. Only when AddRef and Release is added
		// to asIScriptFunction can we add this support again.
		const asIScriptFunction *function = engine->GetFunctionDescriptorById(func);
		if( strcmp(function->GetName(), "@ExecuteString") != 0 )
		{
			printf("%s: Exception function name is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(function->GetDeclaration(), "void @ExecuteString()") != 0 )
		{
			printf("%s: Exception function declaration is wrong\n", TESTNAME);
			fail = true;
		}
*/
		if( line != 2 )
		{
			printf("%s: Exception line number is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(desc, "Divide by zero") != 0 )
		{
			printf("%s: Exception string is wrong\n", TESTNAME);
			fail = true;
		}
	}
	else
	{
		printf("%s: Failed to raise exception\n", TESTNAME);
		fail = true;
	}

	ctx->Release();

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1));
	mod->Build();
	r = engine->ExecuteString(0, "A a; a.Test(\"test\");");
	if( r != asEXECUTION_EXCEPTION )
	{
		fail = true;
	}

	// A test to verify behaviour when exception occurs in script class constructor
	const char *script2 = "class SomeClassA \n"
	"{ \n"
	"	int A; \n"
	" \n"
	"	~SomeClassA() \n"
	"	{ \n"
	"		print('destruct'); \n"
	"	} \n"
	"} \n"
	"class SomeClassB \n"
	"{ \n"
	"	SomeClassA@ nullptr; \n"
	"	SomeClassB(SomeClassA@ aPtr) \n"
	"	{ \n"
	"		this.nullptr.A=100; // Null pointer access. After this class a is destroyed. \n"
	"	} \n"
	"} \n"
	"void test() \n"
	"{ \n"
	"	SomeClassA a; \n"
	"	SomeClassB(a); \n"
	"} \n";
	mod->AddScriptSection("script2", script2);
	r = mod->Build();
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_EXCEPTION )
	{
		fail = true;
	}

	engine->GarbageCollect();

	engine->Release();

	// Success
	return fail;
}
