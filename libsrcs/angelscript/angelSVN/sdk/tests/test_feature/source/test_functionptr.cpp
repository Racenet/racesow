#include "utils.h"

namespace TestFunctionPtr
{

bool receivedFuncPtrIsOK = false;

void ReceiveFuncPtr(asIScriptFunction *funcPtr)
{
	if( funcPtr == 0 ) return;

	if( strcmp(funcPtr->GetName(), "test") == 0 ) 
		receivedFuncPtrIsOK = true;

	funcPtr->Release();
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	bool fail = false;
	int r;
	COutStream out;
	asIScriptEngine *engine;
	asIScriptModule *mod;
	CBufferedOutStream bout;
	const char *script;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Test the declaration of new function signatures
	script = "funcdef void functype();\n"
	// It must be possible to declare variables of the funcdef type
		     "functype @myFunc = null;\n"
	// It must be possible to initialize the function pointer directly
			 "functype @myFunc1 = @func;\n"
		 	 "void func() { called = true; }\n"
			 "bool called = false;\n"
	// It must be possible to compare the function pointer with another
	         "void main() { \n"
			 "  assert( myFunc1 !is null ); \n"
			 "  assert( myFunc1 is func ); \n"
	// It must be possible to call a function through the function pointer
	    	 "  myFunc1(); \n"
			 "  assert( called ); \n"
	// Local function pointer variables are also possible
			 "  functype @myFunc2 = @func;\n"
			 "} \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r != 0 )
		TEST_FAILED;
	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// It must be possible to save the byte code with function handles
	CBytecodeStream bytecode(__FILE__"1");
	mod->SaveByteCode(&bytecode);
	{
		asIScriptModule *mod2 = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		mod2->LoadByteCode(&bytecode);
		r = ExecuteString(engine, "main()", mod2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	}

	// Test function pointers as members of classes. It should be possible to call the function
	// from within a class method. It should also be possible to call it from outside through the . operator.
	script = "funcdef void FUNC();       \n"
		     "class CMyObj               \n"
			 "{                          \n"
			 "  CMyObj() { @f = @func; } \n"
			 "  FUNC@ f;                 \n"
			 "  void test()              \n"
			 "  {                        \n"
			 "    this.f();              \n"
			 "    f();                   \n"
			 "    CMyObj o;              \n"
			 "    o.f();                 \n"
			 "    main();                \n"
			 "    assert( called == 4 ); \n"
			 "  }                        \n"
			 "}                          \n"
			 "void main()                \n"
			 "{                          \n"
			 "  CMyObj o;                \n"
			 "  o.f();                   \n"
			 "}                          \n"
			 "int called = 0;            \n"
			 "void func() { called++; }  \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "CMyObj o; o.test();", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
	}
	ctx->Release();

	// It must not be possible to declare a non-handle variable of the funcdef type
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	script = "funcdef void functype();\n"
		     "functype myFunc;\n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (2, 1) : Error   : Data type can't be 'functype'\n"
					   "script (2, 10) : Error   : No default constructor for object of type 'functype'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// It must not be possible to invoke the funcdef
	bout.buffer = "";
	script = "funcdef void functype();\n"
		     "void func() { functype(); } \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (2, 1) : Info    : Compiling void func()\n"
					   "script (2, 15) : Error   : No matching signatures to 'functype()'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test that a funcdef can't have the same name as other global entities
	bout.buffer = "";
	script = "funcdef void test();  \n"
	         "int test; \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (2, 5) : Error   : Name conflict. 'test' is a funcdef.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Must not be possible to take the address of class methods
	bout.buffer = "";
	script = "class t { \n"
		"  void func() { @func; } \n"
		"} \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (2, 3) : Info    : Compiling void t::func()\n"
	                   "script (2, 18) : Error   : 'func' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// A more complex sample
	bout.buffer = "";
	script = 
		"funcdef bool CALLBACK(int, int); \n"
		"funcdef bool CALLBACK2(CALLBACK @); \n"
		"void main() \n"
		"{ \n"
		"	CALLBACK @func = @myCompare; \n"
		"	CALLBACK2 @func2 = @test; \n"
		"	func2(func); \n"
		"} \n"
		"bool test(CALLBACK @func) \n"
		"{ \n"
		"	return func(1, 2); \n"
		"} \n"
		"bool myCompare(int a, int b) \n"
		"{ \n"
		"	return a > b; \n"
		"} \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}
	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// It must be possible to register the function signature from the application
	r = engine->RegisterFuncdef("void AppCallback()");
	if( r < 0 )
		TEST_FAILED;

	r = engine->RegisterGlobalFunction("void ReceiveFuncPtr(AppCallback @)", asFUNCTION(ReceiveFuncPtr), asCALL_CDECL); assert( r >= 0 );

	// It must be possible to use the registered funcdef
	// It must be possible to receive a function pointer for a registered func def
	bout.buffer = "";
	script = 
		"void main() \n"
		"{ \n"
		"	AppCallback @func = @test; \n"
		"   func(); \n"
		"   ReceiveFuncPtr(func); \n"
		"} \n"
		"void test() \n"
		"{ \n"
		"} \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	if( !receivedFuncPtrIsOK )
		TEST_FAILED;

	mod->SaveByteCode(&bytecode);
	{
		receivedFuncPtrIsOK = false;
		asIScriptModule *mod2 = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		mod2->LoadByteCode(&bytecode);
		r = ExecuteString(engine, "main()", mod2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( !receivedFuncPtrIsOK )
			TEST_FAILED;
	}

	//----------------------------------------------------------
	// TODO: Future improvements below

	// If the function referred to when taking a function pointer is removed from the module,
	// the code must not be invalidated. After removing func() from the module, it must still 
	// be possible to execute func2()
	script = "funcdef void FUNC(); \n"
	         "void func() {} \n";
	         "void func2() { FUNC@ f = @func; f(); } \n";

	// Test that the function in a function pointer isn't released while the function 
	// is being executed, even though the function pointer itself is cleared
	script = "DYNFUNC@ funcPtr;        \n"
		     "funcdef void DYNFUNC(); \n"
			 "@funcPtr = @CompileDynFunc('void func() { @funcPtr = null; }'); \n";

	// The compiler should be able to determine the right function overload by the destination of the function pointer
	script = "funcdef void f(); \n"
		     "f @fp = @func();  \n"
			 "void func() {}    \n"
			 "void func(int) {} \n";

	// Test that it is possible to declare the function signatures out of order
	// This also tests the circular reference between the function signatures
	script = "funcdef void f1(f2@) \n"
	         "funcdef void f2(f1@) \n";

	// It must be possible to identify a function handle type from the type id

	// It must be possible enumerate the function definitions in the module, 
	// and to enumerate the parameters the function accepts

	// A funcdef defined in multiple modules must share the id and signature so that a function implemented 
	// in one module can be called from another module by storing the handle in the funcdef variable

	// An interface that takes a funcdef as parameter must still have its typeid shared if the funcdef can also be shared
	// If the funcdef takes an interface as parameter, it must still be shared

	// Must have a generic function pointer that can store any signature. The function pointer
	// can then be dynamically cast to the correct function signature so that the function it points
	// to can be invoked.

	engine->Release();

	// Test clean up with registered function definitions
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptString(engine);
		r = engine->RegisterFuncdef("void MSG_NOTIFY_CB(const string& strCommand, const string& strTarget)"); assert(r>=0);

		engine->Release();
	}

	// Test registering function pointer as property
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterFuncdef("void fptr()");
		r = engine->RegisterGlobalProperty("fptr f", 0);
		if( r >= 0 ) TEST_FAILED;
		engine->RegisterObjectType("obj", 0, asOBJ_REF);
		r = engine->RegisterObjectProperty("obj", "fptr f", 0);
		if( r >= 0 ) TEST_FAILED;

		engine->Release();
	}

	// Test passing handle to function pointer
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		mod->AddScriptSection("script",
			"class CTempObj             \n"
			"{                          \n"
			"  int Temp;                \n"
			"}                          \n"
			"funcdef void FUNC2(CTempObj@);\n"
			"class CMyObj               \n"
			"{                          \n"
			"  CMyObj() { @f2= @func2; }\n"
			"  FUNC2@ f2;               \n"
			"}                          \n"
			"void main()                \n"
			"{                          \n"
			"  CMyObj o;                \n"
			"  CTempObj t;              \n"
			"  o.f2(t);                 \n"
			"  assert( called == 1 );   \n"
			"}                          \n"
			"int called = 0;            \n"
			"void func2(CTempObj@ Obj)  \n"
			"{ called++; }              \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test out of order declaration with function pointers
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		mod->AddScriptSection("script",
			"funcdef void FUNC2(CTempObj@);\n"
			"class CTempObj {}             \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Success
 	return fail;
}

} // namespace

