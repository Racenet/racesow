#include "utils.h"
using std::string;
#include "scriptstring.h"
#include "../../../add_on/scripthelper/scripthelper.h"
#include <vector>

namespace TestScriptString
{

static string printOutput;

// This function receives the string by reference
// (in fact it is a reference to copy of the string)
static void PrintString(asIScriptGeneric *gen)
{
	CScriptString *str = (CScriptString*)gen->GetArgAddress(0);
	printOutput = str->buffer;
}

// This function shows how to receive an
// object handle from the script engine
static void SetString(asIScriptGeneric *gen)
{
	CScriptString *str = (CScriptString*)gen->GetArgAddress(0);
	if( str )
	{
		str->buffer = "Handle to a string";

		// The generic interface will release the handle in the parameter for us
		// str->Release();
	}
}

// This function shows how to receive a reference
// to an object handle from the script engine
static void SetString2(asIScriptGeneric *gen)
{
	CScriptString *str = *(CScriptString**)gen->GetArgAddress(0);
	if( str )
	{
		str->buffer = "Handle to a string";

		// The generic interface will release the handle in the parameter for us
		// str->Release();
	}
}

// This script tests that variables are created and destroyed in the correct order
static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n";

static const char *script3 =
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

static const char *script4 =
"void test()                    \n"
"{                              \n"
"   string s = \"\"\"           \n"
"Heredoc\\x20test\n"
"            \"\"\" \"\\x21\";  \n"
"   print(s);                   \n"
"}                              \n";

static const char *script5 =
"void test( string @ s )         \n"
"{                               \n"
"   string t = s;                \n"
"}                               \n"
"void Main()                     \n"
"{                               \n"
"   test(\"this is a test\");    \n"
"}                               \n";

static const char *script6 =
"void Main()                     \n"
"{                               \n"
"   test(\"this is a test\");    \n"
"}                               \n"
"void test( string @ s )         \n"
"{                               \n"
"   string t = s;                \n"
"}                               \n";

static const char *script7 =
"void test()                    \n"
"{                              \n"
"   Func(\"test\");             \n"
"}                              \n"
"void Func(const string &in str)\n"
"{                              \n"
"}                              \n";

static const char *script8 =
"void test()                    \n"
"{                              \n"
"   Func('test');               \n"
"}                              \n"
"string Func(string & str)      \n"
"{                              \n"
"  return str;                  \n"
"}                              \n";


//bool Get(int * /*obj*/, const CScriptString &szURL, CScriptString &szHTML)
void Get(asIScriptGeneric *gen)
{
	const CScriptString *szURL = (CScriptString*)gen->GetArgObject(0);
	CScriptString *szHTML = (CScriptString*)gen->GetArgObject(1);

	assert(szHTML != 0);
	assert(szURL->buffer == "stringtest");
	szHTML->buffer = "output";

	gen->SetReturnDWord(false);
}

void GetConstStringRef(asIScriptGeneric *gen)
{
	static string test("test");
	gen->SetReturnAddress(&test);
}

// void TestFunc(int, string&)
void TestFunc(asIScriptGeneric *gen)
{
	int arg0              = *(int*)gen->GetAddressOfArg(0);
	CScriptString *arg1 = *(CScriptString**)gen->GetAddressOfArg(1);

	assert( arg0 == 0 );
	assert( arg1->buffer == "test" );
}

void PrintRef(asIScriptGeneric *gen)
{
	std::string *ref = *(std::string**)gen->GetAddressOfArg(0);
	assert( ref != 0 );
	assert( *ref == "Some String" );
}

bool TestUTF16();
bool Test2();

bool Test()
{
	bool fail = false;
	COutStream out;
	CBufferedOutStream bout;
	asIScriptEngine *engine = 0;
	asIScriptModule *mod = 0;
	int r;

	fail = Test2() || fail;
	fail = TestUTF16() || fail;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptArray(engine, false);
	RegisterScriptString(engine);
	RegisterScriptStringUtils(engine);

	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set(string@)", asFUNCTION(SetString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set2(string@&in)", asFUNCTION(SetString2), asCALL_GENERIC);
	engine->RegisterGlobalFunction("const string &getconststringref()", asFUNCTION(GetConstStringRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);


	// Test index operator for temp strings
	r = ExecuteString(engine, "assert('abc'[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "assert(string('abc')[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = ExecuteString(engine, "string a = 'abc'; assert(a[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;


	// Test string copy constructor
	r = ExecuteString(engine, "string tst(getconststringref()); print(tst);");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	if( printOutput != "test" ) TEST_FAILED;


	printOutput = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("TestScriptString", script2, strlen(script2), 0);
	mod->Build();

	ExecuteString(engine, "testString()", mod);

	if( printOutput != "hello Ida" )
	{
		TEST_FAILED;
		printf("%s: Failed to print the correct string\n", "TestScriptString");
	}

	ExecuteString(engine, "string s = \"test\\\\test\\\\\"");

	// Verify that it is possible to use the string in constructor parameters
	printOutput = "";
	ExecuteString(engine, "string a; a = 1; print(a);");
	if( printOutput != "1" ) TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "string a; a += 1; print(a);");
	if( printOutput != "1" ) TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "string a = \"a\" + 1; print(a);");
	if( printOutput != "a1" ) TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "string a = 1 + \"a\"; print(a);");
	if( printOutput != "1a" ) TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "string a = 1; print(a);");
	if( printOutput != "1" ) TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "print(\"a\" + 1.2)");
	if( printOutput != "a1.2") TEST_FAILED;

