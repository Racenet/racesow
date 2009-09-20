#include "utils.h"

using namespace std;

namespace TestDict
{

#define TESTNAME "TestDict"



static const char *script1 =
"void TestDict()                   \n"
"{                                 \n"
"   Dict d;                        \n"
"   d[\"test\\n\"];                \n"
"}                                 \n";

class CDict
{
public:
	CDict() {}
	~CDict() {}

	CDict &operator=(const CDict &other) { return *this; }

	CDict &operator[](string s) 
	{ 
//		printf(s.c_str()); 
		return *this;
	}
};

void Construct(CDict *o)
{
	new(o) CDict();
}

void Destruct(CDict *o)
{
	o->~CDict();
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->RegisterObjectType("Dict", sizeof(CDict), asOBJ_VALUE | asOBJ_APP_CLASS_CDA);	
	engine->RegisterObjectBehaviour("Dict", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Dict", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Dict", asBEHAVE_ASSIGNMENT, "Dict &f(const Dict &in)", asMETHOD(CDict,operator=), asCALL_THISCALL);

	engine->RegisterObjectBehaviour("Dict", asBEHAVE_INDEX, "Dict &f(string)", asMETHOD(CDict, operator[]), asCALL_THISCALL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "TestDict()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

