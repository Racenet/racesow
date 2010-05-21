#include "utils.h"

namespace TestArrayIntf
{

#define TESTNAME "TestArrayIntf"

// Normal structure
static const char *script1 =
"void Test()                  \n"
"{                            \n"
"   float[] a(2);             \n"
"   a[0] = 1.1f;              \n"
"   a[1] = 1.2f;              \n"
"   @floatArray = a;          \n"
"   string[] b(1);            \n"
"   b[0] = \"test\";          \n"
"   @stringArray = b;         \n"
"}                            \n";


asIScriptArray *floatArray = 0;
asIScriptArray *stringArray = 0;

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString_Generic(engine);

	engine->RegisterGlobalProperty("float[] @floatArray", &floatArray);
	engine->RegisterGlobalProperty("string[] @stringArray", &stringArray);

	COutStream out;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED ) 
		fail = true;
	else
	{
		if( (floatArray->GetArrayTypeId() & asTYPEID_MASK_OBJECT) != asTYPEID_SCRIPTARRAY )
			fail = true;

		if( floatArray->GetArrayTypeId() != engine->GetTypeIdByDecl("float[]") )
			fail = true;

		if( floatArray->GetElementTypeId() != engine->GetTypeIdByDecl("float") )
			fail = true;

		if( floatArray->GetElementCount() != 2 )
			fail = true;

		if( *(float*)floatArray->GetElementPointer(0) != 1.1f )
			fail = true;

		if( *(float*)floatArray->GetElementPointer(1) != 1.2f )
			fail = true;

		if( stringArray->GetArrayTypeId() != engine->GetTypeIdByDecl("string[]") )
			fail = true;

		if( stringArray->GetElementTypeId() != engine->GetTypeIdByDecl("string") )
			fail = true;

		if( stringArray->GetElementCount() != 1 )
			fail = true;

		if( ((CScriptString*)stringArray->GetElementPointer(0))->buffer != "test" )
			fail = true;

		stringArray->Resize(2);
	}

	if( floatArray )
		floatArray->Release();
	if( stringArray )
		stringArray->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

