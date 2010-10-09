//
// This test shows how to register the std::string to be used in the scripts.
// It also used to verify that objects are always constructed before destructed.
//
// Author: Andreas Jonsson
//

#include "utils.h"
using namespace std;

static const char * const TESTNAME = "TestStdString";

static string printOutput;

static void PrintString(string &str)
{
	printOutput = str;
}

static void PrintStringVal(string str)
{
	printOutput = str;
}

// This script tests that variables are created and destroyed in the correct order
static const char *script =
"void blah1()\n"
"{\n"
"	if(true)\n"
"		return;\n"
"\n"
"	string blah = \"Bleh1!\";\n"
"}\n"
"\n"
"void blah2()\n"
"{\n"
"	string blah = \"Bleh2!\";\n"
"\n"
"	if(true)\n"
"		return;\n"
"}\n";

static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n"
"void testString2()                        \n"
"{                                         \n"
"  string str = \"Hello World!\";          \n"
"  printVal(str);                          \n"
"}                                         \n";

static const char *script3 =
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"obj a(\"test\");               \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

static void Construct1(void *o)
{
	UNUSED_VAR(o);
}

static void Construct2(string &str, void *o)
{
	UNUSED_VAR(str);
	UNUSED_VAR(o);
}

static void Destruct(void *o)
{
	UNUSED_VAR(o);
}

static void StringByVal(string &str1, string str2)
{
	assert( str1 == str2 );
}

//--> new: object method string argument test
class StringConsumer
{
public:
	void Consume(string str)
	{
		printOutput = str;
	}
};
static StringConsumer consumerObject;
//<-- new: object method string argument test


class Http
{
public:
bool Get(const string & /*szURL*/, string &szHTML)
{
	assert(&szHTML != 0);
	return false;
}
};

bool TestTwoStringTypes();

