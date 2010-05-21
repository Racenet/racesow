#include "utils.h"

namespace TestCompiler
{

#define TESTNAME "TestCompiler"

// Unregistered types and functions
const char *script1 =
"void testFunction ()                          \n"
"{                                             \n"
" Assert@ assertReached = tryToAvoidMeLeak();  \n"
"}                                             \n";

const char *script2 =
"void CompilerAssert()\n"
"{\n"
"   bool x = 0x0000000000000000;\n"
"   bool y = 1;\n"
"   x+y;\n"
"}";

const char *script3 = "void CompilerAssert(uint8[]@ &in b) { b[0] == 1; }";

const char *script4 = "class C : I {}";

const char *script5 =
"void t() {} \n"
"void crash() { bool b = t(); } \n";

const char *script6 = "class t { bool Test(bool, float) {return false;} }";

const char *script7 =
"class Ship                           \n\
{                                     \n\
	Sprite		_sprite;              \n\
									  \n\
	string GetName() {                \n\
		return _sprite.GetName();     \n\
	}								  \n\
}";

const char *script8 =
"float calc(float x, float y) { Print(\"GOT THESE NUMBERS: \" + x + \", \" + y + \"\n\"); return x*y; }";


const char *script9 =
"void noop() {}\n"
"int fuzzy() {\n"
"  return noop();\n"
"}\n";

const char *script10 =
"void func() {}\n"
"void test() { int v; v = func(); }\n";

const char *script11 =
"class c                                       \n"
"{                                             \n"
"  object @obj;                                \n"
"  void func()                                 \n"
"  {type str = obj.GetTypeHandle();}           \n"
"}                                             \n";

const char *script12 =
"void f()       \n"
"{}             \n"
"               \n"
"void assert()  \n"
"{              \n"
"   2<3?f():1;  \n"
"}              \n";

bool Test2();
bool Test3();
bool Test4();
bool Test5();
bool Test6();
bool Test7();
bool Test8();

bool Test()
{
	bool fail = false;
	int r;

	fail = Test2() || fail;
	fail = Test3() || fail;
	fail = Test4() || fail;
	fail = Test5() || fail;
	fail = Test6() || fail;
	fail = Test7() || fail;
	fail = Test8() || fail;

	asIScriptEngine *engine;
	CBufferedOutStream bout;
	COutStream out;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void testFunction()\n"
                       "TestCompiler (3, 8) : Error   : Expected ';'\n" )
		fail = true;

	engine->Release();

