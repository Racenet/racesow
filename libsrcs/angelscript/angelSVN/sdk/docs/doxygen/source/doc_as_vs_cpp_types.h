/**

\page doc_as_vs_cpp_types Datatypes in AngelScript and C++


\section doc_as_vs_cpp_types_1 Primitives

Primitives in AngelScript have direct matches in C++.

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100><b>AngelScript</b></td><td width=150><b>C++</b></td><td width=100><b>Size (bits)</b></tr>
<tr><td>void  </td><td>void              </td><td>0     </td></tr>
<tr><td>int8  </td><td>signed char       </td><td>8     </td></tr>
<tr><td>int16 </td><td>signed short      </td><td>16    </td></tr>
<tr><td>int   </td><td>signed long (*)   </td><td>32    </td></tr>
<tr><td>int64 </td><td>signed long long  </td><td>64    </td></tr>
<tr><td>uint8 </td><td>unsigned char     </td><td>8     </td></tr>
<tr><td>uint16</td><td>unsigned short    </td><td>16    </td></tr>
<tr><td>uint  </td><td>unsigned long (*) </td><td>32    </td></tr>
<tr><td>uint64</td><td>unsigned long long</td><td>64    </td></tr>
<tr><td>float </td><td>float             </td><td>32    </td></tr>
<tr><td>double</td><td>double            </td><td>64    </td></tr>
<tr><td>bool  </td><td>bool              </td><td>8 (**)</td></tr>
</table>


%*) If the target CPU is 32 bit, the C++ type long is 32 bit, but on 64 bit CPUs
   it is commonly 64 bit, but in AngelScript an int is always 32 bits.

%**) On 32 bit PowerPC platforms the bool type commonly have the size of 32 bit,
   when compiled on such platforms AngelScript also uses 32 bits for the bool type

\section doc_as_vs_cpp_types_2 Arrays

The AngelScript arrays are not directly matched by C++ arrays. The arrays
are stored in an special object, accessed through the \ref asIScriptArray interface. 
Thus you can normally not directly exchange a script with a C++ function expecting 
a C++ array, or vice versa. Nor can the application register C++ arrays as properties 
and expect AngelScript to be able to understand them.

It is however possible to override AngelScript's built-in array objects
with application specified objects, on a per array type basis.

\section doc_as_vs_cpp_types_3 Object handles

The AngelScript object handles are reference counted pointers to objects.
This means that for object handles to work, the object must have some way of
counting references, for example an AddRef/Release method pair.

When AngelScript passes an object handle by value to a function it
increases the reference count to count for the argument instance, thus the
function is responsible for releasing the reference once it is finished with
it. In the same manner AngelScript expects any handle returned from a function
to already have the reference accounted for.

However, when registering functions/methods with AngelScript the
application can tell the library that it should automatically take care of
releasing the handle references once a function return, likewise for returned
handles. This is done by adding a + sign to the \@ type modifier. When doing
this an object handle can be safely passed to a C++ function that expects a
normal pointer, but don't release it afterwards.

\see \ref doc_obj_handle

\section doc_as_vs_cpp_types_4 Parameter references

Because AngelScript needs to guarantee validity of pointers at all times,
it doesn't always pass references to the true object to the function
parameter. Instead it creates a copy of the object, whose reference is passed
to the function, and if the reference is marked to return a value, the clone
is copied back to the original object (if it still exists) once the function
returns.

Because of this, AngelScript's parameter references are mostly compatible
with C++ references, or pointers, except that the address normally shouldn't be
stored for later use, since the object may be destroyed once the function returns.

If it is necessary to store the address of the object, then object handles
should be used instead.

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100 valign=top><b>Reference</b></td><td valign=top><b>Description</b></td></tr>
<tr><td valign=top>&in</td><td>A copy of the value is always taken and the 
reference to the copy is passed to the function. For script functions this is not 
useful, but it is maintained for compatibility with application registered functions.</td></tr>
<tr><td valign=top>const &in</td><td>If the life time of the value can be 
guaranteed to be valid during the execution of the function, the reference to the 
true object is passed to the function, otherwise a copy is made.</td></tr>
<tr><td valign=top>&out</td><td>A reference to an unitialized value is passed 
to the function. When the function returns the value is copied to the true reference. 
The argument expression is evaluated only after the function call. This is the best 
way to have functions return multiple values.</td></tr>
<tr><td valign=top>const &out</td><td>Useless as the function wouldn't be able 
to modify the value.</td></tr>
<tr><td valign=top>&inout</td><td>The true reference is always passed to the 
function. If the life time of the value cannot be guaranteed, a compile time error 
is generated and the script writer will have to copy the value to a local variable 
first. Objects that support object handles are best suitable for this type as they 
can always be guaranteed.</td></tr>
<tr><td valign=top>const &inout</td><td>The reference cannot be changed by the 
function.</td></tr>
</table>

If the application wants parameter references that work like they do in C++, 
then the engine property asEP_ALLOW_UNSAFE_REFERENCES can be set. When this is done, 
the parameter references can be declared without the <code>in</code>, <code>out</code>, 
or <code>inout</code>. The parameter references declared without any of the keywords 
will always pass the address to the original value, and will not have any restrictions 
to the expressions that can be used. The parameter references with the <code>in</code> 
and <code>out</code> keywords will still work like before, but the references with the 
<code>inout</code> keyword will have the restrictions removed so that they work just 
like normal C++ references.

The application writer and script writer has to be aware that it is possible to 
write scripts that access invalid references when the library is compiled in this 
mode, just like it is possible to do so in C++.



*/
