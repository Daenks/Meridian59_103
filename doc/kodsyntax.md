Blakod Syntax
--------------

Blakod (kod) statements correspond roughly to similar statements in C. The
main differences are due to kod's dynamic type scheme, and message handler
calls.

Values in kod are 32 bits long; the highest 4 bits act as a tag that determine
the value's type. One bit is used for sign, leaving 27 bits available for
data. Thus, the maximum expressible number in Blakod is 134,217,727 and care
must be taken to ensure calculations in kod do not exceed this amount. Kod can
directly express values of type integer, resource, class, message handler, and
nil. There is no non-integer numerical type. The interpreter uses other tag
values for runtime types such as objects and list elements. Type checking is
done at runtime; thus, expressions with type errors will compile but may cause
runtime errors.

The special value nil (null) is denoted by a dollar sign (`$`). Nil is assigned
to message handler parameters that are not explicitly assigned a value, and it
is also used to mark the end of a list. Nil can be assigned to variables,
returned from message handlers, or tested for equality; it is an error to
perform any other operation on nil.

The special value `self` contains the identifier of the object whose message
handler is being executed. Self is implemented as a property of every object.

Blakod programs are made up of assignment statements, conditional clauses,
loops, calls, and return statements. Kod comments are introduced with the `%`
character, though recently C and C++ style comments were added (`//` `/* */`).
Comments using `%` and `//` extend to the end of the line, and `/* */` comments
can span multiple lines. Unlike C comments, kod multiline comments can be
nested.

Assignments take the form `lvalue = expression`, where `lvalue` is the name of
a property or a local variable. The right hand side is evaulated, and the result
is assigned to the left hand side.

Expressions consist of identifiers, constants, and message handler calls
combined with standard operators.

### Operators
Kod contains the following operators:
* addition `+`
* subtraction `-`
* multiplication `*`
* division `\`
* pre increment `++i` (add one to the variable (e.g. i))
* post increment `i++` (add one to the variable, but use the original value)
* pre decrement `--i` (subtract one from the variable)
* post decrement `i--` (subract one from the variable, but use the original value)
* unary minus `-`
* modulo `MOD`
* bitwise and, or and not `&`, `|` and `~`
* logical and, or and not `AND`, `OR` and `NOT`
* standard relational operators `>`, `<`, `>=`, `<=`,
* inequality operator `<>`
* equality and assignment operator (depending on context) `=`
* local variable ID 'address' operator `*var` (for sending local var ID in C calls)
* class ID operator `&Class` (used to compare/refer to classes)
* message ID operator `@Message` (used to send/refer to messages)

A boolean expression evaluates to 0 if it is false, or nonzero if it is true.
The logical AND and OR operators "short-circuit;" i.e. they only evaluate their
second arguments if necessary.

Operators arranged in precedence from highest to lowest:
* * (local var address operator) @ & (class ID operator)
* ++ -- - (unary minus) NOT ~
* * / MOD
* + -
* < > <= >= = <>
* &
* |
* AND
* OR

### if/elseif/else
The `if` statement performs conditional execution. Its syntax is
```
if test
{
   then-clause
}
```
or
```
if test
{
   then-clause
}
else
{
   else-clause
}
```
or
```
if test
{
   then-clause
}
else if
{
   else-clause
}
else
{
   else-clause
}
```
The braces are required in all cases. Any number of `else if` clauses can be
chained, and the final `else` is optional.

### while loop
A while statement has the syntax:
```
while loop-test
{
   loop-body
}
```
The loop body is evaluated until the loop test becomes false (equal to zero).
Braces are required. Continue and break statements work as in C.

### do/while loop
Similar syntax to a C do/while loop. The statements inside the loop are
executed once, then the condition is checked and if TRUE, execution begins at
the top of the loop. Braces are required, as is the semicolon after the while
expression.
```
do
{
   loop-body
} while loop-test;
```
Continue and break statements work as in C.

### for loop
The kod for loop operates as in C:
```
for (assignments; loop-test; expressions)
{
   loop-body
}
```
Assignments are done at the beginning of the first loop, and all variables
assigned must be declared previously as paramters or local variables. The
loop-test is then checked and if non-zero, the loop-body executes once. The
expressions are then executed and loop-test checked again. Multiple assignments
and expressions can be placed in the for loop header by separating them with
`,`. Any of the three header fields can be blank, and an infinite loop can be
produced as follows:
```
for (;;)
{
   loop body
}
```
Continue and break statements work as in C.

### foreach loop
The foreach loop construct is for use with lists only. Its syntax is:
```
foreach loop-var in list
{
   loop-body
}
```
The loop variable takes on each of the first values of the list during
evaluation of the body of the loop. The break and continue statements can be
used to interrupt loop execution as in C. These statements apply to the
innermost enclosing loop; it is an error for these statements to appear
outside a loop.

### switch/case statement
Kod switch/case statements work as in C. Cases will fallthrough if `break`,
`continue` or `return` are not specified. Only constant values (numbers, kod
constants, class IDs, message IDs, resource IDs) may be used for `case`
expressions, and the `switch` expression can contain any valid kod expression.
The `default` block is executed if none of the cases provide a match.
```
switch (switch_var)
{
   case case_var:
      case_code
      break;
   case case_var2:
      case2_code
      break;
   default:
      default_code
      break;
}
```

### Function calls (message sending and C calls)
Function calls may appear either as expressions, in which case they evaluate
to the return value of the function they call, or as statements, in which case
the return value is ignored.
```
var = First(list);
Send(self,@Message,#parm1=var);
```

### return and propagate statements
There are two kinds of return statements, `return` and `propagate`. One of these
must be the last statement in every message handler. A propagate statement
indicates that execution should proceed to the message handler of the same
name in the closest superclass in the current class's hierarchy, if any. A
return statement indicates that execution should return immediately to the
caller. Return can optionally be followed by an expression whose value is
returned to the caller as the value of the calling expression. If no expression
appears after the return, the value nil is returned to the caller.

### Debug Strings
Debug strings are a special kind of string that is mainly intended for
debugging use. They are specified by a text string inside double quotes in an
expression (i.e. "text"). These are used for message documentation by including
them in the message header, e.g.
```
   Move(distance = 1, direction = 1)
   "These comments describe what the message is for."
   "And they can extend over multiple lines."
   {
      local i, j;

      // code
   }
```
Debug strings can also be used in most of the string handling functions.

