Blakod (a.k.a. kod)
--------------

Blakod is the language that BlakSton (Blakserv) uses to define objects in its
game world. BlakSton contains a byte compiler from Blakod to an simple
intermediate language, and an interpreter for the intermediate language. This
document informally describes the syntax and semantics of Blakod.

Blakod (kod) is an object-oriented language that uses message passing as a
primary means of flow control. An object consists of some private data, called
properties, and a set of methods (which we refer to as message handlers) for
observing and manipulating this private data. The syntax of the language is
much like that of C or Pascal.

Because kod is a special-purpose language, meant to describe objects for use
in role-playing games, there is a close relationship between the kod written
for a game and the runtime system in Blakserv. Kod code calls built-in C
functions (equivalent to a standard library) in the server when an operation
would be too slow or complicated in kod, or where the operation requires
communication with other parts of the server, such as sending messages to
clients running on user machines.

A kod code file contains definitions for one class. Each class definition
consists of six parts as shown below. The class header lists the name of the
class, and optionally the name of a single superclass. Only single inheritance
is allowed.

### Basic format of a Blakod class
```
Classname is Superclass // Class header

constants: // Constant block

   include constants.khd

   SECONDS_PER_HOUR = 3600

resources: // Resource block

   include classname.lkod

   filename_rsc = picture.bgf
   string_rsc = "Text goes here"
   string_rsc = de "hier schreiben" // German translation

classvars: // Class variable block

   vrBitmap = filename_rsc
   viTreasure_type = TID_NONE

properties: // Property block

   piWeight = 10
   poBrain = $

messages: // Message block

   Move(distance = 1, direction = 1)
   "This message handler moves this object in some way."
   "And these comments can extend over multiple lines."
   {
      local i, j;

      // code
   }

end
```

### Constants block
The constants block lists identifiers to be used as abbreviations for simple
constant expressions. Such an identifier evaluates to the right hand side of
the assignment given in the constant block wherever it appears. The constants
block can also contain include compiler directives that can be used to include
a set of constant definitions from other files. All constants should be in
upper-case with underscores separating words, and should not be too long as
they are used by name in the code.

### [Resources block](./kodresource.md)
Identifiers in the resources block reference filenames and strings. A class's
resources are placed in a resource file (.rsc) during compilation, and this
file is sent to clients. Each resource is assigned a unique number during
compilation, and it is these numbers that the server uses to refer to files
and strings in messages to the client. Resources can also have a two-letter
language specifier to allow the client and server to choose which language to
display for each resource. If none is present, the resource will be assigned
the English language specifier. One resource can only have one string for
each language (183 available).

In kod, resources can only be passed as parameters or appear by themselves
on the right hand side of assignments; they cannot be assigned to or appear in
compound expressions. The resources block can also contain the include compiler
directive, to include an additional file containing resources. The included
filename has no limit on name or extension, but by convention should be the
name of the class with the extension .lkod. Resource names should be in all
lower-case, with underscores separating words, and should not be too long as
they are used by name in the code.

### Class variables block
A class variable is a piece of read-only, per-class data. Class variables are
inherited by subclasses just as properties are. The main use of class variables
is to save space in the object database; data that doesn't vary across
instances of a class should be put in class variables to avoid allocating space
in each object for the data. Under rare circumstances classvars can be set in
code with a `SetClassVar()` C call, but this should be avoided unless absolutely
necessary. Classvars should start with a lower-case v, followed by a letter(s)
denoting the expected type (i for integer, b for boolean, r for resource, c for
class) followed by a meaningful but short CamelCase name with optional
underscores.

### Properties block
Properties are the equivalent of protected class data in C++. A class inherits
all the properties of its direct and indirect superclasses; these properties
are all accessible within the class. Properties of a superclass can appear in
the subclass's property section in order to override the superclass's values
for these properties.

A subclass can overload a superclass's class variable by declaring a property
with the same name. The subclass can then use the property as read-write data
just like any other property. In this way, a class high in the hierarchy can
declare read-only data to save space in most objects, while subclasses can
still write to the location if they need to (though instances of the subclass
will of course require extra space to hold the property). Properties should
start with a lower-case p, followed by a letter(s) denoting the expected type
(i for integer, b for boolean, o for object, r for resource, h for hashtable,
l for list, s for string, c for class, t for timer, rm for roomdata) followed
by a meaningful but short CamelCase name with optional underscores.

Properties and class variables can be initialized to any constant expression.
If no expression is given, they are initialized to null (which in kod is
written as `$`) but the `$` can be added for clarity.

### Messages block
The message block contains the class's message handlers. Each message handler
begins with a header that lists the handler's name and parameters. Parameters
are matched by name, so that calls of a handler need not assign values to all
of the parameters listed in the handler's header. The header can list default
values for any of its parameters; a default value is bound to its parameter
when a call does not list the parameter. The header is optionally followed by a
comment, describing the message handler. Administrators can view these comments
in the game. A message handler's body contains a sequence of statements, each
ending with a semi-colon.

### Local variables
The first statement optionally declares a list of local variables used by the
handler. Local variables are automatically initialized to `$`. Local variables
are also unique in that the `*` operator can be used in some C calls (written
as `*iLocalVar`) to assign a value to the variable from the C call itself. The
local variable does not need to have a kod-assigned value to be used in this
way. Parameters are treated as local variables inside the message. Local vars
should start with a lower-case letter(s) denoting the expected type (i for
integer, b for boolean, o for object, r for resource, h for hashtable, l for
list, s for string, c for class, t for timer, rm for roomdata) followed by a
meaningful but short CamelCase name with optional underscores.

### Inheritance
A class also inherits its superclass's message handlers. When an object's
class hierarchy contains more than one message handler of the same name, the
handler for the lowest class in the object's hierarchy is called first. This
handler can propagate the message to the next handler up the hierarchy, or it
can return a value, in which case the other handlers are not called.

A special message handler called `Constructor` is called when an object is
created. Right before an object's `Constructor` is invoked, its default property
values are set in descending class order, starting with the top of the object's
class hierarchy and ending with the object's actual class. This is the reverse
order of the message handler call sequence, allowing subclasses to override
property values of superclasses. After Object's `Constructor` runs, the
messages `Constructed` and `DefaultValues` are called for any processing that
needs to be done after the object is fully constructed.

Class and message handler names have global scope. Each class name must be
globally unique, and message handler names must be unique within a class. The
scope of all identifiers appearing on the left hand sides of assignments in the
constant, resource, property, and classvar blocks is the class in which the
blocks appear. Parameter names and local variables appearing in a message
handler have scope restricted to that message handler.
