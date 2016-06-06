Blakod Resource Strings
--------------

### Basic Format and Use
Kod resources are defined in the `resources:` block of a kod file. Resources
are used to refer to hardcoded strings or filenames using the following format:
```
resource_name = "string"
resource_filename = filename.ext
```

The resources block can also use the `include` keyword to include a file
containing more resources:
```
include filename.lkod
```
Included files should only contain resources, comments and must end with
a blank line.

Resources can only be assigned values inside the `resources:` block. When a
resource is referred to by name elsewhere in the code, the resource's unique
ID is placed instead. Resources can be assigned to local variables, passed as
parameters in messages and used in C calls (e.g. string comparison calls).

### Language Identifiers
Resources can specify an optional language identifier which takes the form of a
two-letter language code specified by the ISO 639-1 standard. If no language
code is specified, the compiler will assign it the English one by default.
```
object_name_rsc = "something"
object_name_rsc = de "etwas"
```
All resources must have an English string defined as this is the fallback
option the server and clients use for missing resource strings in other
languages. English resources should go into the kod file itself, and other
language strings should be placed in a file of the same name as the kod file,
but with the .lkod extension. If multiple strings are defined in the same
language for a single resource, the compiler will log an error.

### String Formatters
Resource strings can contain string formatters similar to C-style strings which
allow a larger string to be assembled by the clients. The available formatters
are:

* `%i` - used for literal integers
* `%d` - used for literal integers (deprecated)
* `%q` - used for literal string
* `%s` - used for resource IDs
* `%r` - used for specify that the client should process the next data field as a
complete message, and place the result in %r

The client expects 4 bytes for %i, %d, %s and %r, and a 2 byte string length
followed by the literal string for %q.

%s and %r differ in that resources placed in %s can contain their own string
formatters, but these are processed after the original resource is completed
(i.e. the 2nd resource is filled with data from the end of the data stream from
the server). Resources placed in %r are filled with the next data fields
available.

String formatters must match in number and type for each language string
available for a given resource. If not, the compiler will issue a warning
with the type of error (mismatched number, or mismatched type).

### String order formatters
The server will send data to assemble resources in the order in which the string
formatters are specified in the English resource, for example:
```
group_experience_rsc = \
   "You absorb a small amount of the energy released "
   "from %s's slaying of the %s."

Send(self,@MsgSendUser,#message_rsc=group_experience_rsc,
      #parm1=Send(group_member,@GetName),
      #parm2=Send(what,@GetName));
```
The client will receive 12 bytes for this message (in order):
* 4 byte resource ID for group_experience_rsc
* 4 byte resource ID for group member name
* 4 byte resource ID for monster name

The group member's name is placed in the first %s, and the monster's name in
the second %s. Other languages may have different grammar from English and
require the resources to be ordered differently, for example the same resource
in German:
```
group_experience_rsc = de \
   "%s$2 wurde von %s$1 getötet. Du absorbierst eine kleine Menge an Energie "
   "von diesem Mord."
```
This string has the monster's name first, followed by the group member's name.
To allow the client to correctly assemble this string, it must be told that it
should use the 2nd extra data field (the monster name) first, and the first
extra field (group member name) second. This is accomplished by using the $
parameters as seen in the German string.

The $ parameter must be placed after the string formatter it is modifying, e.g.
`%s$1`. The same resource should not contain $ paramters referencing the same
number (e.g. two instances of $1, or a parameter referencing $1 without the
string formatter in position 1 referencing another position).

The other function of the $ parameters is to drop some data fields from being
processed, for example:
```
player_is_holding = "%s is holding %s%s.\n"
player_is_holding = de "%s$0Hält: %s%s.\n"
```
In this case, the first data field meant for the first %s is skipped when
displaying the German resource.