	// test 2
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	r = mod->Build();
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void CompilerAssert()\n"
					   "TestCompiler (3, 13) : Error   : Can't implicitly convert from 'uint' to 'bool'.\n"
					   "TestCompiler (4, 13) : Error   : Can't implicitly convert from 'uint' to 'bool'.\n"
					   "TestCompiler (5, 5) : Error   : No conversion from 'bool' to math type available.\n" )
	   fail = true;

	// test 3
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	r = mod->Build();
	if( r < 0 )
		fail = true;

	// test 4
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	r = mod->Build();
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 11) : Error   : Identifier 'I' is not a data type\n" )
		fail = true;

	// test 5
	RegisterScriptString(engine);
	bout.buffer = "";
	r = engine->ExecuteString(0, "string &ref");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 8) : Error   : Expected '('\n" )
		fail = true;

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestCompiler (2, 1) : Info    : Compiling void crash()\n"
	                   "TestCompiler (2, 25) : Error   : Can't implicitly convert from 'void' to 'bool'.\n" )
		fail = true;

	// test 6
	// Verify that script class methods can have the same signature as
	// globally registered functions since they are in different scope
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("bool Test(bool, float)", asFUNCTION(0), asCALL_GENERIC);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	r = mod->Build();
	if( r < 0 )
	{
		printf("failed on 6\n");
		fail = true;
	}

	// test 7
	// Verify that declaring a void variable in script causes a compiler error, not an assert failure
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	engine->ExecuteString(0, "void m;");
	if( bout.buffer != "ExecuteString (1, 6) : Error   : Data type can't be 'void'\n" )
	{
		printf("failed on 7\n");
		fail = true;
	}

	// test 8
	// Don't assert on implicit conversion to object when a compile error has occurred
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script7, strlen(script7));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (3, 2) : Error   : Identifier 'Sprite' is not a data type\n"
					   "script (5, 2) : Info    : Compiling string Ship::GetName()\n"
					   "script (6, 17) : Error   : Illegal operation on 'int&'\n" )
	{
		fail = true;
	}

	// test 9
	// Don't hang on script with non-terminated string
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script8, strlen(script8));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (1, 1) : Info    : Compiling float calc(float, float)\n"
	                   "script (1, 77) : Error   : Multiline strings are not allowed in this application\n"
	                   "script (1, 32) : Error   : No matching signatures to 'Print(string@&)'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// test 10
	// Properly handle error with returning a void expression
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script9, strlen(script9));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling int fuzzy()\n"
		               "script (3, 3) : Error   : No conversion from 'void' to 'int' available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// test 11
	// Properly handle error when assigning a void expression to a variable
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script10, strlen(script10));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling void test()\n"
		               "script (2, 26) : Error   : Can't implicitly convert from 'void' to 'int'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 12
	// Handle errors after use of undefined objects
	bout.buffer = "";
	engine->RegisterObjectType("type", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script11, strlen(script11));
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (3, 3) : Error   : Identifier 'object' is not a data type\n"
					   "script (3, 10) : Error   : Object handle is not supported for this type\n"
                       "script (4, 3) : Info    : Compiling void c::func()\n"
                       "script (5, 18) : Error   : Illegal operation on 'int&'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 13
	// Don't permit implicit conversion of integer to obj even though obj(int) is a possible constructor
	bout.buffer = "";
	r = engine->ExecuteString(0, "uint32[] a = 0;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : Can't implicitly convert from 'const uint' to 'uint[]&'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 14
	// Calling void function in ternary operator ?:
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script12, strlen(script12));
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (4, 1) : Info    : Compiling void assert()\n"
                       "script (6, 4) : Error   : Both expressions must have the same type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 15
	// Declaring a class inside a function
	bout.buffer = "";
	r = engine->ExecuteString(0, "class XXX { int a; }; XXX b;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : Expected expression value\n"
	                   "ExecuteString (1, 27) : Error   : Expected ';'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 16
	// Compiler should warn if uninitialized variable is used to index an array
	bout.buffer = "";
	const char *script_16 = "void func() { int[] a(1); int b; a[b] = 0; }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script_16, strlen(script_16));
	r = mod->Build();
	if( r < 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 36) : Warning : 'b' is not initialized.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 17
	// Compiler should warn if uninitialized variable is used with post increment operator
	bout.buffer = "";
	const char *script_17 = "void func() { int a; a++; }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script_17, strlen(script_17));
	r = mod->Build();
	if( r < 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 23) : Warning : 'a' is not initialized.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 18
	// Properly notify the error of comparing boolean operands
	bout.buffer = "";
	r = engine->ExecuteString(0, "bool b1,b2; if( b1 <= b2 ) {}");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 20) : Warning : 'b1' is not initialized.\n"
                       "ExecuteString (1, 20) : Warning : 'b2' is not initialized.\n"
                       "ExecuteString (1, 20) : Error   : Illegal operation on this datatype\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 19 - Give proper error upon returning a reference from script function
	// TODO: This should be allowed when proper support has been added
	bout.buffer = "";
	const char *script19 =
		"class Object {}\n"
		"Object &Test(Object &in object)\n"
		"{\n"
		"  return object;\n"
		"}\n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script19", script19, strlen(script19));
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script19 (2, 1) : Info    : Compiling Object& Test(Object&in)\n"
		               "script19 (2, 1) : Error   : Script functions must not return references\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 20
	// Don't crash on invalid script code
	bout.buffer = "";
	const char *script20 = 
		"class A { A @b; } \n"
		"void test()       \n"
		"{ A a; if( @a.b == a.GetClient() ) {} } \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script20", script20, strlen(script20));
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script20 (2, 1) : Info    : Compiling void test()\n"
	                   "script20 (3, 22) : Error   : No matching signatures to 'A::GetClient()'\n"
	                   "script20 (3, 17) : Warning : The operand is implicitly converted to handle in order to compare them\n"
	                   "script20 (3, 17) : Error   : No conversion from 'const int' to 'A@' available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 21
	// Don't crash on undefined variable
	bout.buffer = "";
	const char *script21 =
		"bool MyCFunction() {return true;} \n"
		"void main() \n"
		"{ \n"
		"	if (true and MyCFunction( SomethingUndefined )) \n"
		"	{ \n"
		"	} \n"
		"} \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script21", script21, strlen(script21));
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script21 (2, 1) : Info    : Compiling void main()\n"
					   "script21 (4, 28) : Error   : 'SomethingUndefined' is not declared\n"
					   "script21 (4, 11) : Error   : No conversion from '<unrecognized token>' to 'bool' available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 22
	bout.buffer = "";
	const char *script22 = 
		"class Some{} \n"
		"void Func(Some@ some) \n"
		"{ \n"
		"if( some is null) return; \n"
		"Func_(null); \n"
		"} \n"
		"void Func_(uint i) \n"
		"{ \n"
		"} \n";

	r = mod->AddScriptSection("22", script22);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "22 (2, 1) : Info    : Compiling void Func(Some@)\n"
	                   "22 (5, 1) : Error   : No matching signatures to 'Func_(<null handle>)'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 23 - don't assert on invalid condition expression
	bout.buffer = "";
	const char *script23 = "openHandle.IsValid() ? 1 : 0\n";

	r = engine->ExecuteString(0, script23);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : 'openHandle' is not declared\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 24 - don't assert on invalid return statement
	bout.buffer = "";
	const char *script24 = "string SomeFunc() { return null; }";
	r = mod->AddScriptSection("24", script24);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "24 (1, 1) : Info    : Compiling string SomeFunc()\n"
		               "24 (1, 28) : Error   : Can't implicitly convert from '<null handle>' to 'string'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 25 - don't permit returning references from script functions
	// TODO: This should fail to compile because the reference is to a temporary variable
	bout.buffer = "";
	const char *script25 = "string &SomeFunc() { return ''; }";
	r = mod->AddScriptSection("25", script25);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "25 (1, 1) : Info    : Compiling string& SomeFunc()\n"
		               "25 (1, 1) : Error   : Script functions must not return references\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Success
 	return fail;
}


