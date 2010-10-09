#include "utils.h"
using namespace std;

namespace TestDestructor
{

const char *script1 =
"class T                        \n"
"{                              \n"
"   ~T() {Print(\"destruct\");} \n"
"}                              \n"
"T glob;                        \n"
"void Test()                    \n"
"{                              \n"
"  T local;                     \n"
"}                              \n";

const char *script2 =
"class T                        \n"
"{                              \n"
"   ~T() {Print(\"array\");}    \n"
"}                              \n"
"void Test()                    \n"
"{                              \n"
"  T[] a;                       \n"
"  a.resize(1);                 \n"
"  T@[] b;                      \n"
"  b.resize(1);                 \n"
"  @b[0] = @T();                \n"
"}                              \n";

const char *script3 =
"class T                        \n"
"{                              \n"
"   ~T() {Print(\"garbage\");}  \n"
"   T @m;                       \n"
"}                              \n"
"void Test()                    \n"
"{                              \n"
"   T a;                        \n"
"}                              \n";

const char *script4 =
"class T                        \n"
"{                              \n"
"   ~T() {Print(\"once\");@g = @this;}  \n"
"}                              \n"
"T @g;                          \n"
"void Test()                    \n"
"{                              \n"
"   T a;                        \n"
"}                              \n";

const char *script5 =
"class T                        \n"
"{                              \n"
"   ~T() {Print(\"member\");}   \n"
"}                              \n"
"class M                        \n"
"{                              \n"
"   T m;                        \n"
"}                              \n"
"void Test()                    \n"
"{                              \n"
"   M a;                        \n"
"}                              \n";

int count = 0;

void Print(asIScriptGeneric *gen)
{
	std::string *str = (std::string*)gen->GetArgAddress(0);
	UNUSED_VAR(str);
//	printf("%s\n", str->c_str());
	count++;
}

bool Test()
{
	bool fail = false;
	COutStream out;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, true);
	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print), asCALL_GENERIC);

	// Test destructor for script class as local variable and global variable
	count = 0;
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
		fail = true;
	if( count != 1 )
		fail = true;
	engine->DiscardModule(0);
	if( count != 2 )
		fail = true;

	// Test destructor for script class in array
	count = 0;
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
		fail = true;
	engine->GarbageCollect();
	if( count != 2 )
		fail = true;

	// Test destructor for script class cleaned up by garbage collector
	count = 0;
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script3, strlen(script3));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
		fail = true;
	engine->GarbageCollect();
	if( count != 1 )
		fail = true;

	// Make sure destructor is only called once in the life time of the object, even if resurrected
	count = 0;
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
		fail = true;
	if( count != 1 )
		fail = true;
	engine->DiscardModule(0);
	if( count != 1 )
		fail = true;

	// Destructor for member
	count = 0;
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script5, strlen(script5));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
		fail = true;
	if( count != 1 )
		fail = true;

	engine->Release();

	return fail;
}


}
