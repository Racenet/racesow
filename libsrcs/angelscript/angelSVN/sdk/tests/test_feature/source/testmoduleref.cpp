//
// Test to verify that modules are released correctly after use
//
// Author: Andreas J�nsson
//

#include "utils.h"

static const char * const TESTNAME = "TestModuleRef";
static const char *script = "int global; void Test() {global = 0;}";

bool TestModuleRef()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	asIScriptModule *mod = engine->GetModule("a", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	if( mod->Build() < 0 )
	{
		printf("%s: failed to build module a\n", TESTNAME);
		TEST_FAILED;
	}

	int funcID = engine->GetModule("a")->GetFunctionIdByDecl("void Test()");
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(funcID);

	if( engine->GetModule("a")->GetFunctionCount() < 0 )
	{
		printf("%s: Failed to get function count\n", TESTNAME);
		TEST_FAILED;
	}

	engine->DiscardModule("a");
	if( engine->GetModule("a") )
	{
		printf("%s: Module was not discarded\n", TESTNAME);
		TEST_FAILED;
	} 

	int r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Execution failed\n", TESTNAME);
		TEST_FAILED;
	}

	ctx->Release();

	engine->Release();

	return fail;
}