//------------------------------------
// Test 2 was reported by Dentoid
float add(float &a, float &b)
{
	return a+b;
}

void doStuff(float a, float b)
{
}

bool Test2()
{
	if( strstr(asGetLibraryOptions(), " AS_MAX_PORTABILITY ") )
		return false;

#if defined(__GNUC__) && defined(__amd64__)
	// TODO: Add this support
	// Passing non-complex objects by value is not yet supported, because 
	// it means moving each property of the object into different registers
	return false;
#endif

	bool fail = false;
	int r;
	COutStream out;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->RegisterObjectType( "Test", sizeof(float), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_FLOAT );
	engine->RegisterGlobalBehaviour( asBEHAVE_ADD, "Test f(Test &in, Test &in)", asFUNCTION(add), asCALL_CDECL);
	engine->RegisterGlobalFunction("void doStuff(Test, Test)", asFUNCTION(doStuff), asCALL_CDECL);

	const char *script =
	"Test test1, test2;                \n"
	"doStuff( test1, test1 + test2 );  \n"  // This one will work
	"doStuff( test1 + test2, test1 );  \n"; // This one will blow

	r = engine->ExecuteString(0, script);
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();
	return fail;
}

//-----------------------------------------
// Test 3 was reported by loboWu
bool Test3()
{
	bool fail = false;
	COutStream out;
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	const char *script =
	"uint8 search_no(uint8[]@ cmd, uint16 len, uint8[] @rcv)    \n" //mbno is nx5 array, static
	"{														    \n"
	"	if (@rcv == null) assert(false);						\n"
	"		return(255);										\n"
	"}															\n"
	"void main()												\n"
	"{															\n"
	"	uint8[] cmd = { 0x02, 0x95, 0x45, 0x42, 0x32 };			\n"
	"	uint8[] rcv;											\n"
	"	uint16 len = 8;											\n"
	"	search_no(cmd, cmd.length(), rcv);						\n" //This is OK! @rcv won't be null
	"	search_no(cmd, GET_LEN2(cmd), rcv);						\n" //This is OK!
	"	len = GET_LEN(cmd);										\n"
	"	search_no(cmd, len, rcv);								\n" //This is OK!
	"															\n"//but
	"	search_no(cmd, GET_LEN(cmd), rcv);						\n" //@rcv is null
	"}															\n"
	"uint16 GET_LEN(uint8[]@ cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n"
	"uint16 GET_LEN2(uint8[] cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "main()");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();
	return fail;
}

