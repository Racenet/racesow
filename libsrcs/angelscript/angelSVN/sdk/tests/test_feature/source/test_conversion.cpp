#include "utils.h"

namespace TestConversion
{

#define TESTNAME "TestConversion"


void TestI8(asIScriptGeneric*) {}
void TestI16(asIScriptGeneric*) {}
void TestInt(asIScriptGeneric*) {}
void TestI64(asIScriptGeneric*) {}
void TestUI8(asIScriptGeneric*) {}
void TestUI16(asIScriptGeneric*) {}
void TestUInt(asIScriptGeneric*) {}
void TestUI64(asIScriptGeneric*) {}
void TestFloat(asIScriptGeneric*) {}
void TestDouble(asIScriptGeneric*) {}

void TestI8ByRef(asIScriptGeneric*) {}
void TestI16ByRef(asIScriptGeneric*) {}
void TestIntByRef(asIScriptGeneric*) {}
void TestI64ByRef(asIScriptGeneric*) {}
void TestUI8ByRef(asIScriptGeneric*) {}
void TestUI16ByRef(asIScriptGeneric*) {}
void TestUIntByRef(asIScriptGeneric*) {}
void TestUI64ByRef(asIScriptGeneric*) {}
void TestFloatByRef(asIScriptGeneric*) {}
void TestDoubleByRef(asIScriptGeneric*) {}
/*
void TestC()
{
	char i8 = 0;
	short i16 = 0;
	long i = 0;
	unsigned char ui8 = 0;
	unsigned short ui16 = 0;
	unsigned long ui = 0;
	float f = 0;
	double d = 0;

	i = i;
	i = i8;
	i = i16;
	i = ui;
	i = ui8;
	i = ui16;
	i = f;
	i = d;
	TestInt(i);
	TestInt(i8);
	TestInt(i16);
	TestInt(ui);
	TestInt(ui8);
	TestInt(ui16);
	TestInt(f);
	TestInt(d);

	i8 = i;
	i8 = i8;
	i8 = i16;
	i8 = ui;
	i8 = ui8;
	i8 = ui16;
	i8 = f;
	i8 = d;
	TestI8(i);
	TestI8(i8);
	TestI8(i16);
	TestI8(ui);
	TestI8(ui8);
	TestI8(ui16);
	TestI8(f);
	TestI8(d);
//	TestI8ByRef(i);
//	TestI8ByRef(i8);
//	TestI8ByRef(i16);
//	TestI8ByRef(ui);
//	TestI8ByRef(ui8);
//	TestI8ByRef(ui16);
//	TestI8ByRef(f);
//	TestI8ByRef(d);

	i16 = i;
	i16 = i8;
	i16 = i16;
	i16 = ui;
	i16 = ui8;
	i16 = ui16;
	i16 = f;
	i16 = d;
	TestI16(i);
	TestI16(i8);
	TestI16(i16);
	TestI16(ui);
	TestI16(ui8);
	TestI16(ui16);
	TestI16(f);
	TestI16(d);
//	TestI16ByRef(i);
//	TestI16ByRef(i8);
//	TestI16ByRef(i16);
//	TestI16ByRef(ui);
//	TestI16ByRef(ui8);
//	TestI16ByRef(ui16);
//	TestI16ByRef(f);
//	TestI16ByRef(d);

	ui = i;
	ui = i8;
	ui = i16;
	ui = ui;
	ui = ui8;
	ui = ui16;
	ui = f;
	ui = d;
	TestUInt(i);
	TestUInt(i8);
	TestUInt(i16);
	TestUInt(ui);
	TestUInt(ui8);
	TestUInt(ui16);
	TestUInt(f);
	TestUInt(d);
//	TestUIntByRef(i);
//	TestUIntByRef(i8);
//	TestUIntByRef(i16);
//	TestUIntByRef(ui);
//	TestUIntByRef(ui8);
//	TestUIntByRef(ui16);
//	TestUIntByRef(f);
//	TestUIntByRef(d);

	ui8 = i;
	ui8 = i8;
	ui8 = i16;
	ui8 = ui;
	ui8 = ui8;
	ui8 = ui16;
	ui8 = f;
	ui8 = d;
	TestUI8(i);
	TestUI8(i8);
	TestUI8(i16);
	TestUI8(ui);
	TestUI8(ui8);
	TestUI8(ui16);
	TestUI8(f);
	TestUI8(d);
//	TestUI8ByRef(i);
//	TestUI8ByRef(i8);
//	TestUI8ByRef(i16);
//	TestUI8ByRef(ui);
//	TestUI8ByRef(ui8);
//	TestUI8ByRef(ui16);
//	TestUI8ByRef(f);
//	TestUI8ByRef(d);

	ui16 = i;
	ui16 = i8;
	ui16 = i16;
	ui16 = ui;
	ui16 = ui8;
	ui16 = ui16;
	ui16 = f;
	ui16 = d;
	TestUI16(i);
	TestUI16(i8);
	TestUI16(i16);
	TestUI16(ui);
	TestUI16(ui8);
	TestUI16(ui16);
	TestUI16(f);
	TestUI16(d);
//	TestUI16ByRef(i);
//	TestUI16ByRef(i8);
//	TestUI16ByRef(i16);
//	TestUI16ByRef(ui);
//	TestUI16ByRef(ui8);
//	TestUI16ByRef(ui16);
//	TestUI16ByRef(f);
//	TestUI16ByRef(d);

	f = i;
	f = i8;
	f = i16;
	f = ui;
	f = ui8;
	f = ui16;
	f = f;
	f = d;
	TestFloat(i);
	TestFloat(i8);
	TestFloat(i16);
	TestFloat(ui);
	TestFloat(ui8);
	TestFloat(ui16);
	TestFloat(f);
	TestFloat(d);
//	TestFloatByRef(i);
//	TestFloatByRef(i8);
//	TestFloatByRef(i16);
//	TestFloatByRef(ui);
//	TestFloatByRef(ui8);
//	TestFloatByRef(ui16);
//	TestFloatByRef(f);
//	TestFloatByRef(d);

	d = i;
	d = i8;
	d = i16;
	d = ui;
	d = ui8;
	d = ui16;
	d = f;
	d = d;
	TestDouble(i);
	TestDouble(i8);
	TestDouble(i16);
	TestDouble(ui);
	TestDouble(ui8);
	TestDouble(ui16);
	TestDouble(f);
	TestDouble(d);
//	TestDoubleByRef(i);
//	TestDoubleByRef(i8);
//	TestDoubleByRef(i16);
//	TestDoubleByRef(ui);
//	TestDoubleByRef(ui8);
//	TestDoubleByRef(ui16);
//	TestDoubleByRef(f);
//	TestDoubleByRef(d);
}
*/

static const char *script =
"void TestScript()                  \n"
"{                                  \n"
"  double a = 1.2345;               \n"
"  TestSFloat(a);                   \n"
"  float b = 1.2345f;               \n"
"  TestSDouble(b);                  \n"
"}                                  \n"
"void TestSFloat(float a)           \n"
"{                                  \n"
"  Assert(a == 1.2345f);            \n"
"}                                  \n"
"void TestSDouble(double a)         \n"
"{                                  \n"
"  Assert(a == double(1.2345f));    \n"
"}                                  \n";

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	COutStream out;
	CBufferedOutStream bout;

