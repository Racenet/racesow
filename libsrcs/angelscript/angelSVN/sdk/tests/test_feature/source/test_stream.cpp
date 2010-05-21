#include "utils.h"
#include <sstream>
using namespace std;

namespace TestStream
{

#define TESTNAME "TestStream"

stringstream stream;

class CScriptStream
{
public:
	CScriptStream();
	~CScriptStream();

	CScriptStream &operator=(const CScriptStream&);
	void AddRef();
	void Release();

	stringstream s;
	int refCount;
};

CScriptStream &operator<<(CScriptStream &s, const string &other)
{
//	printf("(%X) << \"%s\"\n", &s, other.c_str());

	stream << other;
	s.s << other;
	return s;
}

CScriptStream &operator>>(CScriptStream &s, string &other)
{
	s.s >> other;
	return s;
}


CScriptStream::CScriptStream()
{
//	printf("new (%X)\n", this);

	refCount = 1;
}

CScriptStream::~CScriptStream()
{
//	printf("del (%X)\n", this);
}

CScriptStream &CScriptStream::operator=(const CScriptStream & /*other*/)
{
//	printf("(%X) = (%X)\n", this, &other);

	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
		ctx->SetException("Illegal use of assignment on stream object");

	return *this;
}

void CScriptStream::AddRef()
{
	refCount++;
}

void CScriptStream::Release()
{
	if( --refCount == 0 )
		delete this;
}

CScriptStream *CScriptStream_Factory()
{
	return new CScriptStream;
}

static const char *script1 =
"void Test()                       \n"
"{                                 \n"
"  stream s;                       \n"
"  s << \"a\" << \"b\" << \"c\";   \n"
"}                                 \n";

static const char *script2 =
"void Test2()                      \n"
"{                                 \n"
"  stream s;                       \n"
"  s << \"a b c\";                 \n"
"  string a,b,c;                   \n"
"  s >> a >> b >> c;               \n"
"  Assert(a == \"a\");             \n"
"  Assert(b == \"b\");             \n"
"  Assert(c == \"c\");             \n"
"}                                 \n";

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

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->RegisterObjectType("stream", sizeof(CScriptStream), asOBJ_REF);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_FACTORY, "stream@ f()", asFUNCTION(CScriptStream_Factory), asCALL_CDECL);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptStream,AddRef), asCALL_THISCALL);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptStream,Release), asCALL_THISCALL);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_ASSIGNMENT, "stream &f(const stream &in)", asMETHOD(CScriptStream, operator=), asCALL_THISCALL);
	engine->RegisterGlobalBehaviour(asBEHAVE_BIT_SLL, "stream &f(stream &inout, const string &in)", asFUNCTIONPR(operator<<, (CScriptStream &s, const string &other), CScriptStream &), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_BIT_SRL, "stream &f(stream &inout, string &out)", asFUNCTIONPR(operator>>, (CScriptStream &s, string &other), CScriptStream &), asCALL_CDECL);

	COutStream out;
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	if( stream.str() != "abc" )
	{
		printf("%s: Failed to create the correct stream\n", TESTNAME);
		fail = true;
	}

	stream.clear();
	
	//-------------------------------
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	r = mod->Build();
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "Test2()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

