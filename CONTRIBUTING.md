# Contributing

## Issues and Bug Reporting

Please submit any issues, bug reports or smaller feature requests to the
GitHub issues page. Any longer feature requests or gameplay-related
discussion should be directed to the [forums](http://openmeridian.org/forums)
or the IRC channel (irc.esper.net, channel #Meridian59).

## Pull Requests

Pull Requests should be directed to the develop branch only, except where
the PR contains a bug fix required immediately on production servers in
which case the PR should be directed to the master branch.

#### Pull Request Content
PRs should, where possible, address a single issue or modify a single area
or system to expedite the code review and testing process. Exceptions can
be made (especially for bug fixes or small inconsequential changes) but if
the contents can be reasonably split into multiple PRs and the resulting
PRs would be easier to review and test, it should be done. If the PR does
combine several small issues, they should be put in separate commits.

PRs should contain a short description (bullet points are fine) detailing
the purpose of the PR and any changes made. These descriptions will be
condensed into patch notes, and PRs with no description will not be considered.
Descriptions should also not be too long - if the PR requires multiple pages
of text to describe it, it probably needs to be split into two or more PRs.

Commit messages should be short and meaningful and ideally contain a
<72 character title followed by a short description of the changes made.
Small commits containing a typo fix should be combined into larger commits
in a multi-commit PR. All commits need to follow the coding guidelines
where applicable, and not contain any errors or unnecessary added files.

Contributors should test and verify their submissions are complete and
working before opening a PR, unless the purpose of the PR is to obtain
help or further guidance on how to proceed. Contributors should also ensure
their repository and branches stay up-to-date with the current code.

Due to the time required to review and test large numbers of submissions,
PRs that frequently ignore these rules won't be considered.

## C/C++ Coding Standards

0. All indentations should be 3 spaces, no tabs. Line endings are Unix-style (LF).
0. Try to keep code width under 80 characters if possible.
0. Opening braces on if/while/for statements should be placed on the next line
on their own, as should closing braces.
0. Don't use typedefed names for variables.
0. Add comment headers for non-obvious functions, and comment any non-obvious
code statements.

## Kod Coding Standards

#### Spacing and Line Endings

0. All indentations should be 3 spaces, no tabs. Line endings are Unix-style (LF).
0. Multi-line statements should indent once for `if` statements and equations,
and twice (6 spaces only) for C calls.
0. Multi-line equations should be broken up at operators, with the operator
placed on the next line.
0. Blank lines should be used to break up blocks of code at logical points,
e.g. after an if statement, and before a control flow change (i.e. break,
continue, return, propagate).
0. Code should be formatted to 80-character width as much as possible; 80-90
characters is acceptable if necessary for ease of reading but 90+ is not.

Good:
```
Send(self,@SendAttackOutOfRangeMessage,#what=oFinalTarget,
      #use_weapon=use_weapon,#stroke_obj=stroke_obj);
```
Bad:
```
Send(self,@SendAttackOutOfRangeMessage,#what=oFinalTarget,
                                       #use_weapon=use_weapon,
                                       #stroke_obj=stroke_obj);
```

#### Comments

0. All code that is not self-explanatory should come with comments describing
briefly the purpose of the code.
0. All messages should contain a string comment in the header describing the
purpose of the message.
0. Use C++-style // comments instead of kod-style % comments, to allow the code
to be parsed correctly by code-document generators such as doxygen.
0. Use C-style /* */ comments for multi-line comments (can be nested in kod).

### Keywords and Braces

0. All keywords (if, else, while, return etc.) should be in lower case.
0. All C calls (Send, Nth, First, Bound etc.) should start with an upper-case
letter, and use camelcase (e.g. CreateTimer) where applicable.
0. Opening braces on messages and if/while/for/foreach statements should be
placed on the next line on their own, as should closing braces.
0. Only use one statement per line, and break up multiple AND/OR expressions
in `if` statements logically. If an `if` or `while` statement has only two
small expressions, they can be placed on the same line.
0. `else` and `else if` should be placed on their own line, with the brace also
on its own line. Braces are mandatory in all cases.
0. Boolean operators should all be uppercase, as should mathematical MOD.
0. Parentheses () are optional on if/while statements, but are preferred. They
are required in for loop declarations.

Good:
```
   if (IsClass(oFinalTarget,&Battler)
      AND stroke_obj <> $
      AND (IsClass(stroke_obj,&Stroke)
         OR (IsClass(stroke_obj,&Spell)
            AND Send(stroke_obj,@GetNumSpellTargets) = 1)))
   {
      // Code
   }

   if (condition1)
   {
      // Code
   }
   else if (condition2)
   {
      // Code
   }
   else
   {
      // Code
   }
```

Acceptable:
```
if (what = $ OR who = $)
{
   // Code
}
```

Bad:
```
   if (IsClass(oFinalTarget,&Battler) AND stroke_obj <> $
      AND (IsClass(stroke_obj,&Stroke) OR (IsClass(stroke_obj,&Spell)
      AND Send(stroke_obj,@GetNumSpellTargets) = 1)))
   {
      // Code
   }

   if (condition1) {
      // Code
   } else if (condition2) {
      // Code
   } else {
      // Code
   }
```

#### Variables

0. Names on all variables and messages should be short and descriptive. "i"
and other single character variable names can be used as foreach loop iterators,
but otherwise ambiguous variable names should be avoided.
0. All property names should start with 'p' and all classvars with 'v'.
0. All local variables should start with a lower-case letter denoting the
expected type of the variable (i for integer, o for object etc.). This
lower-case letter should be the second letter of every property and classvar.
0. Message names should be camelcase (MessageName); variables should also
follow this convention after the identifying letters (piPropertyName).
0. Avoid overly long message and parameter names that force calls to span
multiple lines.
0. All constants should be uppercase, and use underscores to separate words.
0. Constants should be used instead of hardcoded numbers in code. If a constant
is used in more than one file, or will be in the future, it should be defined
in the constants file, blakston.khd.
0. All language-translated resources should go in a separate file, with the
.lkod extension. Resources in the .lkod file must have an English translation
defined in the .kod file.

#### General Coding

0. Message return types should be consistent, i.e. only TRUE or FALSE, or
an object/list or $. Messages return $ by default if no return value is
specified.
0. Created timers need to be attached to a property, as unattached timers
will cause errors during garbage collect/save.
0. Large amounts of timer or object creation should be avoided where possible;
kod interpretation occurs in a single thread and relying heavily on either
of these can negatively affect the server and at best will limit the number
of player the server can handle or future additions we can make.
0. Most objects have properties used as bitflags which can be used for
storing boolean values relating to that object. Use of these bitflags is
preferred over creation of single-purpose boolean integer properties.
0. Avoid making multiple message calls to obtain the same value - if the
result of a call will be used frequently and the result does not change,
consider saving the value into a local or property if possible.
0. Use and expand existing code infrastructure where possible, rather
than introduce parallel systems with high similarity. Many existing
systems are already solid and just require fleshing out rather than
duplicating.
0. Use the Debug C call to log errors that may occur when the code is live,
rather than just returning out from the message. This can speed up resolution
of errors and bugs on the live server.
0. If in doubt about how to do something, please ask for assistance in IRC or
on the forums rather than spend time working on something that can't be used.
We are happy to help out and teach new contributors, and asking for help can
save everyone time.
