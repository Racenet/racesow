/**

\page doc_script_class_ops Operator overloads

It is possible to define what should be done when an operator is 
used with a script class. While not necessary in most scripts it can
be useful to improve readability of the code.

This is called operator overloading, and is done by implementing 
specific class methods. The compiler will recognize and use these  
methods when it compiles expressions involving the overloaded operators 
and the script class.


\section doc_script_class_unary_ops Prefixed unary operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td></tr>
<tr><td>-</td>             <td>opNeg</td></tr>
<tr><td>~</td>             <td>opCom</td></tr>
<tr><td>++</td>            <td>opPreInc</td></tr>
<tr><td>--</td>            <td>opPreDec</td></tr>
</table>

When the expression <tt><i>op</i> a</tt> is compiled, the compiler will rewrite it as <tt>a.<i>opfunc</i>()</tt> and compile that instead.


\section doc_script_class_unary2_ops Postfixed unary operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td></tr>
<tr><td>++</td>            <td>opPostInc</td></tr>
<tr><td>--</td>            <td>opPostDec</td></tr>
</table>

When the expression <tt>a <i>op</i></tt> is compiled, the compiler will rewrite it as <tt>a.<i>opfunc</i>()</tt> and compile that instead.


\section doc_script_class_cmp_ops Comparison operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td></tr>
<tr><td>==</td>            <td>opEquals</td></tr>
<tr><td>!=</td>            <td>opEquals</td></tr>
<tr><td>&lt;</td>          <td>opCmp</td>   </tr>
<tr><td>&lt;=</td>         <td>opCmp</td>   </tr>
<tr><td>&gt;</td>          <td>opCmp</td>   </tr>
<tr><td>&gt;=</td>         <td>opCmp</td>   </tr>
</table>

The <tt>a == b</tt> expression will be rewritten as <tt>a.opEquals(b)</tt> and <tt>b.opEquals(a)</tt> and 
then the best match will be used. <tt>!=</tt> is treated similarly, except that the result is negated. The 
opEquals method must be implemented to return a <tt>bool</tt> in order to be considered by the compiler.

The comparison operators are rewritten as <tt>a.opCmp(b) <i>op</i> 0</tt> and <tt>0 <i>op</i> b.opCmp(a)</tt> 
and then the best match is used. The opCmp method must be implemented to return a <tt>int</tt> in order to be 
considered by the compiler. If the method argument is to be considered larger than the object then the method 
should return a negative value. If they are supposed to be equal the return value should be 0.

If an equality check is made and the opEquals method is not available the compiler looks for the opCmp method 
instead. So if the opCmp method is available it is really not necesary to implement the opEquals method, except
for optimization reasons.


\section doc_script_class_assign_ops Assignment operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td></tr>
<tr><td>=</td>             <td>opAssign</td>    </tr>
<tr><td>+=</td>            <td>opAddAssign</td> </tr>
<tr><td>-=</td>            <td>opSubAssign</td> </tr>
<tr><td>*=</td>            <td>opMulAssign</td> </tr>
<tr><td>/=</td>            <td>opDivAssign</td> </tr>
<tr><td>\%=</td>           <td>opModAssign</td> </tr>
<tr><td>&amp;=</td>        <td>opAndAssign</td> </tr>
<tr><td>|=</td>            <td>opOrAssign</td>  </tr>
<tr><td>^=</td>            <td>opXorAssign</td> </tr>
<tr><td>&lt;&lt;=</td>     <td>opShlAssign</td> </tr>
<tr><td>&gt;&gt;=</td>     <td>opShrAssign</td> </tr>
<tr><td>&gt;&gt;&gt;=</td> <td>opUShrAssign</td></tr>
</table>

The assignment expressions <tt>a <i>op</i> b</tt> are rewritten as <tt>a.<i>opfunc</i>(b)</tt> and then the
best matching method is used. An assignment operator can for example be implemented like this:

<pre>
  obj@ opAssign(const obj &in other)
  {
    // Do the proper assignment
    ...
    
    // Return a handle to self, so that multiple assignments can be chained
    return this;
  }
</pre>

All script classes have a default assignment operator that does a bitwise copy of the content of the class,
so if that is all you want to do, then there is no need to implement this method. 



\section doc_script_class_binary_ops Binary operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td><td><b>opfunc_r</b></td></tr>
<tr><td>+</td>             <td>opAdd</td>        <td>opAdd_r</td></tr>
<tr><td>-</td>             <td>opSub</td>        <td>opSub_r</td></tr>
<tr><td>*</td>             <td>opMul</td>        <td>opMul_r</td></tr>
<tr><td>/</td>             <td>opDiv</td>        <td>opDiv_r</td></tr>
<tr><td>%</td>             <td>opMod</td>        <td>opMod_r</td></tr>
<tr><td>&amp;</td>         <td>opAnd</td>        <td>opAnd_r</td></tr>
<tr><td>|</td>             <td>opOr</td>         <td>opOr_r</td></tr>
<tr><td>^</td>             <td>opXor</td>        <td>opXor_r</td></tr>
<tr><td>&lt;&lt;</td>      <td>opShl</td>        <td>opShl_r</td></tr>
<tr><td>&gt;&gt;</td>      <td>opShr</td>        <td>opShr_r</td></tr>
<tr><td>&gt;&gt;&gt;</td>  <td>opUShr</td>       <td>opUShr_r</td></tr>
</table>

The expressions with binary operators <tt>a <i>op</i> b</tt> will be rewritten as <tt>a.<i>opfunc</i>(b)</tt> 
and <tt>b.<i>opfunc_r</i>(a)</tt> and then the best match will be used. 



\section doc_script_class_index_op Index operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80><b>op</b></td><td width=120><b>opfunc</b></td></tr>
<tr><td>[]</td>             <td>opIndex</td>
</table>

When the expression <tt>a[i]</tt> is compiled, the compiler will rewrite it as <tt>a.opIndex(i)</tt> and compile that instead.

The index operator can also be formed similarly to \ref doc_script_class_prop "property accessors". The get accessor should then be 
named <tt>get_opIndex</tt> and have one parameter for the indexing. The set accessor should be named <tt>set_opIndex</tt> and have two
parameters, the first is for the indexing, and the second for the new value.

<pre>
  class MyObj
  {
    float get_opIndex(int idx) const       { return 0; }
    void set_opIndex(int idx, float value) { }
  }
</pre>

When the expression <tt>a[i]</tt> is used to retrieve the value, the compiler will rewrite it as <tt>a.get_opIndex(i)</tt>. When 
the expression is used to set the value, the compiler will rewrite it as <tt>a.set_opIndex(i, expr)</tt>.



*/
