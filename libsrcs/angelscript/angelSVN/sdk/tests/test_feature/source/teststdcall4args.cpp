//
// Tests calling of a stdcall function from a script with four parameters
// of different types
//
// Test author: Andreas J�nsson
//

#include "utils.h"

#define TESTNAME "TestStdcall4Args"

static bool testVal = false;
static bool called = false;

static int	t1 = 0;
static float	t2 = 0;
static double	t3 = 0;
static char	t4 = 0;

static void STDCALL cfunction(int f1, float f2, double f3, int f4) 
{
	called = true;

	t1 = f1;
	t2 = f2;
	t3 = f3;
	t4 = (char)f4;

	testVal = (f1 == 10) && (f2 == 1.92f) && (f3 == 3.88) && (f4 == 97);
}

bool TestStdcall4Args()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(int, float, double, int)", asFUNCTION(cfunction), asCALL_STDCALL);

	engine->ExecuteString(0, "cfunction(10, 1.92f, 3.88, 97)");

	if (!called) {
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} else if (!testVal) {
		printf("\n%s: testVal is not of expected value. Got (%d, %f, %f, %c), expected (%d, %f, %f, %c)\n\n", TESTNAME, t1, t2, t3, t4, 10, 1.92f, 3.88, 97);
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
