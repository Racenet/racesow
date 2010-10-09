/**

\page doc_register_prop Registering global properties

By registering global properties with the script engine you can allow the scripts to 
inspect and/or modify variables within the application directly, without the need to 
write special functions to do this.

To register the property, you just need to call the \ref asIScriptEngine::RegisterGlobalProperty 
"RegisterGlobalProperty" method, passing the declaration and a pointer to the property. 
Remember that the registered property must stay alive as long as its registration is 
valid in the engine.

\code
// Variables that should be accessible through the script.
int      g_number       = 0;
CObject *g_object       = 0;
Vector3  g_vector       = {0,0,0};
bool     g_readOnlyFlag = false;

// A function to register the global properties. 
void RegisterProperties(asIScriptEngine *engine)
{
    int r;
    
    // Register a primitive property that can be read and written to from the script.
    r = engine->RegisterGlobalProperty("int g_number", &g_number); assert( r >= 0 );
    
    // Register variable where the script can store a handle to a CObject type. 
    // Assumes that the CObject type has been registered with the engine already as a reference type.
    r = engine->RegisterGlobalProperty("CObject @g_object", &g_object); assert( r >= 0 );
    
    // Register a 3D vector variable.
    // Assumes that the Vector3 type has been registered already as a value type.
    r = engine->RegisterGlobalProperty("Vector3 g_vector", &g_vector); assert( r >= 0 );
    
    // Register a boolean flag that can be read, but not modified by the script.
    r = engine->RegisterGlobalProperty("const bool g_readOnlyFlag", &g_readOnlyFlag); assert( r >= 0 );
}
\endcode

It is also possible to expose properties through \ref doc_script_class_prop "property accessors", which are a pair of functions 
with the prefixes get_ and set_ for getting and setting the property value. These functions should be registered with registered 
with \ref doc_register_func "RegisterGlobalFunction". This is especially useful when the offset of the property cannot be 
determined, or if the type of the property is not registered in the script and some translation must occur, i.e. from char* to string. 

\see \ref doc_register_type 

*/
