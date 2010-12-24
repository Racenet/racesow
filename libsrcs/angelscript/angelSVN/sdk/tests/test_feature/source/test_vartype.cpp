#include "utils.h"

namespace TestVarType
{

#define TESTNAME "TestVarType"


// AngelScript syntax: void testFuncI(?& in)
// C++ syntax: void testFuncI(void *ref, int typeId)
// void testFuncI(void *ref, int typeId)
void testFuncI(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	if( typeId == gen->GetEngine()->GetTypeIdByDecl("int") )
		assert(*(int*)ref == 42);
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl("string") )
		assert((*(CScriptString**)ref)->buffer == "test");
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl("string@") )
		assert((*(CScriptString**)ref)->buffer == "test");
	else
		assert(false);
}

// AngelScript syntax: void testFuncO(?& in)
// C++ syntax: void testFuncO(void *ref, int typeId)
// void testFuncO(void *ref, int typeId)
void testFuncO(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	if( typeId == gen->GetEngine()->GetTypeIdByDecl("int") )
		*(int*)ref = 42;
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl("string") )
		(*(CScriptString**)ref)->buffer = "test";
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl("string@") )
		*(CScriptString**)ref = new CScriptString("test");
	else
		assert(false);
}

// AngelScript syntax: void testFuncIS(?& in, const string &in)
// C++ syntax: void testFuncIS(void *ref, int typeId, CScriptString &)
void testFuncIS(void *ref, int typeId, CScriptString &str)
{
	assert(str.buffer == "test");

	// Primitives are received as a pointer to the value
	// Handles are received as a pointer to the handle
	// Objects are received as a pointer to a pointer to the object 
	assert(*(int*)ref == 42 || **(std::string**)ref == "t");
}

void testFuncIS_generic(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	CScriptString *str = *(CScriptString**)gen->GetAddressOfArg(1);

	testFuncIS(ref,typeId,*str);
}

// AngelScript syntax: void testFuncSI(const string &in, ?& in)
// C++ syntax: void testFuncSI(CScriptString &, void *ref, int typeId)
void testFuncSI(CScriptString &str, void *ref, int typeId)
{
	assert(str.buffer == "test");
	assert(*(int*)ref == 42);
}

void testFuncSI_generic(asIScriptGeneric *gen)
{
	CScriptString *str = *(CScriptString**)gen->GetAddressOfArg(0);
	void *ref = gen->GetArgAddress(1);
	int typeId = gen->GetArgTypeId(1);

	testFuncSI(*str, ref, typeId);
}


bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
 	asIScriptEngine *engine = 0;
	asIScriptModule *mod = 0;

	// It must not be possible to declare global variables of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script1 = "? globvar;";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Error   : Unexpected token '?'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare local variables of the var type ?
	const char *script2 = "void func() {? localvar;}";
	mod->AddScriptSection("script", script2);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
                       "script (1, 14) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to register global properties of the var type ?
	r = engine->RegisterGlobalProperty("? prop", 0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "Property (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must not be possible to register object members of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("test", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectProperty("test", "? prop", 0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "Property (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must not be possible to declare script class members of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script3 = "class c {? member;}";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script3);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 10) : Error   : Expected method or property\n"
		               "script (1, 19) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";
	
	// It must not be possible to declare script functions that take the var type ? as parameter 
	const char *script4 = "void func(?&in a) {}";
	mod->AddScriptSection("script", script4);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 11) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script functions that return the var type ?
	const char *script5 = "? func() {}";
	mod->AddScriptSection("script", script5);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Error   : Unexpected token '?'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script class methods that take the var type ? as parameter
	const char *script6 = "class c {void method(?& in a) {}}";
	mod->AddScriptSection("script", script6);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 22) : Error   : Expected data type\n" 
		               "script (1, 22) : Error   : Expected method or property\n" 
					   "script (1, 33) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script class methods that return the var type ?
	const char *script7 = "class c {? method() {}}";
	mod->AddScriptSection("script", script7);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 10) : Error   : Expected method or property\n"
		               "script (1, 23) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare arrays of the var type ?
	const char *script8 = "void func() { ?[] array; }";
	mod->AddScriptSection("script", script8);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 15) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare handles of the var type ?
	const char *script9 = "void func() { ?@ handle; }";
	mod->AddScriptSection("script", script9);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 15) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to register functions that return the var type ?
	r = engine->RegisterGlobalFunction("? testFunc()", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "System function (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must be possible to register functions that take the var type ? as parameter
	// Only when the expression is explicitly sent as @ should the type id be @
	// const ? & in
	// ? & in
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	r = engine->RegisterGlobalFunction("void testFuncI(?& in)", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r < 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncCI(const?&in)", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a = 42; testFuncI(a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a = \"test\"; testFuncI(a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string @a = @\"test\"; testFuncI(@a);");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// It must be possible to register with 'out' references
	// ? & out
	r = engine->RegisterGlobalFunction("void testFuncO(?&out)", asFUNCTION(testFuncO), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a; testFuncO(a); assert(a == 42);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a; testFuncO(a); assert(a == \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string @a; testFuncO(@a); assert(a == \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// It must be possible to mix normal parameter types with the var type ?
	// e.g. func(const string &in, const ?& in), or func(const ?& in, const string &in)
	r = engine->RegisterGlobalFunction("void testFuncIS(?& in, const string &in)", asFUNCTION(testFuncIS_generic), asCALL_GENERIC);
	if( r < 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncSI(const string &in, ?& in)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a = 42; testFuncIS(a, \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "int a = 42; testFuncSI(\"test\", a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a = \"t\"; testFuncIS(a, \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a = \"t\"; testFuncIS(@a, \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;


	// It must be possible to use native functions
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		r = engine->RegisterGlobalFunction("void _testFuncIS(?& in, const string &in)", asFUNCTION(testFuncIS), asCALL_CDECL);
		if( r < 0 ) fail = true;
		r = engine->RegisterGlobalFunction("void _testFuncSI(const string &in, ?& in)", asFUNCTION(testFuncSI), asCALL_CDECL);
		if( r < 0 ) fail = true;

		r = engine->ExecuteString(0, "int a = 42; _testFuncIS(a, \"test\");");
		if( r != asEXECUTION_FINISHED ) fail = true;
		r = engine->ExecuteString(0, "int a = 42; _testFuncSI(\"test\", a);");
		if( r != asEXECUTION_FINISHED ) fail = true;
		r = engine->ExecuteString(0, "string a = \"t\"; _testFuncIS(a, \"test\");");
		if( r != asEXECUTION_FINISHED ) fail = true;
		r = engine->ExecuteString(0, "string a = \"t\"; _testFuncIS(@a, \"test\");");
		if( r != asEXECUTION_FINISHED ) fail = true;
	}

	// Don't give error on passing reference to const to ?&out
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script = 
	"class C { string @a; } \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	mod->Build();
	r = engine->ExecuteString(0, "const C c; testFuncO(@c.a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( bout.buffer != "ExecuteString (1, 22) : Warning : Argument cannot be assigned. Output will be discarded.\n" ) fail = true;
	bout.buffer = "";

	// Don't allow ?& with operators yet
	engine->RegisterObjectType("type", sizeof(int), asOBJ_VALUE | asOBJ_APP_PRIMITIVE);
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_ASSIGNMENT, "type &f(const ?& in)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "type f(const ?& in, const ?& in)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;
	
	// Don't allow use of ? without being reference
	r = engine->RegisterGlobalFunction("void testFunc_err(const ?)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;

	// Don't allow use of 'inout' reference, yet
	// ? & [inout]
	// const ? & [inout]
	r = engine->RegisterGlobalFunction("void testFuncIO(?&)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r >= 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncCIO(const?&)", asFUNCTION(testFuncSI_generic), asCALL_GENERIC);
	if( r >= 0 ) fail = true;

	engine->Release();

	return fail;
}

} // namespace

