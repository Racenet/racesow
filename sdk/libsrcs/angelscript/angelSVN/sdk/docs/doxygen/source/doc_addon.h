/**

\page doc_addon Add-ons

This page gives a brief description of the add-ons that you'll find in the /sdk/add_on/ folder.

 - \subpage doc_addon_application
 - \subpage doc_addon_script

\page doc_addon_application Application modules

 - \subpage doc_addon_build
 - \subpage doc_addon_autowrap
 - \subpage doc_addon_clib

\page doc_addon_script Script extensions

 - \subpage doc_addon_std_string
 - \subpage doc_addon_string
 - \subpage doc_addon_any
 - \subpage doc_addon_dict
 - \subpage doc_addon_file
 - \subpage doc_addon_math
 - \subpage doc_addon_math3d



\page doc_addon_any any object

<b>Path:</b> /sdk/add_on/scriptany/

The <code>any</code> type is a generic container that can hold any value. It is a reference type.

The type is registered with <code>RegisterScriptAny(asIScriptEngine*);</code>.

\section doc_addon_any_1 Public C++ interface

\code
class CScriptAny
{
public:
  // Constructors
  CScriptAny(asIScriptEngine *engine);
  CScriptAny(void *ref, int refTypeId, asIScriptEngine *engine);

  // Memory management
  int AddRef();
  int Release();

  // Copy the stored value from another any object
  CScriptAny &operator=(const CScriptAny&);
  int CopyFrom(const CScriptAny *other);

  // Store the value, either as variable type, integer number, or real number
  void Store(void *ref, int refTypeId);
  void Store(asINT64 &value);
  void Store(double &value);

  // Retrieve the stored value, either as variable type, integer number, or real number
  bool Retrieve(void *ref, int refTypeId) const;
  bool Retrieve(asINT64 &value) const;
  bool Retrieve(double &value) const;

  // Get the type id of the stored value
  int GetTypeId() const;
};
\endcode

\section doc_addon_any_2 Public script interface

<pre>
class any
{
  any();
  any(? &in value);
  
  void store(? &in value);
  void store(int64 &in value);
  void store(double &in value);
  
  bool retrieve(? &out value) const;
  bool retrieve(int64 &out value) const;
  bool retrieve(double &out value) const;
}
</pre>

\section doc_addon_any_3 Example usage

In the scripts it can be used as follows:

<pre>
  int value;
  obj object;
  obj \@handle;
  any a,b,c;
  a.store(value);      // store the value
  b.store(\@handle);    // store an object handle
  c.store(object);     // store a copy of the object
  
  a.retrieve(value);   // retrieve the value
  b.retrieve(\@handle); // retrieve the object handle
  c.retrieve(object);  // retrieve a copy of the object
</pre>

In C++ the type can be used as follows:

\code
CScriptAny *myAny;
int typeId = engine->GetTypeIdByDecl("string@");
CScriptString *str = new CScriptString("hello world");
myAny->Store((void*)&str, typeId);
myAny->Retrieve((void*)&str, typeId);
\endcode



\page doc_addon_std_string string object (STL)

<b>Path:</b> /sdk/add_on/scriptstdstring/

This add-on registers the <code>std::string</code> type as-is with AngelScript. This gives
perfect compatibility with C++ functions that use <code>std::string</code> in parameters or
as return type.

A potential drawback is that the <code>std::string</code> type is a value type, thus may 
increase the number of copies taken when string values are being passed around
in the script code. However, this is most likely only a problem for scripts 
that perform a lot of string operations.

Register the type with <code>RegisterStdString(asIScriptEngine*)</code>.

\see \ref doc_addon_string

\section doc_addon_std_string_1 Public C++ interface

Refer to the <code>std::string</code> implementation for your compiler.

\section doc_addon_std_string_2 Public script interface

<pre>
  class string
  {
    // Constructors
    string();
    
    // Returns the length of the string
    uint length() const;
    
    // The string class has several operators that are not expressable in the script syntax yet

    // Assignment and concatenation
    // string & operator =  (const string &in other)
    // string & operator += (const string &in other)
    // string   operator +  (const string &in a, const string &in b)
    
    // Access individual characters
    // uint8 & operator [] (uint)
    // const uint8 & operator [] (uint) const
    
    // Comparison operators
    // bool operator == (const string &in a, const string &in b)
    // bool operator != (const string &in a, const string &in b)
    // bool operator <  (const string &in a, const string &in b)
    // bool operator <= (const string &in a, const string &in b)
    // bool operator >  (const string &in a, const string &in b)
    // bool operator >= (const string &in a, const string &in b)
    
    // Automatic conversion from number types to string type
    // string & operator =  (double val)
    // string & operator += (double val)
    // string @ operator +  (double val, const string &in str)
    // string @ operator +  (const string &in str, double val)
    // string & operator =  (int val)
    // string & operator += (int val)
    // string @ operator +  (int val, const string &in str)
    // string @ operator +  (const string &in str, int val)
    // string & operator =  (uint val)
    // string & operator += (uint val)
    // string @ operator +  (uint val, const string &in str)
    // string @ operator +  (const string &in str, uint val)
  }
</pre>




\page doc_addon_string string object (reference counted)

<b>Path:</b> /sdk/add_on/scriptstring/

This add-on registers a string type that is in most situations compatible with the 
<code>std::string</code>, except that it uses reference counting. This means that if you have an
application function that takes a <code>std::string</code> by reference, you can register it 
with AngelScript to take a script string by reference. This works because the CScriptString
wraps the <code>std::string</code> type, with the std::string type at the first byte of the CScriptString
object.

Register the type with <code>RegisterScriptString(asIScriptEngine*)</code>. Register the 
utility functions with <code>RegisterScriptStringUtils(asIScriptEngine*)</code>.

\see \ref doc_addon_std_string

\section doc_addon_string_1 Public C++ interface

\code
class CScriptString
{
public:
  // Constructors
  CScriptString();
  CScriptString(const CScriptString &other);
  CScriptString(const char *s);
  CScriptString(const std::string &s);

  // Memory management
  void AddRef();
  void Release();

  // Assignment
  CScriptString &operator=(const CScriptString &other);
  
  // Concatenation
  CScriptString &operator+=(const CScriptString &other);
  friend CScriptString *operator+(const CScriptString &a, const CScriptString &b);
  
  // Memory buffer
  std::string buffer;
};
\endcode

\section doc_addon_string_2 Public script interface

<pre>
  class string
  {
    // Constructors
    string();
    string(const string &in other);
    
    // Returns the length of the string
    uint length() const;
    
    // The string class has several operators that are not expressable in the script syntax yet

    // Assignment and concatenation
    // string & operator =  (const string &in other)
    // string & operator += (const string &in other)
    // string @ operator +  (const string &in a, const string &in b)
    
    // Access individual characters
    // uint8 & operator [] (uint)
    // const uint8 & operator [] (uint) const
    
    // Comparison operators
    // bool operator == (const string &in a, const string &in b)
    // bool operator != (const string &in a, const string &in b)
    // bool operator <  (const string &in a, const string &in b)
    // bool operator <= (const string &in a, const string &in b)
    // bool operator >  (const string &in a, const string &in b)
    // bool operator >= (const string &in a, const string &in b)
    
    // Automatic conversion from number types to string type
    // string & operator =  (double val)
    // string & operator += (double val)
    // string @ operator +  (double val, const string &in str)
    // string @ operator +  (const string &in str, double val)
    // string & operator =  (float val)
    // string & operator += (float val)
    // string @ operator +  (float val, const string &in str)
    // string @ operator +  (const string &in str, float val)
    // string & operator =  (int val)
    // string & operator += (int val)
    // string @ operator +  (int val, const string &in str)
    // string @ operator +  (const string &in str, int val)
    // string & operator =  (uint val)
    // string & operator += (uint val)
    // string @ operator +  (uint val, const string &in str)
    // string @ operator +  (const string &in str, uint val)
  }

  // Get a substring of a string
  string @ substring(const string &in str, int start, int length);

  // Find the first occurrance of the substring
  int findFirst(const string &in str, const string &in sub);
  int findFirst(const string &in str, const string &in sub, int startAt)
  
  // Find the last occurrance of the substring
  int findLast(const string &in str, const string &in sub);
  int findLast(const string &in str, const string &in sub, int startAt);
  
  // Find the first character from the set 
  int findFirstOf(const string &in str, const string &in set);
  int findFirstOf(const string &in str, const string &in set, int startAt);
  
  // Find the first character not in the set
  int findFirstNotOf(const string &in str, const string &in set);
  int findFirstNotOf(const string &in str, const string &in set, int startAt);
  
  // Find the last character from the set
  int findLastOf(const string &in str, const string &in set);
  int findLastOf(const string &in str, const string &in set, int startAt);
  
  // Find the last character not in the set
  int findLastNotOf(const string &in str, const string &in set);
  int findLastNotOf(const string &in str, const string &in set, int startAt);
  
  // Split the string into an array of substrings
  string@[]@ split(const string &in str, const string &in delimiter);
  
  // Join an array of strings into a larger string separated by a delimiter
  string@ join(const string@[] &in str, const string &in delimiter);
</pre>




\page doc_addon_dict dictionary object 

<b>Path:</b> /sdk/add_on/scriptdictionary/

The dictionary object maps string values to values or objects of other types. 

Register with <code>RegisterScriptDictionary(asIScriptEngine*)</code>.

\section doc_addon_dict_1 Public C++ interface

\code
class CScriptDictionary
{
public:
  // Memory management
  CScriptDictionary(asIScriptEngine *engine);
  void AddRef();
  void Release();

  // Sets/Gets a variable type value for a key
  void Set(const std::string &key, void *value, int typeId);
  bool Get(const std::string &key, void *value, int typeId) const;

  // Sets/Gets an integer number value for a key
  void Set(const std::string &key, asINT64 &value);
  bool Get(const std::string &key, asINT64 &value) const;

  // Sets/Gets a real number value for a key
  void Set(const std::string &key, double &value);
  bool Get(const std::string &key, double &value) const;

  // Returns true if the key is set
  bool Exists(const std::string &key) const;
  
  // Deletes the key
  void Delete(const std::string &key);
  
  // Deletes all keys
  void DeleteAll();
};
\endcode

\section doc_addon_dict_2 Public script interface

<pre>
  class dictionary
  {
    void set(const string &in key, ? &in value);
    bool get(const string &in value, ? &out value) const;
    
    void set(const string &in key, int64 &in value);
    bool get(const string &in key, int64 &out value) const;
    
    void set(const string &in key, double &in value);
    bool get(const string &in key, double &out value) const;
    
    bool exists(const string &in key) const;
    void delete(const string &in key);
    void deleteAll();
  }
</pre>

\section doc_addon_dict_3 Script example

<pre>
  dictionary dict;
  obj object;
  obj \@handle;
  
  dict.set("one", 1);
  dict.set("object", object);
  dict.set("handle", \@handle);
  
  if( dict.exists("one") )
  {
    bool found = dict.get("handle", \@handle);
    if( found )
    {
      dict.delete("object");
    }
  }
  
  dict.deleteAll();
</pre>





\page doc_addon_file file object 

<b>Path:</b> /sdk/add_on/scriptfile/

This object provides support for reading and writing files.

Register with <code>RegisterScriptFile(asIScriptEngine*)</code>.

If you do not want to provide write access for scripts then you can compile 
the add on with the define AS_WRITE_OPS 0, which will disable support for writing. 
This define can be made in the project settings or directly in the header.


\section doc_addon_file_1 Public C++ interface

\code
class CScriptFile
{
public:
  // Constructor
  CScriptFile();

  // Memory management
  void AddRef();
  void Release();

  // Opening and closing file handles
  // mode = "r" -> open the file for reading
  // mode = "w" -> open the file for writing (overwrites existing files)
  // mode = "a" -> open the file for appending
  int Open(const std::string &filename, const std::string &mode);
  int Close();
  
  // Returns the size of the file
  int GetSize() const;
  
  // Returns true if the end of the file has been reached
  bool IsEOF() const;

  // Reads a specified number of bytes into the string
  int ReadString(unsigned int length, std::string &str);
  
  // Reads to the next new-line character
  int ReadLine(std::string &str);
  
  // Writes a string to the file
  int WriteString(const std::string &str);
  
  // File cursor manipulation
  int GetPos() const;
  int SetPos(int pos);
  int MovePos(int delta);
};
\endcode

\section doc_addon_file_2 Public script interface

<pre>
  class file
  {
    int      open(const string &in filename, const string &in mode);
    int      close();
    int      getSize() const;
    bool     isEndOfFile() const;
    int      readString(uint length, string &out str);
    int      readLine(string &out str);
    int      writeString(const string &in string);
    int      getPos() const;
    int      setPos(int pos);
    int      movePos(int delta);
  }
</pre>

\section doc_addon_file_3 Script example

<pre>
  file f;
  // Open the file in 'read' mode
  if( f.open("file.txt", "r") >= 0 ) 
  {
      // Read the whole file into the string buffer
      string str;
      f.readString(f.getSize(), str); 
      f.close();
  }
</pre>





\page doc_addon_math math functions

<b>Path:</b> /sdk/add_on/scriptmath/

This add-on registers the math functions from the standard C runtime library with the script 
engine. Use <code>RegisterScriptMath(asIScriptEngine*)</code> to perform the registration.

By defining the preprocessor word AS_USE_FLOAT=0, the functions will be registered to take 
and return doubles instead of floats.

\section doc_addon_math_1 Public script interface

<pre>
  float cos(float rad);
  float sin(float rad);
  float tan(float rad);
  float acos(float val);
  float asin(float val);
  float atan(float val);
  float cosh(float rad);
  float sinh(float rad);
  float tanh(float rad);
  float log(float val);
  float log10(float val);
  float pow(float val, float exp);
  float sqrt(float val);
  float ceil(float val);
  float abs(float val);
  float floor(float val);
  float fraction(float val);
</pre>
 


 
 
\page doc_addon_math3d 3D math functions

<b>Path:</b> /sdk/add_on/scriptmath3d/

This add-on registers some value types and functions that permit the scripts to perform 
3D mathematical operations. Use <code>RegisterScriptMath3D(asIScriptEngine*)</code> to
perform the registration.

Currently the only thing registered is the <code>vector3</code> type, representing a 3D vector, 
with basic math operators, such as add, subtract, scalar multiply, equality comparison, etc.

This add-on serves mostly as a sample on how to register a value type. Application
developers will most likely want to register their own math library rather than use 
this add-on as-is. 







\page doc_addon_build Script builder helper

<b>Path:</b> /sdk/add_on/scriptbuilder/

This class is a helper class for loading and building scripts, with a basic pre-processor 
that supports conditional compilation, include directives, and metadata declarations.

If you do not want process metadata then you can compile the add-on with the define 
AS_PROCESS_METADATA 0, which will exclude the code for processing this. This define
can be made in the project settings or directly in the header.


\section doc_addon_build_1 Public C++ interface

\code
class CScriptBuilder
{
public:
  // Load and build a script file from disk
  int BuildScriptFromFile(asIScriptEngine *engine, 
                          const char      *module, 
                          const char      *filename);

  // Build a script file from a memory buffer
  int BuildScriptFromMemory(asIScriptEngine *engine, 
                            const char      *module, 
                            const char      *script, 
                            const char      *sectionname = "");

  // Add a pre-processor define for conditional compilation
  void DefineWord(const char *word);

  // Get metadata declared for class types and interfaces
  const char *GetMetadataStringForType(int typeId);

  // Get metadata declared for functions
  const char *GetMetadataStringForFunc(int funcId);

  // Get metadata declared for global variables
  const char *GetMetadataStringForVar(int varIdx);
};
\endcode

\section doc_addon_build_2 Include directives

Example script with include directive:

<pre>
  \#include "commonfuncs.as"
  
  void main()
  {
    // Call a function from the included file
    CommonFunc();
  }
</pre>


\section doc_addon_build_condition Conditional programming

The builder supports conditional programming through the \#if/\#endif preprocessor directives.
The application may define a word with a call to DefineWord(), then the scripts may check
for this definition in the code in order to include/exclude parts of the code.

This is especially useful when scripts are shared between different binaries, for example, in a 
client/server application.

Example script with conditional compilation:

<pre>
  class CObject
  {
    void Process()
    {
  \#if SERVER
      // Do some server specific processing
  \#endif

  \#if CLIENT
      // Do some client specific processing
  \#endif 

      // Do some common processing
    }
  }
</pre>





\section doc_addon_build_metadata Metadata in scripts

Metadata can be added before script class, interface, function, and global variable 
declarations. The metadata is removed from the script by the builder class and stored
for post build lookup by the type id, function id, or variable index.

Exactly what the metadata looks like is up to the application. The builder class doesn't
impose any rules, except that the metadata should be added between brackets []. After 
the script has been built the application can obtain the metadata strings and interpret
them as it sees fit.

Example script with metadata:

<pre>
  [factory func = CreateOgre,
   editable: myPosition,
   editable: myStrength [10, 100]]
  class COgre
  {
    vector3 myPosition;
    int     myStrength;
  }
  
  [factory]
  COgre \@CreateOgre()
  {
    return \@COgre();
  }
</pre>

Example usage:

\code
CScriptBuilder builder;
int r = builder.BuildScriptFromMemory(engine, "my module", script);
if( r >= 0 )
{
  // Find global variables that have been marked as editable by user
  int count = engine->GetGlobalVarCount("my module");
  for( int n = 0; n < count; n++ )
  {
    string metadata = builder.GetMetadataStringForVar(n);
    if( metadata == "editable" )
    {
      // Show the global variable in a GUI
      ...
    }
  }
}
\endcode




\page doc_addon_autowrap Automatic wrapper functions

<b>Path:</b> /sdk/add_on/autowrapper/aswrappedcall.h

This header file declares some macros and template functions that will let the application developer
automatically generate wrapper functions using the \ref doc_generic "generic calling convention" with 
a simple call to a preprocessor macro. This is useful for those platforms where the native calling 
conventions are not yet supported.

The macros are defined as

\code
#define asDECLARE_FUNCTION_WRAPPER(wrapper_name,func)
#define asDECLARE_FUNCTION_WRAPPERPR(wrapper_name,func,params,rettype)
#define asDECLARE_METHOD_WRAPPER(wrapper_name,cl,func)
#define asDECLARE_METHOD_WRAPPERPR(wrapper_name,cl,func,params,rettype)
\endcode

where wrapper_name is the name of the function that you want to generate, and func is a function pointer 
to the function you want to wrap, cl is the class name, params is the parameter list, and rettype is the return type. 

Unfortunately the template functions needed to perform this generation are quite complex and older
compilers may not be able to handle them. One such example is Microsoft Visual C++ 6, though luckily 
it has no need for them as AngelScript already supports native calling conventions for it.

\section doc_addon_autowrap_1 Example usage

\code
#include "aswrappedcall.h"

// The application function that we want to register
void DoSomething(std::string param1, int param2);

// Generate the wrapper for the function
asDECLARE_FUNCTION_WRAPPER(DoSomething_Generic, DoSomething);

// Registering the wrapper with AngelScript
void RegisterWrapper(asIScriptEngine *engine)
{
  int r;

  r = engine->RegisterGlobalFunction("void DoSomething(string, int)", asFUNCTION(DoSomething_Generic), asCALL_GENERIC); assert( r >= 0 );
}
\endcode






\page doc_addon_clib ANSI C library interface

<b>Path:</b> /sdk/add_on/clib/

This add-on defines a pure C interface, that can be used in those applications that do not
understand C++ code but do understand C, e.g. Delphi, Java, and D.

To compile the AngelScript C library, you need to compile the library source files in sdk/angelscript/source together 
with the source files in sdk/add-on/clib, and link them as a shared dynamic library. In the application that will use 
the AngelScript C library, you'll include the <code>angelscript_c.h</code> header file, instead of the ordinary 
<code>%angelscript.h</code> header file. After that you can use the library much the same way that it's used in C++. 

To find the name of the C functions to call, you normally take the corresponding interface method
and give a prefix according to the following table:

<table border=0 cellspacing=0 cellpadding=0>
<tr><td><b>interface      &nbsp;</b></td><td><b>prefix&nbsp;</b></td></tr>
<tr><td>asIScriptEngine   &nbsp;</td>    <td>asEngine_</td></tr>
<tr><td>asIScriptModule   &nbsp;</td>    <td>asModule_</td></tr>
<tr><td>asIScriptContext  &nbsp;</td>    <td>asContext_</td></tr>
<tr><td>asIScriptGeneric  &nbsp;</td>    <td>asGeneric_</td></tr>
<tr><td>asIScriptArray    &nbsp;</td>    <td>asArray_</td></tr>
<tr><td>asIScriptObject   &nbsp;</td>    <td>asObject_</td></tr>
<tr><td>asIObjectType     &nbsp;</td>    <td>asObjectType_</td></tr>
<tr><td>asIScriptFunction &nbsp;</td>    <td>asScriptFunction_</td></tr>
</table>

All interface methods take the interface pointer as the first parameter when in the C function format, the rest
of the parameters are the same as in the C++ interface. There are a few exceptions though, e.g. all parameters that
take an <code>asSFuncPtr</code> take a normal function pointer in the C function format. 

Example:

\code
#include <stdio.h>
#include <assert.h>
#include "angelscript_c.h"

void MessageCallback(asSMessageInfo *msg, void *);
void PrintSomething();

int main(int argc, char **argv)
{
  int r = 0;

  // Create and initialize the script engine
  asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
  r = asEngine_SetMessageCallback(engine, (asFUNCTION_t)MessageCallback, 0, asCALL_CDECL); assert( r >= 0 );
  r = asEngine_RegisterGlobalFunction(engine, "void print()", (asFUNCTION_t)PrintSomething, asCALL_CDECL); assert( r >= 0 );

  // Execute a simple script
  r = asEngine_ExecuteString(engine, 0, "print()", 0, 0);
  if( r != asEXECUTION_FINISHED )
  {
      printf("Something wen't wrong with the execution\n");
  }
  else
  {
      printf("The script was executed successfully\n");
  }

  // Release the script engine
  asEngine_Release(engine);
  
  return r;
}

void MessageCallback(asSMessageInfo *msg, void *)
{
  const char *msgType = 0;
  if( msg->type == 0 ) msgType = "Error  ";
  if( msg->type == 1 ) msgType = "Warning";
  if( msg->type == 2 ) msgType = "Info   ";

  printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
}

void PrintSomething()
{
  printf("Called from the script\n");
}
\endcode

*/  
