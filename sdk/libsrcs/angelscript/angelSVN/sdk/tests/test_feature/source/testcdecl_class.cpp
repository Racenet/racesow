//
// This test was designed to test the asOBJ_CLASS flag with cdecl
//
// Author: Andreas J�nsson
//

#include "utils.h"

#define TESTNAME "TestCDecl_Class"

class Class1
{
public:
	unsigned long a;
};

class Class2
{
public:
	unsigned long a;
	unsigned long b;
};

class Class3
{
public:
	unsigned long a;
	unsigned long b;
	unsigned long c;
};

static Class1 class1()
{
	Class1 c = {0xDEADC0DE};
	return c;
}

static Class2 class2()
{
	Class2 c = {0xDEADC0DE, 0x01234567};
	return c;
}

static Class3 class3()
{
	Class3 c = {0xDEADC0DE, 0x01234567, 0x89ABCDEF};
	return c;
}

static Class1 c1;
static Class2 c2;
static Class3 c3;


bool TestCDecl_Class()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("class1", sizeof(Class1), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectType("class2", sizeof(Class2), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectType("class3", sizeof(Class3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	
	engine->RegisterGlobalProperty("class1 c1", &c1);
	engine->RegisterGlobalProperty("class2 c2", &c2);
	engine->RegisterGlobalProperty("class3 c3", &c3);

	engine->RegisterGlobalFunction("class1 _class1()", asFUNCTION(class1), asCALL_CDECL);
	engine->RegisterGlobalFunction("class2 _class2()", asFUNCTION(class2), asCALL_CDECL);
	engine->RegisterGlobalFunction("class3 _class3()", asFUNCTION(class3), asCALL_CDECL);

	COutStream out;

	c1.a = 0;

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	int r = engine->ExecuteString(0, "c1 = _class1();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c1.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign object returned from function. c1.a = %X\n", TESTNAME, c1.a);
		fail = true;
	}


	c2.a = 0;
	c2.b = 0;

	r = engine->ExecuteString(0, "c2 = _class2();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c2.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign object returned from function. c2.a = %X\n", TESTNAME, c2.a);
		fail = true;
	}

	if( c2.b != 0x01234567 )
	{
		printf("%s: Failed to assign object returned from function. c2.b = %X\n", TESTNAME, c2.b);
		fail = true;
	}

	c3.a = 0;
	c3.b = 0;
	c3.c = 0;

	r = engine->ExecuteString(0, "c3 = _class3();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c3.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign object returned from function. c3.a = %X\n", TESTNAME, c3.a);
		fail = true;
	}

	if( c3.b != 0x01234567 )
	{
		printf("%s: Failed to assign object returned from function. c3.b = %X\n", TESTNAME, c3.b);
		fail = true;
	}

	if( c3.c != 0x89ABCDEF )
	{
		printf("%s: Failed to assign object returned from function. c3.c = %X\n", TESTNAME, c3.c);
		fail = true;
	}

	engine->Release();

	return fail;
}
