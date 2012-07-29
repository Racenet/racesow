/**


\page doc_script_class_desc Script class overview

With classes the script writer can declare new data types that hold groups
of properties and methods to manipulate them. 

Script classes are reference types, which means that multiple references 
or \ref doc_script_handle "handles" can be held for the same object instance.
The classes uses automatic memory management so the object instances are only
destroyed when the last reference ot the instance is cleared.

The class methods are implemented the same way as \ref doc_global_function "global functions", 
with the addition that the class method can access the class instance properties through either
directly or through the 'this' keyword in the case a local variable has the same name.

<pre>
  // The class declaration
  class MyClass
  {
    // A class method
    void DoSomething()
    {
      // The class properties can be accessed directly
      a *= 2;

      // The declaration of a local variable may hide class properties
      int b = 42;

      // In this case the class property have to be accessed explicitly
      this.b = b;
    }

    // Class properties
    int a;
    int b;
  }
</pre>

A class can implement specific methods to \ref doc_script_class_ops "overload operators".
This can simplify how the object instances are used in expressions, so that it is not
necessary to explicitly name each function, e.g. the opAdd method translates to the + operator.

Another useful feature is the ability to implement \ref doc_script_class_prop "property accessors",
which can be used either to provide virtual properties, i.e. that look like properties but really 
aren't, or to implement specific routines that must be executed everytime a property is accessed.

A script class can also \ref doc_script_class_inheritance "inherit" from other classes, and 
implement \ref doc_global_interface "interfaces".

\todo Explain const methods




\section doc_script_class_construct Class constructors

Class constructors are specific methods that will be used to create new instances
of the class. It is not required for a class to declare constructors, but doing
so may make it easier to use the class as it will not be necessary to first instanciate
the class and then manually set the properties.

The constructors are declared without a return type, and must have the same name as
the class itself. Multiple constructors with different parameter lists can be 
implemented for different forms of initializations.

<pre>
  class MyClass
  {
    // Implement a default constructor
    MyClass()
    {
    }

    // Implement the copy constructor
    MyClass(const MyClass &in other)
    {
      // Copy the value of the other instance
    }

    // Implement other constructors with different parameter lists
    MyClass(int a, string b) {}
    MyClass(float x, float y, float z) {}
  }
</pre>

The copy constructor is a specific constructor that the compiler can use to build
more performatic code when copies of an object must be made. Without the copy 
constructor the compiler will be forced to first instanciate the copy using the 
default constructor, and then copy the attributes with the 
\ref doc_script_class_prop "opAssign" method.

One constructor cannot call another constructor. If you wish to share 
implementations in the constructors you should use a specific method for that.

If a class isn't explicitly declared with any constructor, the compiler will automatically
provide a default constructor for the class. This automatically generated constructor will
simply call the default constructor for all object members, and set all handles to null. 
If a member cannot be initialized with a default constructor, then a compiler error will be
emitted.







\section doc_script_class_destruct Class destructor

It is normally not necessary to implement the class destructor as AngelScript
will by default free up any resources the objects holds when it is destroyed. 
However, there may be situations where a more explicit cleanup routine must be
done as part of the destruction of the object.

The destructor is declared similarly to the constructor, except that it must be
prefixed with the ~ symbol (also known as the bitwise not operator).

<pre>
  class MyClass
  {
    // Implement the destructor if explicit cleanup is needed
    ~MyClass()
    {
      // Perform explicit cleanup here
    }
  }
</pre>

Observe that AngelScript uses automatic memory management with garbage collection
so it may not always be easy to predict when the destructor is executed. AngelScript will also
call the destructor only once, even if the object is resurrected by adding a
reference to it while executing the destructor.

It is not possible to directly invoke the destructor. If you need to be able to 
directly invoke the cleanup, then you should implement a public method for that.



\page doc_script_class_inheritance Inheritance and polymorphism

AngelScript supports single inheritance, where a derived class inherits the 
properties and methods of its base class. Multiple inheritance is not supported,
but polymorphism is supprted by implementing \ref doc_global_interface "interfaces".

All the class methods are virtual, so it is not necessary to specify this manually. 
When a derived class overrides an implementation, it can extend 
the original implementation by specifically calling the base class' method using the
scope resolution operator. When implementing the constructor for a derived class
the constructor for the base class is called using the <code>super</code> keyword. 
If none of the base class' constructors is manually called, the compiler will 
automatically insert a call to the default constructor in the beginning. The base class'
destructor will always be called after the derived class' destructor, so there is no
need to manually do this.

<pre>
  // A derived class
  class MyDerived : MyClass
  {
    // The default constructor
    MyDerived()
    {
      // Calling the non-default constructor of the base class
      super(10);
      
      b = 0;
    }
    
    // Overloading a virtual method
    void DoSomething()
    {
      // Call the base class' implementation
      MyClass::DoSomething();
      
      // Do something more
      b = a;
    }
    
    int b;
  }
</pre>

\todo Show how the polymorphism is used with cast behaviours

\section doc_script_class_inheritance_2 Extra control with final and override

A class can be marked as 'final' to prevent the inheritance of it. This is an optional feature and
mostly used in larger projects where there are many classes and it may be difficult to manually 
control the correct use of all classes. It is also possible to mark individual class methods of a 
class as 'final', in which case it is still possible to inherit from the class, but the finalled
method cannot be overridden.

<pre>
  // A final class that cannot be inherited from
  final class MyFinal
  {
    MyFinal() {}
    void Method() {}
  }
  
  // A class with individual methods finalled
  class MyPartiallyFinal
  {
    // A final method that cannot be overridden
    void Method1() final {}

    // Normal method that can still be overridden by derived class
    void Method2() {}
  }
</pre>

When deriving a class it is possible to tell the compiler that a method is meant to override a method in the 
inherited base class. When this is done and there is no matching method in the base class the compiler will
emit an error, as it knows that something wasn't implemented quite the way it was meant. This is especially
useful to catch errors in large projects where a base class might be modified, but the derived classes was 
forgotten.

<pre>
  class MyBase
  {
    void Method() {}
    void Method(int) {}
  }
  
  class MyDerived : MyBase
  {
    void Method() override {}      // OK. The method is overriding a method in the base class
    void Method(float) override {} // Not OK. The method isn't overriding a method in base class
  }
</pre>






\page doc_script_class_private Private class members

Class members can be declared as private if you wish do not intend for them to be accessed 
from outside the public class methods. This can be useful in large programs where you to 
avoid programmer errors where properties or methods are inappropriately used.

<pre>
  // A class with private members
  class MyPrivate
  {
    // The following are public members
    void PublicFunc()
    {
      // The class can access its own private members
      PrivateProp = 0; // OK
      PrivateFunc();   // OK
    }    
 
    int PublicProp;

    // The following are private members
    private void PrivateFunc()
    {
    } 

    private int PrivateProp;
  }

  void GlobalFunc()
  {
    MyPrivate obj;

    // Public members can be accessed normally
    obj.PublicProp = 0;  // OK
    obj.PublicFunc();    // OK

    // Accessing private members will give a compiler error
    obj.PrivateProp = 0; // Error
    obj.PrivateFunc();   // Error
  }
</pre>



*/