bool TestStdString()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = TestTwoStringTypes();

	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void printVal(string)", asFUNCTION(PrintStringVal), asCALL_CDECL);

	engine->RegisterObjectType("obj", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct1), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(string &in)", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);

	engine->RegisterGlobalFunction("void StringByVal(string &in, string)", asFUNCTION(StringByVal), asCALL_CDECL);

	//--> new: object method string argument test
	engine->RegisterObjectType("StringConsumer", sizeof(StringConsumer), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectMethod("StringConsumer", "void Consume(string str)", asMETHOD(StringConsumer,Consume), asCALL_THISCALL);
	engine->RegisterGlobalProperty("StringConsumer consumerObject", &consumerObject);
	//<-- new: object method string argument test

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("string", script);
	mod->Build();

	int r = ExecuteString(engine, "blah1(); blah2();", mod);
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2);
	mod->Build();

	ExecuteString(engine, "testString()", mod);

	if( printOutput != "hello Ida" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	ExecuteString(engine, "string s = \"test\\\\test\\\\\"", mod);

	// Verify that it is possible to use the string in constructor parameters
	ExecuteString(engine, "obj a; a = obj(\"test\")");
	ExecuteString(engine, "obj a(\"test\")");

	// Verify that it is possible to pass strings by value
	printOutput = "";
	ExecuteString(engine, "testString2()", mod);
	if( printOutput != "Hello World!" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	printOutput = "";
	ExecuteString(engine, "string a; a = 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	ExecuteString(engine, "string a; a += 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	ExecuteString(engine, "string a = \"a\" + 1; print(a);");
	if( printOutput != "a1" ) fail = true;

	printOutput = "";
	ExecuteString(engine, "string a = 1 + \"a\"; print(a);");
	if( printOutput != "1a" ) fail = true;

	printOutput = "";
	ExecuteString(engine, "string a = 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	ExecuteString(engine, "print(\"a\" + 1.2)");
	if( printOutput != "a1.2") fail = true;

	printOutput = "";
	ExecuteString(engine, "print(1.2 + \"a\")");
	if( printOutput != "1.2a") fail = true;

	ExecuteString(engine, "StringByVal(\"test\", \"test\")");

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3);
	if( mod->Build() < 0 )
	{
		fail = true;
	}

	//--> new: object method string argument test
	printOutput = "";
	ExecuteString(engine, "consumerObject.Consume(\"This is my string\")");
	if( printOutput != "This is my string") fail = true;
	//<-- new: object method string argument test

	engine->RegisterObjectType("Http", sizeof(Http), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectMethod("Http","bool get(const string &in,string &out)", asMETHOD(Http,Get),asCALL_THISCALL);
	ExecuteString(engine, "Http h; string str; h.get(\"string\", str);");
	ExecuteString(engine, "Http h; string str; string a = \"a\"; h.get(\"string\"+a, str);");

	engine->Release();

	return fail;
}

//////////////////////////////
// This test was reported by dxj19831029 on Sep 9th, 2008

class _String
{
public:
	_String() {}
	_String(_String &o) {buffer = o.buffer;}
	~_String() {}
	_String &operator=(const _String&o) {buffer = o.buffer; return *this;}
	_String &operator=(const std::string&o) {buffer = o; return *this;}
	_String &operator+=(const _String&o) {buffer += o.buffer; return *this;}
	_String &operator+=(const std::string&o) {buffer += o; return *this;}
	std::string buffer;
};

void stringDefaultCoonstructor(void *mem)
{
	new(mem) _String();
}

void stringCopyConstructor(_String &o, void *mem)
{
	new(mem) _String(o);
}

void stringStringConstructor(std::string &o, void *mem)
{
	new(mem) _String();
	((_String*)mem)->buffer = o;
}

void stringDecontructor(_String &s)
{
	s.~_String();
}

_String operation_StringAdd(const _String &a, const _String &b)
{
	_String r;
	r.buffer = a.buffer + b.buffer;
	return r;
}

_String operation_StringStringAdd(const _String &a, const std::string &b)
{
	_String r;
	r.buffer = a.buffer + b;
	return r;
}

_String operationString_StringAdd(const std::string &a, const _String &b)
{
	_String r;
	r.buffer = a + b.buffer;
	return r;
}

bool TestTwoStringTypes()
{
	bool fail = false;
	int r;
	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);

	// Register the second string type
	engine->RegisterObjectType("_String", sizeof(_String),   asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f()",                 asFUNCTION(stringDefaultCoonstructor), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f(const _String &in )",		asFUNCTION(stringCopyConstructor), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(stringStringConstructor), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(stringDecontructor), asCALL_CDECL_OBJLAST);
	// =
	engine->RegisterObjectMethod("_String", "_String &opAssign(const string &in )", asMETHODPR(_String, operator=, (const string &), _String& ), asCALL_THISCALL);
	engine->RegisterObjectMethod("_String", "_String &opAssign(const _String &in )", asMETHODPR(_String, operator=, (const _String &), _String& ), asCALL_THISCALL);
	// +=
	engine->RegisterObjectMethod("_String", "_String &opAddAssign(const string &in )", asMETHODPR(_String, operator+=, (const string &), _String& ), asCALL_THISCALL);
	engine->RegisterObjectMethod("_String", "_String &opAddAssign(const _String &in )", asMETHODPR(_String, operator+=, (const _String &), _String& ), asCALL_THISCALL);
	// comparison
/*	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const _String &in, const _String &in)", asFUNCTION(compare_StringEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const _String &in, const string &in)", asFUNCTION(compare_StringStringEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const string &in, const _String &in)", asFUNCTION(compareString_StringEqual), asCALL_CDECL);
	// not equal
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const _String &in, const _String &in)", asFUNCTION(compare_StringNotEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const _String &in, const string &in)", asFUNCTION(compare_StringStringNotEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const string &in, const _String &in)", asFUNCTION(compareString_StringNotEqual), asCALL_CDECL);
*/	// +
	r = engine->RegisterObjectMethod("_String" , "_String opAdd(const _String &in) const", asFUNCTION(operation_StringAdd), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("_String" , "_String opAdd(const string &in) const", asFUNCTION(operation_StringStringAdd), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("_String" , "_String opAdd_r(const string &in) const", asFUNCTION(operationString_StringAdd), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = ExecuteString(engine, "_String('ab') + 'a'");
	if( r < 0 )
		fail = true;

	r = ExecuteString(engine, "_String a; a+= 'a' + 'b' ; _String b = 'c'; a += b + 'c';");
	if( r < 0 )
		fail = true;

	r = ExecuteString(engine, "string a; a+= 'a' + 'b' ; string b = 'c'; a += b + 'c';");
	if( r < 0 )
		fail = true;

	engine->Release();

	return fail;
}