//----------------------------------------
// Test 4 reported by dxj19831029
bool Test4()
{
	bool fail = false;
	COutStream out;
	int r = 0;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->RegisterObjectType("Chars", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_FACTORY, "Chars@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_ASSIGNMENT, "Chars &f(const Chars &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterObjectType("_Save", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectProperty("_Save", "Chars FieldName", 0);

	engine->RegisterObjectType("Struct", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_FACTORY, "Struct@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectProperty("Struct", "_Save Save", 0);

	engine->RegisterObjectType("ScriptObject", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectMethod("ScriptObject", "Struct @f()", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterGlobalProperty("ScriptObject current", 0);

	engine->RegisterGlobalFunction("void print(Chars&)", asFUNCTION(0), asCALL_GENERIC);

	const char *script1 = "void main() { print(current.f().Save.FieldName); }";
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script1, strlen(script1));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	const char *script2 = "void main() { Chars a = current.f().Save.FieldName; print(a); }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	engine->Release();
	return fail;
}

//-----------------------------------------------
// Test5 reported by jal
bool Test5()
{
	// This script caused an assert failure during compilation
	const char *script =
		"class cFlagBase {} \n"
		"void CTF_getBaseForOwner( )   \n"
		"{  \n"
		"   for ( cFlagBase @flagBase; ; @flagBase = null ) \n"
		"   {  \n"
		"	}  \n"
		"}   ";

	bool fail = false;
	COutStream out;
	int r = 0;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 0);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	engine->Release();
	return fail;
}

//-------------------------------------------------
// Test6 reported by SiCrane
bool Test6()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	// The following script would enter an infinite loop while building
	const char *script1 =
		"class Foo { \n"
		"const int foo(int a) { return a; } \n"
		"} \n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// This should also work
	const char *script2 =
		"interface IFoo { \n"
		"	const int foo(int a); \n"
		"} \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// This would cause an assert failure
	const char *script3 =
		"class MyClass { \n"
		"    MyClass(int a) {} \n"
		"} \n"
		"const MyClass foo(int (a) ,bar); \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script3, strlen(script3));
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (4, 18) : Info    : Compiling const MyClass foo\n"
					   "script (4, 28) : Error   : 'bar' is not declared\n"
					   "script (4, 24) : Error   : 'a' is not declared\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// This would also cause an assert failure
	const char *script4 =
		"void main() { \n"
		"  for (;i < 10;); \n"
		"} \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void main()\n"
					   "script (2, 9) : Error   : 'i' is not declared\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	return fail;
}

//---------------------------------------
// Test7 reported by Vicious
// http://www.gamedev.net/community/forums/topic.asp?topic_id=525467
bool Test7()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		return false;

	bool fail = false;

	const char *script = 
	"void GENERIC_CommandDropItem( cClient @client )	\n"
	"{													\n"
	"	client.getEnt().health -= 1;					\n"
	"}													\n";

	// 1. tmp1 = client.getEnt()
	// 2. tmp2 = tmp1.health
	// 3. tmp3 = tmp2 - 1
	// 4. free tmp2
	// 5. tmp1.health = tmp3
	// 6. free tmp3
	// 7. free tmp1

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);

	engine->RegisterObjectType("cEntity", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("cEntity", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("cEntity", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("cEntity", "cEntity @getEnt()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("cEntity", "int health", 0);
	engine->RegisterObjectType("cClient", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("cClient", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("cClient", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("cClient", "cEntity @getEnt()", asFUNCTION(0), asCALL_CDECL_OBJLAST);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	int r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}

bool Test8()
{
	bool fail = false;

	CBufferedOutStream bout;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	RegisterScriptString(engine);

	// Must allow returning a const string
	const char *script = "const string func() { return ''; }";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	int r = mod->Build();
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	r = engine->ExecuteString(0, "string str = func(); assert( str == '' );");
	if( r != asEXECUTION_FINISHED ) 
		fail = true;

	engine->Release();

	return fail;
}

} // namespace