	float f = 0;
	double d = 0;
	asUINT ui = 0;
	asINT64 i64 = 0;
	int i = 0;
	signed char i8 = 0;
	asQWORD ui64 = 0;
	short i16 = 0;
	unsigned char ui8 = 0;
	unsigned short ui16 = 0;
	engine->RegisterGlobalProperty("float f", &f);
	engine->RegisterGlobalProperty("double d", &d);
	engine->RegisterGlobalProperty("uint ui", &ui);
	engine->RegisterGlobalProperty("uint8 ui8", &ui8);
	engine->RegisterGlobalProperty("uint16 ui16", &ui16);
	engine->RegisterGlobalProperty("uint64 ui64", &ui64);
	engine->RegisterGlobalProperty("int i", &i);
	engine->RegisterGlobalProperty("int8 i8", &i8);
	engine->RegisterGlobalProperty("int16 i16", &i16);
	engine->RegisterGlobalProperty("int64 i64", &i64);

	engine->RegisterGlobalFunction("void TestDouble(double)", asFUNCTION(TestDouble), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestFloat(float)", asFUNCTION(TestFloat), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI64(int16)", asFUNCTION(TestI64), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestInt(int)", asFUNCTION(TestInt), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI16(int16)", asFUNCTION(TestI16), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI8(int8)", asFUNCTION(TestI8), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI64(uint)", asFUNCTION(TestUI64), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUInt(uint)", asFUNCTION(TestUInt), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI16(uint16)", asFUNCTION(TestUI16), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI8(uint8)", asFUNCTION(TestUI8), asCALL_GENERIC);

	engine->RegisterGlobalFunction("void TestDoubleByRef(double &in)", asFUNCTION(TestDoubleByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestFloatByRef(float &in)", asFUNCTION(TestFloatByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI64ByRef(int &in)", asFUNCTION(TestI64ByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestIntByRef(int &in)", asFUNCTION(TestIntByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI16ByRef(int16 &in)", asFUNCTION(TestI16ByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestI8ByRef(int8 &in)", asFUNCTION(TestI8ByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI64ByRef(uint &in)", asFUNCTION(TestUI64ByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUIntByRef(uint &in)", asFUNCTION(TestUIntByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI16ByRef(uint16 &in)", asFUNCTION(TestUI16ByRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestUI8ByRef(uint8 &in)", asFUNCTION(TestUI8ByRef), asCALL_GENERIC);

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	d    = 12.3;  engine->ExecuteString(0, "d = double(d);");    if( d  !=   12.3  ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "d = double(f);");    if( d  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "d = double(ui);");   if( d  !=  123.0  ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "d = double(ui8);");  if( d  !=  123.0  ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "d = double(ui16);"); if( d  !=  123.0  ) fail = true;
	ui64 = 123;   engine->ExecuteString(0, "d = double(ui64);"); if( d  !=  123.0  ) fail = true;
	i    = -123;  engine->ExecuteString(0, "d = double(i);");    if( d  != -123.0  ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "d = double(i8);");   if( d  != -123.0  ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "d = double(i16);");  if( d  != -123.0  ) fail = true;
	i64  = -123;  engine->ExecuteString(0, "d = double(i64);");  if( d  != -123.0  ) fail = true;

	ui64 = asQWORD(I64(-1000000000000000000)); 
	double d2 = 18446744073709551615.0 + double(asINT64(ui64));
	engine->ExecuteString(0, "d = double(ui64);"); if( d  !=  d2  ) fail = true;

	d    = 12.3;  engine->ExecuteString(0, "d = d;");    if( d  !=   12.3  ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "d = f;");    if( d  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "d = ui;");   if( d  !=  123.0  ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "d = ui8;");  if( d  !=  123.0  ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "d = ui16;"); if( d  !=  123.0  ) fail = true;
	ui64 = 123;   engine->ExecuteString(0, "d = ui64;"); if( d  !=  123.0  ) fail = true;
	i    = -123;  engine->ExecuteString(0, "d = i;");    if( d  != -123.0  ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "d = i8;");   if( d  != -123.0  ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "d = i16;");  if( d  != -123.0  ) fail = true;
	i64  = -123;  engine->ExecuteString(0, "d = i64;");  if( d  != -123.0  ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "d = 12.3; "); if( !CompareDouble(d,12.3) ) fail = true; 
	engine->ExecuteString(0, "d = 12.3f;"); if( !CompareDouble(d,12.3f) ) fail = true; 
	engine->ExecuteString(0, "d = 123;  "); if( !CompareDouble(d,123.0) ) fail = true;
	engine->ExecuteString(0, "d = -123; "); if( !CompareDouble(d,-123.0) ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;  engine->ExecuteString(0, "f = float(d);");     if( f  !=   12.3f ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "f = float(f);");     if( f  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "f = float(ui);");    if( f  !=  123.0f ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "f = float(ui8);");   if( f  !=  123.0f ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "f = float(ui16);");  if( f  !=  123.0f ) fail = true;
	ui64 = 123;   engine->ExecuteString(0, "f = float(ui64);");  if( f  !=  123.0f ) fail = true;
	i    = -123;  engine->ExecuteString(0, "f = float(i);");     if( f  != -123.0f ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "f = float(i8);");    if( f  != -123.0f ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "f = float(i16);");   if( f  != -123.0f ) fail = true;
	i64  = -123;  engine->ExecuteString(0, "f = float(i64);");   if( f  != -123.0f ) fail = true;

	d    = 12.3;  engine->ExecuteString(0, "f = d;");     if( f  !=   12.3f ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "f = f;");     if( f  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "f = ui;");    if( f  !=  123.0f ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "f = ui8;");   if( f  !=  123.0f ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "f = ui16;");  if( f  !=  123.0f ) fail = true;
	ui64 = 123;   engine->ExecuteString(0, "f = ui64;");  if( f  !=  123.0f ) fail = true;
	i    = -123;  engine->ExecuteString(0, "f = i;");     if( f  != -123.0f ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "f = i8;");    if( f  != -123.0f ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "f = i16;");   if( f  != -123.0f ) fail = true;
	i64  = -123;  engine->ExecuteString(0, "f = i64;");   if( f  != -123.0f ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "f = 12.3; "); if( f  !=   12.3f ) fail = true; 
	engine->ExecuteString(0, "f = 12.3f;"); if( f  !=   12.3f ) fail = true; 
	engine->ExecuteString(0, "f = 123;  "); if( f  !=  123.0f ) fail = true;
	engine->ExecuteString(0, "f = -123; "); if( f  != -123.0f ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "i64 = int64(d);");      if( i64  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i64 = int64(f);");      if( i64  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i64 = int64(ui);");     if( i64  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i64 = int64(ui8);");    if( i64  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i64 = int64(ui16);");   if( i64  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i64 = int64(ui64);");   if( i64  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i64 = int64(i);");      if( i64  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i64 = int64(i8);");     if( i64  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i64 = int64(i16);");    if( i64  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i64 = int64(i64);");    if( i64  != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i64 = d;");      if( i64  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i64 = f;");      if( i64  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i64 = ui;");     if( i64  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i64 = ui8;");    if( i64  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i64 = ui16;");   if( i64  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i64 = ui64;");   if( i64  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i64 = i;");      if( i64  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i64 = i8;");     if( i64  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i64 = i16;");    if( i64  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i64 = i64;");    if( i64  != -123 ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "i64 = 12.3;  "); if( i64  !=   12 ) fail = true; 
	engine->ExecuteString(0, "i64 = -12.3f;"); if( i64  != - 12 ) fail = true;
	engine->ExecuteString(0, "i64 = 123;   "); if( i64  !=  123 ) fail = true;
	engine->ExecuteString(0, "i64 = -123;  "); if( i64  != -123 ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "i = int(d);");      if( i  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i = int(f);");      if( i  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i = int(ui);");     if( i  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i = int(ui8);");    if( i  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i = int(ui16);");   if( i  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i = int(ui64);");   if( i  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i = int(i);");      if( i  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i = int(i8);");     if( i  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i = int(i16);");    if( i  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i = int(i64);");    if( i  != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i = d;");      if( i  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i = f;");      if( i  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i = ui;");     if( i  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i = ui8;");    if( i  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i = ui16;");   if( i  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i = ui64;");   if( i  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i = i;");      if( i  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i = i8;");     if( i  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i = i16;");    if( i  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i = i64;");    if( i  != -123 ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "i = 12.3;  "); if( i  !=   12 ) fail = true; 
	engine->ExecuteString(0, "i = -12.3f;"); if( i  != - 12 ) fail = true;
	engine->ExecuteString(0, "i = 123;   "); if( i  !=  123 ) fail = true;
	engine->ExecuteString(0, "i = -123;  "); if( i  != -123 ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "i8 = int8(d);");     if( i8 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i8 = int8(f);");     if( i8 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i8 = int8(ui);");    if( i8 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i8 = int8(ui8);");   if( i8 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i8 = int8(ui16);");  if( i8 !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i8 = int8(ui64);");  if( i8 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i8 = int8(i);");     if( i8 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i8 = int8(i8);");    if( i8 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i8 = int8(i16);");   if( i8 != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i8 = int8(i64);");   if( i8 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i8 = d;");     if( i8 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i8 = f;");     if( i8 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i8 = ui;");    if( i8 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i8 = ui8;");   if( i8 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i8 = ui16;");  if( i8 !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i8 = ui64;");  if( i8 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i8 = i;");     if( i8 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i8 = i8;");    if( i8 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i8 = i16;");   if( i8 != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i8 = i64;");   if( i8 != -123 ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "i8 = 12.3;  "); if( i8 !=   12 ) fail = true; 
	engine->ExecuteString(0, "i8 = -12.3f;"); if( i8 != - 12 ) fail = true;
	engine->ExecuteString(0, "i8 = 123;   "); if( i8 !=  123 ) fail = true;
	engine->ExecuteString(0, "i8 = -123;  "); if( i8 != -123 ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "i16 = int16(d);");    if( i16 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i16 = int16(f);");    if( i16 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i16 = int16(ui);");   if( i16 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i16 = int16(ui8);");  if( i16 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i16 = int16(ui16);"); if( i16 !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i16 = int16(ui64);"); if( i16 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i16 = int16(i);");    if( i16 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i16 = int16(i8);");   if( i16 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i16 = int16(i16);");  if( i16 != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i16 = int16(i64);");  if( i16 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i16 = d;");    if( i16 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i16 = f;");    if( i16 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i16 = ui;");   if( i16 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i16 = ui8;");  if( i16 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i16 = ui16;"); if( i16 !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "i16 = ui64;"); if( i16 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i16 = i;");    if( i16 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i16 = i8;");   if( i16 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i16 = i16;");  if( i16 != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "i16 = i64;");  if( i16 != -123 ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "i16 = 12.3;  "); if( i16 !=   12 ) fail = true; 
	engine->ExecuteString(0, "i16 = -12.3f;"); if( i16 != - 12 ) fail = true;
	engine->ExecuteString(0, "i16 = 123;   "); if( i16 !=  123 ) fail = true;
	engine->ExecuteString(0, "i16 = -123;  "); if( i16 != -123 ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "ui64 = uint64(d);");      if( ui64  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui64 = uint64(f);");      if( ui64  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui64 = uint64(ui);");     if( ui64  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui64 = uint64(ui8);");    if( ui64  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui64 = uint64(ui16);");   if( ui64  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui64 = uint64(ui64);");   if( ui64  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui64 = uint64(i);");      if( ui64  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui64 = uint64(i8);");     if( ui64  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui64 = uint64(i16);");    if( ui64  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui64 = uint64(i64);");    if( ui64  != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui64 = d;");      if( ui64  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui64 = f;");      if( ui64  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui64 = ui;");     if( ui64  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui64 = ui8;");    if( ui64  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui64 = ui16;");   if( ui64  !=  123 ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui64 = ui64;");   if( ui64  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui64 = i;");      if( ui64  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui64 = i8;");     if( ui64  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui64 = i16;");    if( ui64  != -123 ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui64 = i64;");    if( ui64  != -123 ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "ui64 = 12.3;  "); if( ui64  !=   12 ) fail = true; 
	engine->ExecuteString(0, "ui64 = -12.3f;"); if( ui64  != - 12 ) fail = true;
	engine->ExecuteString(0, "ui64 = 123;   "); if( ui64  !=  123 ) fail = true;
	engine->ExecuteString(0, "ui64 = -123;  "); if( ui64  != -123 ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "ui = uint(d);");    if( ui != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui = uint(f);");    if( ui != asUINT(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui = uint(ui);");   if( ui !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui = uint(ui8);");  if( ui !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui = uint(ui16);"); if( ui !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui = uint(ui64);"); if( ui !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui = uint(i);");    if( ui != asUINT(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui = uint(i8);");   if( ui != asUINT(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui = uint(i16);");  if( ui != asUINT(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui = uint(i64);");  if( ui != asUINT(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui = d;");    if( ui != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui = f;");    if( ui != asUINT(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui = ui;");   if( ui !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui = ui8;");  if( ui !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui = ui16;"); if( ui !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui = ui64;"); if( ui !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui = i;");    if( ui != asUINT(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui = i8;");   if( ui != asUINT(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui = i16;");  if( ui != asUINT(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui = i64;");  if( ui != asUINT(-123) ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "ui = 12.3;  "); if( ui != 12           ) fail = true; 
	engine->ExecuteString(0, "ui = -12.3f;"); if( ui != asUINT(-12)  ) fail = true;
	engine->ExecuteString(0, "ui = 123;   "); if( ui !=  123         ) fail = true;
	engine->ExecuteString(0, "ui = -123;  "); if( ui != asUINT(-123) ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "ui8 = uint8(d);");    if( ui8 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui8 = uint8(f);");    if( ui8 != asBYTE(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui8 = uint8(ui);");   if( ui8 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui8 = uint8(ui8);");  if( ui8 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui8 = uint8(ui16);"); if( ui8 !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui8 = uint8(ui64);"); if( ui8 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui8 = uint8(i);");    if( ui8 != asBYTE(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui8 = uint8(i8);");   if( ui8 != asBYTE(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui8 = uint8(i16);");  if( ui8 != asBYTE(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui8 = uint8(i64);");  if( ui8 != asBYTE(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui8 = d;");    if( ui8 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui8 = f;");    if( ui8 != asBYTE(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui8 = ui;");   if( ui8 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui8 = ui8;");  if( ui8 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui8 = ui16;"); if( ui8 !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui8 = ui64;"); if( ui8 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui8 = i;");    if( ui8 != asBYTE(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui8 = i8;");   if( ui8 != asBYTE(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui8 = i16;");  if( ui8 != asBYTE(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui8 = i64;");  if( ui8 != asBYTE(-123) ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "ui8 = 12.3;  "); if( ui8 != 12           ) fail = true; 
	engine->ExecuteString(0, "ui8 = -12.3f;"); if( ui8 != asBYTE(-12)  ) fail = true; // asBYTE(-12.3f) doesn't seem to produce the same result on MSVC and GNUC
	engine->ExecuteString(0, "ui8 = 123;   "); if( ui8 !=  123         ) fail = true;
	engine->ExecuteString(0, "ui8 = -123;  "); if( ui8 != asBYTE(-123) ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	d    = 12.3;   engine->ExecuteString(0, "ui16 = uint16(d);");    if( ui16 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui16 = uint16(f);");    if( ui16 != asWORD(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui16 = uint16(ui);");   if( ui16 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui16 = uint16(ui8);");  if( ui16 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui16 = uint16(ui16);"); if( ui16 !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui16 = uint16(ui64);"); if( ui16 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui16 = uint16(i);");    if( ui16 != asWORD(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui16 = uint16(i8);");   if( ui16 != asWORD(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui16 = uint16(i16);");  if( ui16 != asWORD(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui16 = uint16(i64);");  if( ui16 != asWORD(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui16 = d;");    if( ui16 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui16 = f;");    if( ui16 != asWORD(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui16 = ui;");   if( ui16 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui16 = ui8;");  if( ui16 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui16 = ui16;"); if( ui16 !=  123         ) fail = true;
	ui64 = 123;    engine->ExecuteString(0, "ui16 = ui64;"); if( ui16 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui16 = i;");    if( ui16 != asWORD(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui16 = i8;");   if( ui16 != asWORD(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui16 = i16;");  if( ui16 != asWORD(-123) ) fail = true;
	i64  = -123;   engine->ExecuteString(0, "ui16 = i64;");  if( ui16 != asWORD(-123) ) fail = true;

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "ui16 = 12.3;  "); if( ui16 != 12           ) fail = true; 
	engine->ExecuteString(0, "ui16 = -12.3f;"); if( ui16 != asWORD(-12)  ) fail = true; // asWORD(-12.3f) doesn't seem to produce the same result on MSVC and GNUC
	engine->ExecuteString(0, "ui16 = 123;   "); if( ui16 !=  123         ) fail = true;
	engine->ExecuteString(0, "ui16 = -123;  "); if( ui16 != asWORD(-123) ) fail = true;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	engine->ExecuteString(0, "TestDouble(d); TestFloat(d); TestInt(d); TestI16(d); TestI8(d); TestUInt(d); TestUI16(d); TestUI8(d);");
	engine->ExecuteString(0, "TestDouble(f); TestFloat(f); TestInt(f); TestI16(f); TestI8(f); TestUInt(f); TestUI16(f); TestUI8(f);");
	engine->ExecuteString(0, "TestDouble(ui); TestFloat(ui); TestInt(ui); TestI16(ui); TestI8(ui); TestUInt(ui); TestUI16(ui); TestUI8(ui);");
	engine->ExecuteString(0, "TestDouble(ui8); TestFloat(ui8); TestInt(ui8); TestI16(ui8); TestI8(ui8); TestUInt(ui8); TestUI16(ui8); TestUI8(ui8);");
	engine->ExecuteString(0, "TestDouble(ui16); TestFloat(ui16); TestInt(ui16); TestI16(ui16); TestI8(ui16); TestUInt(ui16); TestUI16(ui16); TestUI8(ui16);");
	engine->ExecuteString(0, "TestDouble(ui64); TestFloat(ui64); TestInt(ui64); TestI16(ui64); TestI8(ui64); TestUInt(ui64); TestUI16(ui64); TestUI8(ui64);");
	engine->ExecuteString(0, "TestDouble(i); TestFloat(i); TestInt(i); TestI16(i); TestI8(i); TestUInt(i); TestUI16(i); TestUI8(i);");
	engine->ExecuteString(0, "TestDouble(i8); TestFloat(i8); TestInt(i8); TestI16(i8); TestI8(i8); TestUInt(i8); TestUI16(i8); TestUI8(i8);");
	engine->ExecuteString(0, "TestDouble(i16); TestFloat(i16); TestInt(i16); TestI16(i16); TestI8(i16); TestUInt(i16); TestUI16(i16); TestUI8(i16);");
	engine->ExecuteString(0, "TestDouble(i64); TestFloat(i64); TestInt(i64); TestI16(i64); TestI8(i64); TestUInt(i64); TestUI16(i64); TestUI8(i64);");

	d = 0; i8 = -22; engine->ExecuteString(0, "d = d + i8"); if( d != -22 ) fail = true;

	engine->ExecuteString(0, "int[] a(1); a[0] = 0; a[0] == 1");
	engine->ExecuteString(0, "ui + i");
	engine->ExecuteString(0, "int a = 0, ui = 0; (a+ui)&1;");

	// There is no bitwise conversion between uint to float anymore
	// f = 0; engine->ExecuteString(0, "f = float(0x3f800000)"); if( f != 1 ) fail = true;

	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "i == ui"); 
	if( bout.buffer != "ExecuteString (1, 3) : Warning : Signed/Unsigned mismatch\n" )
		fail = true;

	bout.buffer = "";

	int r;

	// TODO: PPC: We cannot allow this with PPC
	// Allow the conversion of a type to another even for reference parameters (C++ doesn't allow this)
	r = engine->ExecuteString(0, "TestDoubleByRef(d); TestFloatByRef(d); TestIntByRef(d); TestI16ByRef(d); TestI8ByRef(d); TestUIntByRef(d); TestUI16ByRef(d); TestUI8ByRef(d);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(f); TestFloatByRef(f); TestIntByRef(f); TestI16ByRef(f); TestI8ByRef(f); TestUIntByRef(f); TestUI16ByRef(f); TestUI8ByRef(f);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui); TestFloatByRef(ui); TestIntByRef(ui); TestI16ByRef(ui); TestI8ByRef(ui); TestUIntByRef(ui); TestUI16ByRef(ui); TestUI8ByRef(ui);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui8); TestFloatByRef(ui8); TestIntByRef(ui8); TestI16ByRef(ui8); TestI8ByRef(ui8); TestUIntByRef(ui8); TestUI16ByRef(ui8); TestUI8ByRef(ui8);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui16); TestFloatByRef(ui16); TestIntByRef(ui16); TestI16ByRef(ui16); TestI8ByRef(ui16); TestUIntByRef(ui16); TestUI16ByRef(ui16); TestUI8ByRef(ui16);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui64); TestFloatByRef(ui64); TestIntByRef(ui64); TestI16ByRef(ui64); TestI8ByRef(ui64); TestUIntByRef(ui64); TestUI16ByRef(ui64); TestUI8ByRef(ui64);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i); TestFloatByRef(i); TestIntByRef(i); TestI16ByRef(i); TestI8ByRef(i); TestUIntByRef(i); TestUI16ByRef(i); TestUI8ByRef(i);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i8); TestFloatByRef(i8); TestIntByRef(i8); TestI16ByRef(i8); TestI8ByRef(i8); TestUIntByRef(i8); TestUI16ByRef(i8); TestUI8ByRef(i8);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i16); TestFloatByRef(i16); TestIntByRef(i16); TestI16ByRef(i16); TestI8ByRef(i16); TestUIntByRef(i16); TestUI16ByRef(i16); TestUI8ByRef(i16);"); if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i64); TestFloatByRef(i64); TestIntByRef(i64); TestI16ByRef(i64); TestI8ByRef(i64); TestUIntByRef(i64); TestUI16ByRef(i64); TestUI8ByRef(i64);"); if( r < 0 ) fail = true;

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	mod->Build();

	// This test is to make sure that the float is in fact converted to a double
	engine->ExecuteString(0, "TestScript();");

	// Make sure uint and int can be converted to bits when using the ~ operator
	engine->ExecuteString(0, "uint x = 0x34; x = ~x;");
	engine->ExecuteString(0, "int x = 0x34; x = ~x;");

	engine->Release();

	if( fail )
		printf("%s: failed\n", TESTNAME);

	// Success
	return fail;
}

} // namespace