	printOutput = "";
	ExecuteString(engine, "print(1.2 + \"a\")");
	if( printOutput != "1.2a")
	{
		printf("Get '%s'\n", printOutput.c_str());
		TEST_FAILED;
	}

	// Passing a handle to a function
	printOutput = "";
	ExecuteString(engine, "string a; set(@a); print(a);");
	if( printOutput != "Handle to a string" ) TEST_FAILED;

	// Implicit conversion to handle
	printOutput = "";
	ExecuteString(engine, "string a; set(a); print(a);");
	if( printOutput != "Handle to a string" ) TEST_FAILED;

	// Passing a reference to a handle to the function
	printOutput = "";
	ExecuteString(engine, "string a; set2(@a); print(a);");
	if( printOutput != "Handle to a string" ) TEST_FAILED;

	// Implicit conversion to reference to a handle
	printOutput = "";
	ExecuteString(engine, "string a; set2(a); print(a);");
	if( printOutput != "Handle to a string" ) TEST_FAILED;

    printOutput = "";
    ExecuteString(engine, "string a = \" \"; a[0] = 65; print(a);");
    if( printOutput != "A" ) TEST_FAILED;

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("TestScriptString", script3, strlen(script3), 0);
	if( mod->Build() < 0 )
		TEST_FAILED;

	printOutput = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("TestScriptString", script4, strlen(script4), 0);
	if( mod->Build() < 0 )
		TEST_FAILED;
	ExecuteString(engine, "test()", mod);
	if( printOutput != "Heredoc\\x20test!" ) TEST_FAILED;

