Blakod (kod) variable data types
--------------

Kod is dynamically typed language - global variables are declared as classvars
(cannot be assigned to in messages) or properties (can be assigned to) and
these variables can hold any data type. Classvars are declared in the `classvars:`
block, properties in the `properties:` block. When these are first declared,
they can only be assigned constant values (nil, integer, class ID or resource ID).

Resource names are declared in the `resources:` block and must hold a string or
a filename.

Parameters in message headers must be assigned a value. These can be assigned
any constant value (nil, integer, class ID or resource ID).

Local variables are declared as the first line of executable code inside
a message block. Only declarations are allowed here, all variables are assigned
nil/null (i.e. `$`) automatically and cannot be manually assigned. Like
properties and classvars, locals can hold any data type.

Each variable is identified by the interpreter using the first 4 bits, allowing
for 16 data types. The remaining 28 bits are used for the data.

Kod has the following data types:

#### Nil
`Data type tag: 0000 (0)`

Special nil value (kod's equivalent of null). Written in kod as `$`, and is
used to denote any empty variable. `$` is also the value used to show an empty
list node, or the end of a list.

#### Integer
`Data type tag: 0001 (1)`

Signed integer, capable of expressing -134217728 to 134217727.

#### Object
`Data type tag: 0010 (2)`

An object ID referencing a kod object in an array of objects. Everything in
the game that can be interacted with (players, monsters, rooms, spells, trees,
items etc.) are objects and can be addressed in code by their object ID.

#### List / list node
`Data type tag: 0011 (3)`

Lists are kod's main collection data type. A kod list is a singly-linked list,
and list IDs refer to the first list node of the list.

#### Resource
`Data type tag: 0100 (4)`

Resource identifier - specifies either a filename or a string.

#### Timer
`Data type tag: 0101 (5)`

Timer identifier.  Timers are set in Blakod and can deliver any message to any
object when they expire.

#### Session
`Data type tag: 0110 (6)`

Identifies a connection to a user.

#### Room data
`Data type tag: 0111 (7)`

Room description/file data.

#### Temp. string
`Data type tag: 1000 (8)`

A special temporary string that is used as a scratch variable in the
interpreter.

#### String
`Data type tag: 1001 (9)`

A string typed by a player, such as a mail message. Referred to in
documentation as "kod string" to differentiate them from other strings.

#### Class
`Data type tag: 1010 (10)`

Class identifier of an object.

#### Message
`Data type tag: 1011 (11)`

Identifies the name of a message handler.

#### Debug string
`Data type tag: 1100 (12)`

String type for any hardcoded strings in kod that are not assigned to resources.

#### Table
`Data type tag: 1101 (13)`

A variable size hash table with open hashing (each entry in
the table is a linked list).
