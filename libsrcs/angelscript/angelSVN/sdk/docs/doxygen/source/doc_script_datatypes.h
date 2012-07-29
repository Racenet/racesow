/**

\page doc_datatypes Data types

Note that the host application may add types specific to that application, refer to the application's manual for more information.

 - \subpage doc_datatypes_primitives 
 - \subpage doc_datatypes_arrays
 - \subpage doc_datatypes_obj
 - \subpage doc_datatypes_strings
 - \subpage doc_datatypes_funcptr

\page doc_datatypes_primitives Primitives

\section void void

<code>void</code> is not really a data type, more like lack of data type. It can only be used to tell the compiler that a function doesn't return any data.


\section bool bool

<code>bool</code> is a boolean type with only two
possible values: <code>true</code> or
<code>false</code>. The keywords
<code>true</code> and
<code>false</code> are constants of type
<code>bool</code> that can be used as such in expressions.


\section int Integer numbers

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100><b>type</b></td><td width=200><b>min value</b></td><td><b>max value</b></td></tr>
<tr><td><code>int8  </code></td><td>                      -128</td><td>                       127</td></tr>
<tr><td><code>int16 </code></td><td>                   -32,768</td><td>                    32,767</td></tr>
<tr><td><code>int   </code></td><td>            -2,147,483,648</td><td>             2,147,483,647</td></tr>
<tr><td><code>int64 </code></td><td>-9,223,372,036,854,775,808</td><td> 9,223,372,036,854,775,807</td></tr>
<tr><td><code>uint8 </code></td><td>                         0</td><td>                       255</td></tr>
<tr><td><code>uint16</code></td><td>                         0</td><td>                    65,535</td></tr>
<tr><td><code>uint  </code></td><td>                         0</td><td>             4,294,967,295</td></tr>
<tr><td><code>uint64</code></td><td>                         0</td><td>18,446,744,073,709,551,615</td></tr>
</table>

As the scripting engine has been optimized for 32 bit datatypes, using the smaller variants is only recommended for accessing application specified variables. For local variables it is better to use the 32 bit variant.

<code>int32</code> is an alias for <code>int</code>, and <code>uint32</code> is an alias for <code>uint</code>.




\section real Real numbers

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100><b>type</b></td><td width=230><b>range of values</b></td><td width=230><b>smallest positive value</b></td> <td><b>maximum digits</b></td></tr>
<tr><td><code>float </code></td> <td>+/- 3.402823466e+38        </td> <td>1.175494351e-38        </td> <td>6 </td></tr>
<tr><td><code>double</code></td> <td>+/- 1.7976931348623158e+308</td> <td>2.2250738585072014e-308</td> <td>15</td></tr>
</table>

Rounding errors will occur if more digits than the maximum number of digits are used.

<b>Curiousity</b>: Real numbers may also have the additional values of positive and negative 0 or 
infinite, and NaN (Not-a-Number). For <code>float</code> NaN is represented by the 32 bit data word 0x7fc00000.








\page doc_datatypes_arrays Arrays

<b>Observe:</b> Arrays are only available in the scripts if the application registers the support for them. The syntax
for using arrays may differ for the application you're working with so consult the application's manual
for more details.

It is possible to declare array variables by appending the [] brackets to the type. When declaring a 
variable with a type modifier, i.e. [], the type modifier affects the type of all variables in the list.
Example:

<pre>
  int[] a, b, c;
</pre>

<code>a</code>, <code>b</code>, and <code>c</code> are now arrays of integers.

When declaring arrays it is possible to define the initial size of the array by passing the length as
a parameter to the constructor. The elements can also be individually initialized by specifying an 
initialization list. Example:

<pre>
  int[] a;           // A zero-length array of integers
  int[] b(3);        // An array of integers with 3 elements
  int[] c(3, 1);     // An array of integers with 3 elements, all set to 1 by default
  int[] d = {,3,4,}; // An array of integers with 4 elements, where
                     // the second and third elements are initialized
</pre>

Multidimensional arrays are supported as arrays of arrays, for example:

<pre>
  int[][] a;                // An empty array of arrays of integers
  int[][] b = {{1,2},{3,4}} // A 2 by 2 array with initialized values
  int[][] c(10, int[](10)); // A 10 by 10 array of integers with uninitialized values
</pre>

Each element in the array is accessed with the indexing operator. The indices are zero based, i.e the
range of valid indices are from 0 to length - 1.

<pre>
  a[0] = some_value;
</pre>

The standard array implementation also has the following methods:

<pre>
  // Adding and removing elements
  void insertAt(uint index, const T& in);
  void removeAt(uint index);
  void insertLast(const T& in);
  void removeLast();

  // Determine size of array
  uint length() const;
  void resize(uint);

  // Sort the array
  void sortAsc();
  void sortAsc(uint index, uint count);
  void sortDesc();
  void sortDesc(uint index, uint count);
  void reverse();

  // Find elements
  int  find(const T& in);
  int  find(uint index, const T& in);
</pre>










\page doc_datatypes_obj Objects and handles

\section objects Objects

There are two forms of objects, reference types and value types. 

Value types behave much like the primitive types, in that they are allocated on the stack and deallocated 
when the variable goes out of scope. Only the application can register these types, so you need to check
with the application's documentation for more information about the registered types.

Reference types are allocated on the memory heap, and may outlive the initial variable that allocates
them if another reference to the instance is kept. All \ref doc_global_class "script declared classes" are reference types. 
\ref doc_global_interface "Interfaces" are a special form of reference types, that cannot be instanciated, but can be used to access
the objects that implement the interfaces without knowing exactly what type of object it is.

<pre>
  obj o;      // An object is instanciated
  o = obj();  // A temporary instance is created whose 
              // value is assigned to the variable
</pre>



\section handles Object handles

Object handles are a special type that can be used to hold references to other objects. When calling methods or 
accessing properties on a variable that is an object handle you will be accessing the actual object that the 
handle references, just as if it was an alias. Note that unless initialized with the handle of an object, the 
handle is <code>null</code>.

<pre>
  obj o;
  obj@ a;           // a is initialized to null
  obj@ b = \@o;      // b holds a reference to o

  b.ModifyMe();     // The method modifies the original object

  if( a is null )   // Verify if the object points to an object
  {
    \@a = \@b;        // Make a hold a reference to the same object as b
  }
</pre>

Not all types allow a handle to be taken. Neither of the primitive types can have handles, and there may exist some object types that do not allow handles. Which objects allow handles or not, are up to the application that registers them.

Object handle and array type modifiers can be combined to form handles to arrays, or arrays of handles, etc.

\see \ref doc_script_handle











\page doc_datatypes_strings Strings

<b>Observe:</b> Strings are only available in the scripts if the application registers the support for them. The syntax
for using strings may differ for the application you're working with so consult the application's manual
for more details.

Strings hold an array of bytes or 16bit words depending on the application settings. 
Normally they are used to store text but can really store any kind of binary data.

There are two types of string constants supported in the AngelScript
language, the normal quoted string, and the documentation strings,
called heredoc strings.

The normal strings are written between double quotation marks (<code>"</code>) or single quotation marks (<code>'</code>).
Inside the constant strings some escape sequences can be used to write exact
byte values that might not be possible to write in your normal editor.


<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=100 valign=top><b>sequence</b></td>
<td valign=top width=100><b>value</b></td>
<td valign=top><b>description</b></td></tr>

<tr><td width=80 valign=top><code>\\0</code>&nbsp;  </td>
<td valign=top width=50>0</td>
<td valign=top>null character</td></tr>
<tr><td width=80 valign=top><code>\\\\</code>&nbsp;  </td>
<td valign=top width=50>92</td>
<td valign=top>back-slash</td></tr>
<tr><td width=80 valign=top><code>\\'</code>&nbsp;  </td>
<td valign=top width=50>39</td>
<td valign=top>single quotation mark (apostrophe)</td></tr>
<tr><td width=80 valign=top><code>\\"</code>&nbsp;  </td>
<td valign=top width=50>34</td>
<td valign=top>double quotation mark</td></tr>
<tr><td width=80 valign=top><code>\\n</code>&nbsp;  </td>
<td valign=top width=50>10</td>
<td valign=top>new line feed</td></tr>
<tr><td width=80 valign=top><code>\\r</code>&nbsp;  </td>
<td valign=top width=50>13</td>
<td valign=top>carriage return</td></tr>
<tr><td width=80 valign=top><code>\\t</code>&nbsp;  </td>
<td valign=top width=50>9</td>
<td valign=top>tab character</td></tr>
<tr><td width=80 valign=top><code>\\xFFFF</code>&nbsp;</td>
<td valign=top width=50>0xFFFF</td>
<td valign=top>FFFF should be exchanged for a 1 to 4 digit hexadecimal number representing the value wanted. If the application uses 8bit strings then only values up to 255 is accepted.</td></tr>
<tr><td width=80 valign=top><code>\\uFFFF</code>&nbsp;</td>
<td valign=top width=50>0xFFFF</td>
<td valign=top>FFFF should be exchanged for the hexadecimal number representing the unicode code point</td></tr>
<tr><td width=80 valign=top><code>\\UFFFFFFFF</code>&nbsp;</td>
<td valign=top width=50>0xFFFFFFFF</td>
<td valign=top>FFFFFFFF should be exchanged for the hexadecimal number representing the unicode code point</td></tr>
</table>


<pre>
  string str1 = "This is a string with \"escape sequences\".";
  string str2 = 'If single quotes are used then double quotes can be included without "escape sequences".';
</pre>


The heredoc strings are designed for inclusion of large portions of text
without processing of escape sequences. A heredoc string is surrounded by
triple double-quotation marks (<code>"""</code>), and can span multiple lines
of code. If the characters following the start of the string until the first
linebreak only contains white space, it is automatically removed by the
compiler. Likewise if the characters following the last line break until the
end of the string only contains white space this is also remove, including the
linebreak.


<pre>
  string str = """
  This is some text without "escape sequences". This is some text.
  This is some text. This is some text. This is some text. This is
  some text. This is some text. This is some text. This is some
  text. This is some text. This is some text. This is some text.
  This is some text.
  """;
</pre>

If more than one string constants are written in sequence with only whitespace or
comments between them the compiler will concatenate them into one constant.

<pre>
  string str = "First line.\n"
               "Second line.\n"
               "Third line.\n";
</pre>

The escape sequences \\u and \\U will add the specified unicode code point as a
UTF-8 or UTF-16 encoded sequence depending on the application settings. Only valid unicode 5.1 
code points are accepted, i.e. code points between U+D800 and U+DFFF (reserved for surrogate pairs) 
or above U+10FFFF are not accepted.

The standard string implementation also comes with the following methods:

<pre>
  // Returns the length of the string
  uint length() const;
  
  // Assignment and concatenation
  string &opAssign(const string &in other);
  string &opAddAssign(const string &in other);
  string  opAdd(const string &in right) const;
  
  // Access individual characters
  uint8       &opIndex(uint);
  const uint8 &opIndex(uint) const;
  
  // Comparison operators
  bool opEquals(const string &in right) const;
  int  opCmp(const string &in right) const;
  
  // Substring
  string substr(uint start = 0, int count = -1) const;
  array<string>@ split(const string &in delimiter) const;
  
  // Search
  int findFirst(const string &in str, uint start = 0) const;
  int findLast(const string &in str, int start = -1) const;
  
  // Automatic conversion from primitive types to string type
  string &opAssign(double val);
  string &opAddAssign(double val);
  string  opAdd(double val) const;
  string  opAdd_r(double val) const;
  
  string &opAssign(int val);
  string &opAddAssign(int val);
  string  opAdd(int val) const;
  string  opAdd_r(int val) const;
  
  string &opAssign(uint val);
  string &opAddAssign(uint val);
  string  opAdd(uint val) const;
  string  opAdd_r(uint val) const;

  string &opAssign(bool val);
  string &opAddAssign(bool val);
  string  opAdd(bool val) const;
  string  opAdd_r(bool val) const;
</pre>

There is also a standard global function joining an array of strings into a single string with delimeter.

<pre>
  string join(const array<string> &in arr, const string &in delimiter);
</pre>







\page doc_datatypes_funcptr Function pointers

A function pointer is a data type that can be dynamically set to point to a global function that has
a matching function signature as that defined by the variable declaration. Function pointers are commonly
used for callbacks, i.e. where a piece of code must be able to call back to some code based on some 
conditions, but the code that needs to be called is not known at compile time.

To use function pointers it is first necessary to \ref doc_global_funcdef "define the function signature" 
that will be used at the global scope. Once that is done the variables can be declared using that definition.

Here's an example that shows the syntax for using function pointes:

<pre>
  // Define a function signature for the function pointer
  funcdef bool CALLBACK(int, int);

  // An example function that shows how to use this
  void main()
  {
    // Declare a function pointer, and set it 
    // to point to the myCompare function.
    CALLBACK \@func = \@myCompare;

    // The function pointer can be compared with the 'is' operator
    if( func is null )
    {
      print("The function pointer is null\n");
      return;
    }

    // Call the function through the pointer, just as if it was a normal function
    if( func(1, 2) )
    {
      print("The function returned true\n");
    }
    else
    {
      print("The function returned false\n");
    }
  }

  // This function matches the CALLBACK definition, since it has 
  // the same return type and parameter types.
  bool myCompare(int a, int b)
  {
    return a > b;
  }
</pre>


*/
