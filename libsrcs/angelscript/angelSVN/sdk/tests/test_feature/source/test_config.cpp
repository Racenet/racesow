#include "utils.h"

namespace TestConfig
{

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;
	CBufferedOutStream bout;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->ClearMessageCallback(); // Make sure this works
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	r = engine->RegisterGlobalFunction("void func(mytype)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) TEST_FAILED;

	r = engine->RegisterGlobalFunction("void func(int &)", asFUNCTION(0), asCALL_GENERIC);
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( r >= 0 ) TEST_FAILED;
	}
	else
	{
		if( r < 0 ) TEST_FAILED;
	}
	
	r = engine->RegisterObjectType("mytype", 0, asOBJ_REF);
	if( r < 0 ) TEST_FAILED;

	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_CONSTRUCT, "void f(othertype)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) TEST_FAILED;

	r = engine->RegisterObjectMethod("mytype", "type opAdd(int) const", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) TEST_FAILED;

	r = engine->RegisterGlobalProperty("type a", 0);
	if( r >= 0 ) TEST_FAILED;

	r = engine->RegisterObjectMethod("mytype", "void method(int &)", asFUNCTION(0), asCALL_GENERIC);
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( r >= 0 ) TEST_FAILED;
	}
	else
	{
		if( r < 0 ) TEST_FAILED;
	}

	r = engine->RegisterObjectProperty("mytype", "type a", 0);
	if( r >= 0 ) TEST_FAILED;

	r = engine->RegisterStringFactory("type", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) TEST_FAILED;

	// Verify the output messages
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
			               " (0, 0) : Error   : Failed in call to function 'RegisterGlobalFunction' with 'void func(mytype)'\n"
						   "System function (1, 15) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterGlobalFunction' with 'void func(int &)'\n"
						   "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterObjectBehaviour' with 'mytype' and 'void f(othertype)'\n"
						   "System function (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterObjectMethod' with 'mytype' and 'type opAdd(int) const'\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterGlobalProperty' with 'type a'\n"
						   "System function (1, 17) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterObjectMethod' with 'mytype' and 'void method(int &)'\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterObjectProperty' with 'mytype' and 'type a'\n"
						   " (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterStringFactory' with 'type'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}
	else
	{
		if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
						   "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
						   "System function (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "System function (1, 8) : Error   : Identifier 'type' is not a data type\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (1, 1) : Error   : Identifier 'type' is not a data type\n")
			TEST_FAILED;
	}

	engine->Release();

	// Success
 	return fail;
}

} // namespace

