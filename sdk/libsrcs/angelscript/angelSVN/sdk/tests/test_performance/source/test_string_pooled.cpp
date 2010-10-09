//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include <assert.h>
#include <string>
#include <vector>

using namespace std;

namespace TestStringPooled
{


class CScriptString2
{
public:
	CScriptString2();
	CScriptString2(const char *s, unsigned int len);
	CScriptString2(const std::string &s);
	~CScriptString2();

	void AddRef();
	void Release();

	CScriptString2 &operator=(const CScriptString2 &other);
	CScriptString2 &operator+=(const CScriptString2 &other);
	friend CScriptString2 *operator+(const CScriptString2 &a, const CScriptString2 &b);

	std::string buffer;

protected:
	int refCount;
};

static vector<CScriptString2*> pool;

CScriptString2::CScriptString2()
{
	// Count the first reference
	refCount = 1;
}

CScriptString2::CScriptString2(const char *s, unsigned int len)
{
	refCount = 1;
	buffer.assign(s, len);
}

CScriptString2::CScriptString2(const string &s)
{
	refCount = 1;
	buffer = s;
}

CScriptString2::~CScriptString2()
{
	assert( refCount == 0 );
}

void CScriptString2::AddRef()
{
	refCount++;
}

void CScriptString2::Release()
{
	if( --refCount == 0 )
	{
		// Move this to the pool
		pool.push_back(this);
	}
}

CScriptString2 &CScriptString2::operator=(const CScriptString2 &other)
{
	// Copy only the buffer, not the reference counter
	buffer = other.buffer;

	// Return a reference to this object
	return *this;
}

CScriptString2 *operator+(const CScriptString2 &a, const CScriptString2 &b)
{
	if( pool.size() > 0 )
	{
		CScriptString2 *str = pool[pool.size()-1];
		pool.pop_back();
		str->buffer.resize(a.buffer.length() + b.buffer.length());
		str->buffer += a.buffer;
		str->buffer += b.buffer;
		str->AddRef();
		return str;
	}

	// Return a new object as a script handle
	return new CScriptString2(a.buffer + b.buffer);
}

// This is the string factory that creates new strings for the script based on string literals
static CScriptString2 *StringFactory(asUINT length, const char *s)
{
	if( pool.size() > 0 )
	{
		CScriptString2 *str = pool[pool.size()-1];
		pool.pop_back();
		str->buffer.assign(s, length);
		str->AddRef();
		return str;
	}

	return new CScriptString2(s, length);
}

// This is the default string factory, that is responsible for creating empty string objects, e.g. when a variable is declared
static CScriptString2 *StringDefaultFactory()
{
	if( pool.size() > 0 )
	{
		CScriptString2 *str = pool[pool.size()-1];
		pool.pop_back();
		str->buffer.resize(0);
		str->AddRef();
		return str;
	}

	// Allocate and initialize with the default constructor
	return new CScriptString2();
}

// Copy constructor
static CScriptString2 *StringCopyFactory(const string &s)
{
	if( pool.size() > 0 )
	{
		CScriptString2 *str = pool[pool.size()-1];
		pool.pop_back();
		str->buffer.assign(s);
		str->AddRef();
		return str;
	}

	// Allocate and initialize with the default constructor
	return new CScriptString2(s);
}

// This is where we register the string type
void RegisterScriptString2(asIScriptEngine *engine)
{
	int r;

	// Register the type
	r = engine->RegisterObjectType("string", 0, asOBJ_REF); assert( r >= 0 );

	// Register the object operator overloads
	// Note: We don't have to register the destructor, since the object uses reference counting
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY,    "string @f()",                 asFUNCTION(StringDefaultFactory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY,    "string @f(const string &in)", asFUNCTION(StringCopyFactory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADDREF,     "void f()",                    asMETHOD(CScriptString2,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_RELEASE,    "void f()",                    asMETHOD(CScriptString2,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(CScriptString2, operator =, (const CScriptString2&), CScriptString2&), asCALL_THISCALL); assert( r >= 0 );

	// Register the factory to return a handle to a new string
	// Note: We must register the string factory after the basic behaviours,
	// otherwise the library will not allow the use of object handles for this type
	r = engine->RegisterStringFactory("string@", asFUNCTION(StringFactory), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "string@ opAdd(const string &in) const", asFUNCTIONPR(operator +, (const CScriptString2 &, const CScriptString2 &), CScriptString2*), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
}


#define TESTNAME "TestStringPooled"

static const char *script =
"string BuildStringP(string &in a, string &in b, string &in c)    \n"
"{                                                                \n"
"    return a + b + c;                                            \n"
"}                                                                \n"
"                                                                 \n"
"void TestStringP()                                               \n"
"{                                                                \n"
"    string a = \"Test\";                                         \n"
"    string b = \"string\";                                       \n"
"    int i = 0;                                                   \n"
"                                                                 \n"
"    for ( i = 0; i < 1000000; i++ )                              \n"
"    {                                                            \n"
"        string res = BuildStringP(a, \" \", b);                  \n"
"    }                                                            \n"
"}                                                                \n";

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.18.0             : 2.48 secs\n");
	printf("AngelScript 2.18.1 WIP         : 2.61 secs\n");
	printf("AngelScript 2.19.1 WIP         : 2.63 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString2(engine);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script, strlen(script), 0);
	int r = mod->Build();
	if( r >= 0 )
	{
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionIdByDecl("void TestStringP()"));

		printf("Executing AngelScript version...\n");

		double time = GetSystemTimer();

		r = ctx->Execute();

		time = GetSystemTimer() - time;

		if( r != 0 )
		{
			printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
			if( r == asEXECUTION_EXCEPTION )
			{
				printf("Script exception\n");
				asIScriptFunction *func = engine->GetFunctionDescriptorById(ctx->GetExceptionFunction());
				printf("Func: %s\n", func->GetName());
				printf("Line: %d\n", ctx->GetExceptionLineNumber());
				printf("Desc: %s\n", ctx->GetExceptionString());
			}
		}
		else
			printf("Time = %f secs\n", time);

		ctx->Release();
	}
	else
		printf("Build failed\n");
	engine->Release();

	// Clean up the string pool
	for( asUINT n = 0; n < pool.size(); n++ )
		delete pool[n];
}

} // namespace