	CScriptString *a = new CScriptString("a");
	engine->RegisterGlobalProperty("string a", a);
	r = ExecuteString(engine, "print(a == 'a' ? 't' : 'f')");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", "TestScriptString");
	}
	a->Release();

	// test new
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("TestScriptString", script5, strlen(script5), 0);
	if( mod->Build() < 0 ) TEST_FAILED;
	r = ExecuteString(engine, "Main()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("TestScriptString", script6, strlen(script6), 0);
	if( mod->Build() < 0 ) TEST_FAILED;
	r = ExecuteString(engine, "Main()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;


	// Test character literals
	r = engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true); assert( r >= 0 );
	printOutput = "";
	r = ExecuteString(engine, "print(\"\" + 'a')");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	if( printOutput != "97" ) TEST_FAILED;

	printOutput = "";
	r = ExecuteString(engine, "print(\"\" + '\\'')");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	if( printOutput != "39" ) TEST_FAILED;

	printOutput = "";
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0); // ASCII
	r = ExecuteString(engine, "print(\"\" + '\xFF')");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "print(\"\" + '')");
	if( r != -1 ) TEST_FAILED;
	r = engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, false); assert( r >= 0 );

	// Test special characters (>127) in non unicode scripts
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0); // ASCII
	r = ExecuteString(engine, "string s = '\xC8'; \n assert(s.length() == 1); \n assert(s[0] == 200);");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 1); // UTF8

	//-------------------------------------
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script7, strlen(script7), 0);
	mod->Build();
	r = ExecuteString(engine, "test()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	engine->RegisterObjectType("Http", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectMethod("Http","bool get(const string &in,string &out)", asFUNCTION(Get),asCALL_GENERIC);

	r = ExecuteString(engine, "Http h; string str; h.get('stringtest', str); assert(str == 'output');");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	r = ExecuteString(engine, "Http h; string a = 'test', b; h.get('string'+a, b); assert(b == 'output');");
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	// Test the string utils
	ExecuteString(engine, "string str = 'abcdef'; assert(findFirst(str, 'def') == 3);");
	ExecuteString(engine, "string str = 'abcdef'; assert(findFirstOf(str, 'feb') == 1);");
	ExecuteString(engine, "string str = 'a|b||d'; array<string@>@ arr = split(str, '|'); assert(arr.length() == 4); assert(arr[1] == 'b');");
	ExecuteString(engine, "array<string@> arr = {'a', 'b', '', 'd'}; assert(join(arr, '|') == 'a|b||d');");

	engine->Release();

	//---------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script8, strlen(script8), 0);
	mod->Build();
	r = ExecuteString(engine, "test()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	engine->Release();

	//---------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	engine->RegisterGlobalFunction("void TestFunc(int, string&)", asFUNCTION(TestFunc), asCALL_GENERIC);

	// CHKREF was placed incorrectly
	r = ExecuteString(engine, "TestFunc(0, 'test');");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	r = ExecuteString(engine, "string @s; TestFunc(0, s);");
	if( r != asEXECUTION_EXCEPTION )
		TEST_FAILED;

	engine->Release();

	//----------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script7, strlen(script7), 0);
	mod->Build();
	r = ExecuteString(engine, "test()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;

	engine->Release();

	//------------------------------------------
	// Test the comparison method
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterStdString(engine);

		std::string a = "a";
		std::string b = "b";

		int type = engine->GetTypeIdByDecl("string");
		int c;
		r = CompareRelation(engine, &a, &b, type, c); assert( r >= 0 );
		if( c >= 0 ) TEST_FAILED;
		bool br;
		r = CompareEquality(engine, &a, &b, type, br); assert( r >= 0 );
		if( br ) TEST_FAILED;

		engine->Release();
	}

	//-----
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->RegisterGlobalFunction("void Print(string &str)",asFUNCTION(PrintRef), asCALL_GENERIC);

		const char *script =
			"string str = 'Some String'; \n"
			"void Update() \n"
			"{ \n"
			" Print(str); \n"
			"} \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script, strlen(script));
		mod->Build();

		CScriptString *str = (CScriptString*)mod->GetAddressOfGlobalVar(0);
		UNUSED_VAR(str);

		r = ExecuteString(engine, "Update()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	//-------
	// Multiline strings
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
		engine->RegisterGlobalFunction("void assert(bool)",asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"string str1 = '1\\n' '2'; \n"
			"string str2 = '1\n2';     \n"
			"assert(str1 == str2);     \n";

		r = ExecuteString(engine, script);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	//--------------
	// Empty heredoc string
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);

		const char *script = 
			"void func() { \n"
			" string @tutPage = string(\"\"\" \n"
	        "    \"\"\"); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	//--------------
	// Unicode strings
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, "assert( '\\u0000'.length() == 1 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( '\\U00000000'.length() == 1 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( '\\uFFFF'.length() == 3 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( '\\U0010FFFF'.length() == 4 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// Test compiler warnings
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		// Invalid value
		bout.buffer = "";
		r = ExecuteString(engine, "assert( '\\uD800'.length() == 0 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 9) : Warning : Invalid unicode code point\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Invalid value
		bout.buffer = "";
		r = ExecuteString(engine, "assert( '\\U00FFFFFF'.length() == 0 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 9) : Warning : Invalid unicode code point\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Invalid format
		bout.buffer = "";
		r = ExecuteString(engine, "assert( '\\u001'.length() == 0 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 9) : Warning : Invalid unicode escape sequence, expected 4 hex digits\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Invalid format
		bout.buffer = "";
		r = ExecuteString(engine, "assert( '\\U00001'.length() == 0 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 9) : Warning : Invalid unicode escape sequence, expected 8 hex digits\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// We don't expect any messages
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		// unicode escape sequence in character literals can generate unsigned integers larger than 255
		r = engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true); assert( r >= 0 );
		r = ExecuteString(engine, "assert( '\\uFFFF' == 65535 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( '\\U0010FFFF' == 0x10FFFF )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// A unicode character in a character literal should be properly decoded by the compiler
		char scriptUnicode[] = "assert( '   ' == 0xFFFF )";
		scriptUnicode[ 9] = (char)0xEF;
		scriptUnicode[10] = (char)0xBF;
		scriptUnicode[11] = (char)0xBF;
		r = ExecuteString(engine, scriptUnicode);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// When scanning script as ASCII, only the first byte will count
		engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0); // ASCII
		char scriptUnicode2[] = "assert( '   ' == 0xEF )";
		scriptUnicode2[ 9] = (char)0xEF;
		scriptUnicode2[10] = (char)0xBF;
		scriptUnicode2[11] = (char)0xBF;
		r = ExecuteString(engine, scriptUnicode2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
			
		// \xFF shall produce the actual value, even if it may not be a correctly encoded Unicode character
		engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 1); // UTF8
		engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, false); 
		r = ExecuteString(engine, "assert('\\xFF'[0] == 255)");
		if( r != asEXECUTION_FINISHED ) TEST_FAILED;

		engine->Release();
	}

	return fail;
}

bool Test2()
{
	bool fail = false;

	int r;
	COutStream out;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	const char *string =
		"class Jerome  \n"
		"{  \n"
		"  string a;  \n"
		"  string b;  \n"
		"  double c;  \n"
		"  Jerome(string A,string B,double C)  \n"
		"  {  \n"
		"    a = A;  \n"
		"    b = B;  \n"
		"    c = C;  \n"
		"    assert( a == 'Hello' ); \n"
		"    assert( b == 'Hi' ); \n"
		"    assert( c == 1.23456 ); \n"
		"  }  \n"
		"} \n"
		"Jerome cc('Hello','Hi',1.23456);  \n";
	asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", string);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

//========================================================================

using namespace std;

vector<asWORD> StringFactoryUTF16(unsigned int byteLength, const asWORD *data)
{
	return vector<asWORD>(data, data+byteLength/2);
}

void StringConstructUTF16(vector<asWORD> *o)
{
	new(o) vector<asWORD>();
}

void StringDestructUTF16(vector<asWORD> *o)
{
#if !defined(__BORLANDC__) || __BORLANDC__ >= 0x590
	// Some weird BCC bug (which was fixed in C++Builder 2007) prevents us from calling a
	// destructor explicitly on template functions.
	o->~vector();
#endif
}

bool TestUTF16()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		return false;

	bool fail = false;
	CBufferedOutStream bout;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// Set the string encoding to UTF16 (default is UTF8)
	engine->SetEngineProperty(asEP_STRING_ENCODING, 1);

	// Register our UTF16 string type
	engine->RegisterObjectType("string", sizeof(std::vector<asWORD>), asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
	engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(StringConstructUTF16), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(StringDestructUTF16), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(vector<asWORD>, operator=, (const vector<asWORD> &), vector<asWORD> &), asCALL_THISCALL);

	engine->RegisterStringFactory("string", asFUNCTION(StringFactoryUTF16), asCALL_CDECL);

	vector<asWORD> str;
	engine->RegisterGlobalProperty("string s", &str);

	// Test a normal ASCII string
	r = ExecuteString(engine, "s = 'hello'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	const unsigned short s[] = {'h','e','l','l','o'};
	if( str.size() != 5 || memcmp(&str[0], s, 10) != 0 )
		TEST_FAILED;

	// Test a string with UTF8 scanning above 127 and below 256
	r = ExecuteString(engine, "s = '\xC2\x80'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != 128 )
		TEST_FAILED;

	// Test a string with escape sequence
	r = ExecuteString(engine, "s = '\\n'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != '\n' )
		TEST_FAILED;

	// Test a string with characters above 65535 (requires surrogate pairs)
	r = ExecuteString(engine, "s = '\\U0010FFFFg'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 3 || str[0] != 0xDBFF || str[1] != 0xDFFF || str[2] != 'g' )
		TEST_FAILED;

	// Test hexadecimal escape sequences
	r = ExecuteString(engine, "s = '\\xFF'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != 0xFF )
		TEST_FAILED;

	r = ExecuteString(engine, "s = '\\xFFFg'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 2 || str[0] != 0xFFF || str[1] != 'g' )
		TEST_FAILED;

	// Test a string with ASCII scanning above 127
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0); // ASCII
	r = ExecuteString(engine, "s = '\xFF'");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != 0xFF )
		TEST_FAILED;
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 1); // UTF8

	// Test incomplete UTF8 encoded chars
	r = ExecuteString(engine, "s = '\xFF'"); 
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != 0xFF )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 5) : Warning : Invalid unicode sequence in source\n" )
		printf("%s", bout.buffer.c_str());

	// Test heredoc strings
	r = ExecuteString(engine, "s = \"\"\"\xC2\x80\"\"\"");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( str.size() != 1 || str[0] != 128 )
		TEST_FAILED;

	// Test character literals (they shouldn't be affected by the string encoding)
	engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true); 
	r = ExecuteString(engine, "assert('\xC2\x80' == 0x80)");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();

	return fail;
}


} // namespace

