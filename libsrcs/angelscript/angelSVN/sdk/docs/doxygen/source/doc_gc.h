/**

\page doc_gc Garbage collection

Though AngelScript uses reference counting for memory management, there is still need for a garbage collector to 
take care of the few cases where circular referencing between objects prevents the reference counter from reaching zero.
By default AngelScript automatically runs a few incremental steps of the garbage collector everytime a new garbage 
collected object is created, this allows the destruction of garbage in an automatic way, yet doesn't waste time or 
disrupts the responsiveness of the application. 

The automatic execution may not always be enough to clean up the garbage, for example, if the scripts create a lot of
garbage with circular references. In this case the application may need to manually invoke the garbage collector from
time to time. For this reason it is recommended that the application monitor the statistics for the garbage collector 
and adjust the frequency of the manual calls as necessary. The statistics is obtained through a call to 
\ref asIScriptEngine::GetGCStatistics "GetGCStatistics", which returns the number of objects currently known to the 
garbage collector as well as the number of objects that have been destroyed and the number of object that have been 
detected as garbage with circular references. 

The garbage collector implemented in AngelScript is incremental, so it can be executed for short periods of time without
requiring the entire application to halt. For this reason, it is recommended that one or more calls to \ref 
asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_ONE_STEP) is made at least once during the normal event processing.
The number of calls that should be made depends on how much garbage is created.

If the scripts produce a lot of garbage but only a low number of garbage with circular references, the application can make a 
call to \ref asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_FULL_CYCLE | \ref asGC_DESTROY_GARBAGE), which will 
only destroy the known garbage without trying to detect circular references. This call is relatively fast as the garbage 
collector only has to make a trivial local check to determine if an object is garbage without circular references.

Finally, if the application goes into a state where responsiveness is not so critical, it might be a good idea to do a full
cycle on the garbage collector, thus cleaning up all garbage at once. To do this, call \ref 
asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_FULL_CYCLE).

Should the automatic garbage collections not be desired, e.g. in critical inner loops where maximum performance is
needed, it can easily be turned off with a call to \ref asIScriptEngine::SetEngineProperty "SetEngineProperty"(\ref asEP_AUTO_GARBAGE_COLLECT, false).

\see \ref doc_memory



*/
