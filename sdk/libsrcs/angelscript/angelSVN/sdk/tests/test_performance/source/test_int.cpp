//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestInt
{

#define TESTNAME "TestInt"

static const char *script =
"int N;                                    \n"
"                                          \n"
"void ifunc5()                             \n"
"{                                         \n"
"    N += Average( N, N );                 \n"
"}                                         \n"
"                                          \n"
"void ifunc4()                             \n"
"{                                         \n"
"    N += 2 * Average( N + 1, N + 2 );     \n"
"}                                         \n"
"                                          \n"
"void ifunc3()                             \n"
"{                                         \n"
"    N *= 2 * N;                           \n"
"}                                         \n"
"                                          \n"
"void ifunc2()                             \n"
"{                                         \n"
"    N /= 3;                               \n"
"}                                         \n"
"                                          \n"
"void iRecursion( int nRec )               \n"
"{                                         \n"
"    if ( nRec >= 1 )                      \n"
"        iRecursion( nRec - 1 );           \n"
"                                          \n"
"    if ( nRec == 5 )                      \n"
"        ifunc5();                         \n"
"    else if ( nRec == 4 )                 \n"
"        ifunc4();                         \n"
"    else if ( nRec == 3 )                 \n"
"        ifunc3();                         \n"
"    else if ( nRec == 2 )                 \n"
"        ifunc2();                         \n"
"    else                                  \n"
"        N *= 2;                           \n"
"}                                         \n"
"                                          \n"
"int TestInt()                             \n"
"{                                         \n"
"    N = 0;                                \n"
"    int i = 0;                            \n"
"                                          \n"
"    for ( i = 0; i < 250000; i++ )        \n"
"    {                                     \n"
"        Average( i, i );                  \n"
"        iRecursion( 5 );                  \n"
"                                          \n"
"        if ( N > 100 ) N = 0;             \n"
"    }                                     \n"
"                                          \n"
"    return 0;                             \n"
"}                                         \n";

int Average(int a, int b)
{
	return (a+b)/2;
}

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.15.0             : 0.422 secs\n");
	printf("AngelScript 2.18.0             : 0.397 secs\n");
	printf("AngelScript 2.18.1 WIP         : 0.338 secs\n");
	printf("AngelScript 2.19.1 WIP         : 0.348 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	engine->RegisterGlobalFunction("int Average(int, int)", asFUNCTION(Average), asCALL_CDECL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script, strlen(script), 0);
	engine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);
	mod->Build();

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(mod->GetFunctionIdByDecl("int TestInt()"));

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

	time = GetSystemTimer() - time;

	if( r != 0 )
	{
		printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("Script exception\n");
			asIScriptFunction *func = engine->GetFunctionDescriptorById(ctx->GetExceptionFunction());
			printf("Func: %s\n", func->GetName());
			printf("Line: %d\n", ctx->GetExceptionLineNumber());
			printf("Desc: %s\n", ctx->GetExceptionString());
		}
	}
	else
		printf("Time = %f secs\n", time);

	ctx->Release();
	engine->Release();
}

} // namespace







