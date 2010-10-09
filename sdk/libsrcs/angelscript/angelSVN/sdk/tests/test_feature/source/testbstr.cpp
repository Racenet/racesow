// 
// Test designed to verify functionality of the bstr type
//
// Written by Andreas J�nsson 
//

#include "utils.h"
#include "bstr.h"

static const char * const TESTNAME = "TestBStr";

static asBSTR NewString(int length)
{
	return asBStrAlloc(length);
}

static const char *script =
"void test(bstr a, bstr b)  \n"
"{                          \n"
"  assert(a == \"a\");      \n"
"  assert(b == \"b\");      \n"
"}                          \n";

bool TestBStr()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Test skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterBStr(engine);

	engine->RegisterGlobalFunction("bstr NewString(int)", asFUNCTION(NewString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	int r = ExecuteString(engine, "bstr s = NewString(10)");
	if( r < 0 ) 
	{
		printf("%s: ExecuteString() failed\n", TESTNAME);
		ret = true;
	}
	else if( r != asEXECUTION_FINISHED )
	{
		printf("%s: ExecuteString() returned %d\n", TESTNAME, r);
		ret = true;
	}

	// Test passing bstr strings to a script function
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		ret = true;
	}

	int funcId = mod->GetFunctionIdByIndex(0);
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(funcId);

	// Create the object and initialize it, then give 
	// the pointer directly to the script engine. 
	// The script engine will free the object.
	asBSTR *a = (asBSTR*)engine->CreateScriptObject(engine->GetTypeIdByDecl("bstr"));
	*a = asBStrAlloc(1);
	strcpy((char*)*a, "a");
	*(asBSTR**)ctx->GetAddressOfArg(0) = a;

	// Create a local instance and have the script engine copy it.
	// The application must free its copy of the object.
	asBSTR b = asBStrAlloc(1);
	strcpy((char*)b, "b");
	ctx->SetArgObject(1, &b);
	asBStrFree(b);

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		ret = true;

	if( ctx ) ctx->Release();

	engine->Release();

	return ret;
}
