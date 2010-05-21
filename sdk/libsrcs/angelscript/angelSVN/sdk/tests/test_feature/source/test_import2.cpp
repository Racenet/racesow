//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport2
{

#define TESTNAME "TestImport2"




static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"void Run()                                   \n"
"{                                            \n"
"  Test();                                    \n"
"}                                            \n";

static const char *script2 =
"void Test()             \n"
"{                       \n"
"  // Cause an exception \n"
"  CheckFunc();          \n"
"}                       \n";

bool failed = false;

void CheckFunc(asIScriptGeneric *)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		asIScriptEngine *engine = ctx->GetEngine();
		int funcID = ctx->GetCurrentFunction();
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		if( strcmp(func->GetModuleName(), "DynamicModule") != 0 )
			failed = true;

		if( strcmp(func->GetDeclaration(), "void Test()") != 0 )
			failed = true;

		ctx->SetException("Generated exception");
	}
	else
		failed = true;
}

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("void CheckFunc()", asFUNCTION(CheckFunc), asCALL_GENERIC);

	COutStream out;
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME ":1", script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod->Build();

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME ":2", script2, strlen(script2), 0);
	mod->Build();

	// Bind all functions that the module imports
	engine->GetModule(0)->BindAllImportedFunctions();

	asIScriptContext *ctx;
	int r = engine->ExecuteString(0, "Run()", &ctx);
	if( r == asEXECUTION_EXCEPTION )
	{
		int funcID = ctx->GetExceptionFunction();
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		if( strcmp(func->GetModuleName(), "DynamicModule") != 0 )
			failed = true;

		if( strcmp(func->GetDeclaration(), "void Test()") != 0 )
			failed = true;
	}
	if( ctx ) ctx->Release();
	engine->Release();

	if( failed )
	{
		fail = true;
		printf("%s: failed\n", TESTNAME);
	}
	

	// Success
	return fail;
}

} // namespace

