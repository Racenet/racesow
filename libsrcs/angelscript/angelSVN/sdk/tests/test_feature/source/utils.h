#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <math.h>
#include <vector>

#include <angelscript.h>

#include "../../../add_on/scriptstring/scriptstring.h"
#include "../../../add_on/scriptstdstring/scriptstdstring.h"

#ifdef AS_USE_NAMESPACE
using namespace AngelScript;
#endif

#if defined(__GNUC__) && !(defined(__ppc__) || defined(__PPC__))
#define STDCALL __attribute__((stdcall))
#elif defined(_MSC_VER)
#define STDCALL __stdcall
#else
#define STDCALL
#endif

class COutStream
{
public:
	void Callback(asSMessageInfo *msg) 
	{ 
		const char *msgType = 0;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
	}
};

class CBufferedOutStream
{
public:
	void Callback(asSMessageInfo *msg) 
	{ 
		const char *msgType = 0;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		char buf[256];
#ifdef _MSC_VER
#if _MSC_VER >= 1500
		_snprintf_s(buf, 255, 255, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
#else
		_snprintf(buf, 255, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
#endif
#else
		snprintf(buf, 255, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
#endif
		buf[255] = '\0';

		buffer += buf;
	}

	std::string buffer;
};

#ifdef STREAM_TO_FILE
class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream(const char *name) {this->name = name; this->name += ".stream"; f = 0; isReading = false;}
	~CBytecodeStream() { if( f ) fclose(f); }

	void Write(const void *ptr, asUINT size) 
	{
		if( size == 0 ) return; 
		if( f == 0 || isReading ) 
		{ 
			if( f ) 
				fclose(f); 
			f = fopen(name.c_str(), "wb"); 
			isReading = false;
		} 
		fwrite(ptr, size, 1, f); 
	}
	void Read(void *ptr, asUINT size) 
	{ 
		if( size == 0 ) return; 
		if( f == 0 || !isReading ) 
		{ 
			if( f ) 
				fclose(f); 
			f = fopen(name.c_str(), "rb");
			isReading = true;
		} 
		fread(ptr, size, 1, f); 
	}
	void Restart() {if( f ) fseek(f, 0, SEEK_SET);}

protected:
	std::string name;
	FILE *f;
	bool isReading;
};
#else
class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream(const char *name) {wpointer = 0;rpointer = 0;}

	void Write(const void *ptr, asUINT size) {if( size == 0 ) return; buffer.resize(buffer.size() + size); memcpy(&buffer[wpointer], ptr, size); wpointer += size;}
	void Read(void *ptr, asUINT size) {memcpy(ptr, &buffer[rpointer], size); rpointer += size;}
	void Restart() {rpointer = 0;}

protected:
	int rpointer;
	int wpointer;
	std::vector<asBYTE> buffer;
};
#endif

void PrintException(asIScriptContext *ctx);
void Assert(asIScriptGeneric *gen);

void InstallMemoryManager();
void RemoveMemoryManager();


#if defined(_MSC_VER) && _MSC_VER <= 1200 // MSVC++ 6
	#define I64(x) x##l
#else // MSVC++ 7, GNUC, etc
	#define I64(x) x##ll
#endif

#endif

inline bool CompareDouble(double a,double b)
{
	if( fabs( a - b ) > 0.00000001 )
		return false;
	return true;
}

inline bool CompareFloat(float a,float b)
{
	if( fabsf( a - b ) > 0.000001f )
		return false;
	return true;
}

#define UNUSED_VAR(x) ((void)(x))

