#include "utils.h"
#include "../../../add_on/scriptmath3d/scriptmath3d.h"

static const char * const TESTNAME = "TestVector3";

static char *script =
"vector3 TestVector3()  \n"
"{                      \n"
"  vector3 v;           \n"
"  v.x=1;               \n"
"  v.y=2;               \n"
"  v.z=3;               \n"
"  return v;            \n"
"}                      \n"
"vector3 TestVector3Val(vector3 v)  \n"
"{                                  \n"
"  return v;                        \n"
"}                                  \n"
"void TestVector3Ref(vector3 &out v)\n"
"{                                  \n"
"  v.x=1;                           \n"
"  v.y=2;                           \n"
"  v.z=3;                           \n"
"}                                  \n";

bool TestVector3()
{
	bool fail = false;
	COutStream out;
	CBufferedOutStream bout;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptMath3D(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	Vector3 v;
	engine->RegisterGlobalProperty("vector3 v", &v);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script);
	r = mod->Build();
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		fail = true;
	}
	else
	{
		// Internal return
		r = ExecuteString(engine, "v = TestVector3();", mod);
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			fail = true;
		}
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		// Manual return
		v.x = 0; v.y = 0; v.z = 0;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionIdByDecl("vector3 TestVector3()"));

		ctx->Execute();
		Vector3 *ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 1 || ret->y != 2 || ret->z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(mod->GetFunctionIdByDecl("vector3 TestVector3Val(vector3)"));
		v.x = 3; v.y = 2; v.z = 1;
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 3 || ret->y != 2 || ret->z != 1 )
		{
			printf("%s: Failed to pass Vector3 by val\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(mod->GetFunctionIdByDecl("void TestVector3Ref(vector3 &out)"));
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to pass Vector3 by ref\n", TESTNAME);
			fail = true;
		}

		ctx->Release();
	}

	// Assignment of temporary object
	r = ExecuteString(engine, "vector3 v; float x = (v = vector3(10.0f,7,8)).x; assert( x > 9.9999f && x < 10.0001f );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test some operator overloads
	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v*2).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (2*v).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v+v).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( v == vector3(1,0,0) );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v *= 2).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test error message when constructor is not found
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "vector3 v = vector3(4,3);");
	if( r >= 0 )
	{
		fail = true;
	}
	// TODO: the function signature for the constructors/factories should carry the name of the object
	if( bout.buffer != "ExecuteString (1, 13) : Error   : No matching signatures to 'vector3(const uint, const uint)'\n"
					   "ExecuteString (1, 13) : Info    : Candidates are:\n"
					   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_()\n"
				   	   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_(const vector3&in)\n"
					   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_(float, float, float)\n"
	                   "ExecuteString (1, 13) : Error   : Can't implicitly convert from 'const int' to 'vector3&'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	return fail;
}
