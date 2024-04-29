rdef grammar
============

This is the (somewhat boring) specification of the rdef file format as it is understood by librdef.
It also describes to a certain extent how the compiler works. You don't need to read this unless
you want to hack librdef. Knowledge of compiler theory and lex/yacc is assumed.

The lexer
---------

Like any compiler, librdef contains a lexer (aka scanner) and a parser. The lexer reads the input
file and chops it up into tokens. The lexer ignores single-line ``//`` comments and ``/* ... */``
multi-line comments. It also ignores whitespace and newlines.

The lexer recognizes the following tokens:

BOOL
    true or false

INTEGER
    You can specify integers as decimal numbers, hexadecimal numbers (starting with 0x or 0X, alpha
    digits are case insensitive), octal numbers (starting with a leading 0), binary numbers
    (starting with 0b or 0B), or as a four character code ('CCCC'). Valid range is 64 bits. At
    this point, numbers are always unsigned. The minus sign is treated as a separate token, and is
    dealt with by the parser.

FLOAT
    A floating point literal. Must contain a decimal point, may contain a signed exponent. Stored internally as a double.

STRING
    UTF-8 compatible string literal, enclosed by double quotes. Can contain escape sequences
    (\b \f \n \r \t \v \" \\ \0), octal escapes (\000) and hex escapes (\0x00 or \x00). May not
    span more than one line, although you are allowed to specify multiple string literals in a row
    and the lexer will automatically concatenate them. There is no maximum length.

RAW
    Hexadecimal representation of raw data, enclosed by double quotes, and prefixed by a dollar
    sign: $"12FFAB". Each byte is represented by two hex characters, so there must be an even
    number of characters between the quotes. The alpha digits are not case sensitive. Like STRING,
    a RAW token may not span more than one line, but multiple consecutive RAW tokens are
    automatically concatenated. No maximum length.

IDENT
    C/C++ identifier. First character is alphabetic or underscore. Other characters are
    alphanumeric or underscore.

TYPECODE
    A hash sign followed by a 32-bit unsigned decimal number, hex number, or four character code.
    Examples: #200, #0x00C8, #'MIMS'

The following are treated as keywords and special symbols:

``enum resource array message archive type import { } ( ) , ; = - + * / % ^ | &amp; ~``

The lexer also deals with #include statements, which look like: #include "filename"\n. When you
#include a file, the lexer expects it to contain valid rdef syntax. So even though the include file
is probably a C/C++ header, it should not contain anything but the enum statement and/or comments.
The lexer only looks for include files in the include search paths that you have specified, so if
you want it to look in the current working directory you have to explicitly specify that. You may
nest #includes.

A note about UTF-8. Since the double quote (hex 0x22) is never part of the second or third byte of
a UTF-8 character, the lexer can safely deal with UTF-8 characters inside string literals. That is
also the reason that the decompiler does not escape characters that are not human-readable
(except the ones in the 7-bit ASCII range), because they could be part of a UTF-8 encoding.
The current version of librdef does not handle L"..." (wide char) strings, but nobody uses them anyway.

The parser
----------

The parser takes the tokens from the lexer and matches them against the rules of the grammar. What
follows is the grammar in a simplified variation of BNF, so the actual bison source file may look
a little different. Legend:

+-------------+-------------------------------------+
| `[ a ]`     | match a 0 or 1 times                |
+-------------+-------------------------------------+
| `{ b }`     | match b 0 or more times             |
+-------------+-------------------------------------+
| `c | d`     | match either c or d                 |
+-------------+-------------------------------------+
| `( e f )`   | group e and f together              |
+-------------+-------------------------------------+
| lowercase   | nonterminal                         |
+-------------+-------------------------------------+
| UPPER       | token from the lexer                |
+-------------+-------------------------------------+
| `'c'`       | token from the lexer                |
+-------------+-------------------------------------+

The rdef grammar consists of the following rules:


script
     {enum | typedef | resource}

enum
    ENUM '{' [symboldef {',' symboldef} [',']] '}' ';'

symboldef
    IDENT ['=' integer]

typedef
    TYPE [id] [TYPECODE] IDENT '{' fielddef {',' fielddef} '}' ';'

fielddef
    datatype IDENT ['[' INTEGER ']'] ['=' expr]

resource
    RESOURCE [id] [typecode] expr ';'

id
    '(' [(integer | IDENT) [',' STRING] | STRING] ')'

typecode
    ['('] TYPECODE [')']

expr
    expr BINARY_OPER expr | UNARY_OPER expr | data

data
    [typecast] (BOOL | integer | float | STRING | RAW | array | message | archive | type | define | '(' expr ')' )

typecast
    ['(' datatype ')']

datatype
    ARRAY | MESSAGE | ARCHIVE IDENT | IDENT

integer
    ['-'] INTEGER

float
    ['-'] FLOAT

array
    ARRAY ['{' [expr {',' expr}] '}'] | [ARRAY] IMPORT STRING

message
    MESSAGE ['(' integer ')'] ['{' [msgfield {',' msgfield}] '}']

msgfield
    [TYPECODE] [datatype] STRING '=' expr

archive
    ARCHIVE [archiveid] IDENT '{' msgfield {',' msgfield} '}'

archiveid
    '(' [STRING] [',' integer] ')'

type
    IDENT [data | '{' [typefield {',' typefield}] '}']

typefield
    [IDENT '='] expr

define
    IDENT

Semantics
---------

Resource names
##############

There are several different ways to specify the ID and name of a new resource:

``resource``
    The resource is assigned the default name and ID of its data type.

``resource()``
    The resource is assigned the default name and ID of its data type.

``resource(1)``
    The resource is assigned the numeric ID 1, and the default name of its data type.

``resource("xxx")``
    The resource is assigned the name "xxx" and the default ID of its data type.

``resource(1, "xxx")``
    The resource is assigned the numeric ID 1, and the name "xxx".

``resource(sss)``
    The resource is assigned the numeric ID that corresponds with the symbol sss, which should have
    been defined in an enum earlier. If the "auto names" option is passed to the compiler, the
    resource is also given the name "sss", otherwise the default name from its data type is used

``resource(sss, "xxx")``
    The resource is assigned the numeric ID that corresponds with the symbol sss, and the name "xxx".

Data types and type casts
#########################

Resources (and message fields) have a type code and a data type. The data type determines the
format the data is stored in, while the type code tells the user how to interpret the data.
Typically, there is some kind of relation between the two, otherwise the resource will be a little
hard to read.

The following table lists the compiler's built-in data types. (Users can also define their own
types; this is described in a later section.)

+---------+----------------+
| bool    | B_BOOL_TYPE    |
+---------+----------------+
| int8    | B_INT8_TYPE    |
+---------+----------------+
| uint8   | B_UINT8_TYPE   |
+---------+----------------+
| int16   | B_INT16_TYPE   |
+---------+----------------+
| uint16  | B_UINT16_TYPE  |
+---------+----------------+
| int32   | B_INT32_TYPE   |
+---------+----------------+
| uint32  | B_UINT32_TYPE  |
+---------+----------------+
| int64   | B_INT64_TYPE   |
+---------+----------------+
| uint64  | B_UINT64_TYPE  |
+---------+----------------+
| size_t  | B_SIZE_T_TYPE  |
+---------+----------------+
| ssize_t | B_SSIZE_T_TYPE |
+---------+----------------+
| off_t   | B_OFF_T_TYPE   |
+---------+----------------+
| time_t  | B_TIME_TYPE    |
+---------+----------------+
| float   | B_FLOAT_TYPE   |
+---------+----------------+
| double  | B_DOUBLE_TYPE  |
+---------+----------------+
| string  | B_STRING_TYPE  |
+---------+----------------+
| raw     | B_RAW_TYPE     |
+---------+----------------+
| array   | B_RAW_TYPE     |
+---------+----------------+
| buffer  | B_RAW_TYPE     |
+---------+----------------+
| message | B_MESSAGE_TYPE |
+---------+----------------+
| archive | B_MESSAGE_TYPE |
+---------+----------------+

The type code has no effect on how the data is stored. For example, if you do this:
"resource(x) #'LONG' true", then the data will not automatically be stored as a 32-bit number!
If you don't specify an explicit type code, the compiler uses the type of the data for that.

You can change the data type with a type cast. The following casts are allowed:

+--------------------+--------------------------------------------------------------------------+
| bool               | You cannot cast bool data.                                               |
+--------------------+--------------------------------------------------------------------------+
| integer            | You can cast to all numeric data types. Casts to smaller datatypes will  |
|                    | truncate the number. Casting negative numbers to unsigned datatypes (and |
|                    | vice versa) will wrap them, i.e. (uint8) -1 becomes 255.                 |
+--------------------+--------------------------------------------------------------------------+
| floating point     | You can only cast to float or double.                                    |
+--------------------+--------------------------------------------------------------------------+
| string             | You cannot cast string data.                                             |
+--------------------+--------------------------------------------------------------------------+
| raw, buffer, array | You can cast anything to raw, but not the other way around.              |
+--------------------+--------------------------------------------------------------------------+
| message, archive   | You cannot cast message data.                                            |
+--------------------+--------------------------------------------------------------------------+
| type               | You cannot cast user-defined types.                                      |
+--------------------+--------------------------------------------------------------------------+

In addition to the "simple" built-in data types, the compiler also natively supports several data
structures from the BeOS API (point, rect, rgb_color) and a few convenience types (app_signature,
app_flags, etc). These types all follow the same rules as user-defined types.

Arrays
######

The following definitions are semantically equivalent:

.. code-block:: c

    resource(x) $"AABB";
    resource(x) array { $"AA" $"BB" };
    resource(x) array { $"AA", $"BB" };

The comma is optional and simply concatenates the two literals. When you decompile this code,
it always looks like:

.. code-block:: c

    resource(x) $"AABB";

Strings behave differently. The following two definitions are equivalent, and concatenate the two
literals into one string:

.. code-block::

    resource(x) "AA" "BB";
    resource(x) #'CSTR' array { "AA" "BB" };

However, if you put a comma between the the strings, the compiler will still glue them together
but with a '\0' character in the middle. Now the resource contains *two* strings: "AA" and "BB".
You can also specify the '\0' character yourself:

.. code-block::

    resource(x) "AA\0BB";
    resource(x) #'CSTR' array { "AA", "BB" };

The following is not proper grammar; use an array instead:

.. code-block:: c

    resource(x) "AA", "BB";
    resource(x) $"AA", $"BB";

Note that the data type of an array is always raw data, no matter how you specify its contents.
Because raw literals may be empty ($""), so may arrays.

Messages and archives
#####################

A message resource is a flattened BMessage. By default it has the data type B_MESSAGE_TYPE and
corresponding type code #'MSGG'. If you don't specify a "what" code for the message, it defaults to 0.

Message fields assume the type of their data, unless you specify a different type in front of the
field name. (Normal casting rules apply here.) You can also give the field a different type code,
which tells the BMessage how to interpret the data, but not how it is stored in the message.
This type code also goes in front of the field name. You can give the same name to multiple fields,
provided that they all have the same type. (The data of these fields does not have to be the same
size.) A message may be empty; it is still a valid BMessage, but it contains no fields.

An archive is also a flattened BMessage, but one that was made by Archive()'ing a BArchivable class,
such as BBitmap. The name of the archive, in this case BBitmap, is automatically added to the
message in a field called "class". The "archive" keyword is optionally followed by a set of
parentheses that enclose a string and/or an integer. The int is the "what" code, the string is
stored in a field called "add_on" (used for dynamic loading of BArchivables). Other than that,
archives and messages are identical. The compiler does not check whether the contents of the
archive actually make sense, so if you don't structure the data properly you may be unable to
unarchive the object later. Unlike a message, an archive may not be empty, because that is pointless.

User-defined types
##################

We allow users to define their own types. A "type" is just a fancy array, because the data from the
various fields is simply concatenated into one big block of bytes. The difference is that
user-defined types are much easier to fill in.

A user-defined type has a symbolic name, a type code, and a number of data fields. After all the
fields have been concatenated, the type code is applied to the whole block. So, the data type of
this resource is always the same as its type code (unlike arrays, which are always raw data).
If no type code is specified, it defaults to B_RAW_TYPE.

The data fields always have a default value. For simple fields this is typically 0 (numeric types)
or empty (string, raw, message). The default value of a user-defined type as a whole is the
combination of the default values of its fields. Of course, the user can specify other defaults.
(When a user creates a new resource that uses such a type, he is basically overriding the default
values with his own.)

The user may fill in the data fields by name, by order, or using a combination of both. Every time
the compiler sees an unnamed data item, it stuffs it into the next available field. Named data
items are simply assigned to the field with the same name, and may overwrite a value that was
previously put there "by order". Any fields that are not filled in keep their default value. For
example:

.. code-block:: c

    type vector { int32 x, int32 y, int32 z, int32 w = 4 };
    resource(1) vector { 1, 3, x = 2 };

Here, x is first set to 1, y is set to 3, x is now overwritten by the value 2, z is given the
default value 0, and w defaults to 4.

Note: if a user-defined type contains string, raw, or message fields, the size of the type depends
on the data that the user puts into it, because these fields have a variable size. However, the
user may specify a fixed size for a field (number of bytes, enclosed in square brackets following
the field name). In this case, data that is too long will be truncated and data that is too short
will be padded with zeroes. You can do this for all types, but it really only makes sense for
strings and raw data. More about this in the manual.

A type definition may also contain a default resource ID and name. The default ID of built-in types
is usually 1 and the name is empty (NULL). For example:

.. code-block:: c

    type(10, "MyName") mytype { int32 a };
    resource mytype 123;

The resource is now called "MyName" and has ID 10. Obviously you can only do this once or you will
receive a duplicate resource error. If this type is used inside an array or other compound type,
the default ID and resource name are ignored. Note: this feature introduces a shift/reduce conflict
in the compiler:

.. code-block:: c

    resource (int8) 123;

This probably doesn't do what you expect. The compiler now considers the "(int8)" to be the
resource ID, not a typecast. If you did not declare "int8" in an enum (probably not), this gives a
compiler error. Not a big problem, because it is unlikely that you will ever do this. Here is a
workaround:

.. code-block:: c

    resource() (int8) 123;

The grammar and Bison
#####################

Above I mentioned one of the shift/reduce conflicts from the grammar. There are several others.
These are mostly the result of keeping the original grammar intact as much as possible, without
having to introduce weird syntax rules for the new features. These issues aren't fatal but if you
try to do something funky in your script, you may get an error message.

The main culprit here is the "( expr )" rule from "data", which allows you to nest expressions with
parens, e.g. "`(10 + 5) * 3`". This causes problems for Bison, because we already use parens all
over the place. Specifically, this rule conflicts with the empty "MESSAGE" from the "message" rule,
"ARRAY" from "array", and "IDENT" from "type". These rules have no other symbols following them,
which makes them ambiguous with respect to the "datatype" rules. Still with me? The parser will
typically pick the right one, though.

The nested expressions rule also caused a reduce/reduce conflict. To get rid of that, I had to
explicitly mention the names of the various types in the "typecast" rule, which introduces a little
code duplication but it's not too bad. Just so you know, the original rule was simply:

.. code-block::

    typecast
        : '(' datatype ')' { $$ = $2; }
        ;

The new rule is a little more bulky:

.. code-block::

    typecast
        : '(' ARRAY ')'   { ... }
        | '(' MESSAGE ')' { ... }
        ... and so on for all the datatypes ...
        ;

The unary minus operator is not part of the "expr" (or "data") rules, but of "integer" and "float".
This also causes a shift/reduce warning.

And finally, "type" is a member of "data" which is called by "expr". One of the rules of "type"
refers back to "expr". This introduces a recursive dependency and a whole bunch of shift/reduce
conflicts. Fortunately, it seems to have no bad side effects. Yay!

Symbol table
############

The compiler uses two symbol tables: one for the enum symbols, and one with the data type
definitions. We need those tables to find the numeric ID/type definition that corresponds with an
identifier, and to make sure that there are no duplicate or missing identifiers. These two symbol
tables are independent, so you may use the same identifier both as an enum symbol and a type name.

The compiler does not need to keep a symbol table for the resources. Although the combination of a
resource's ID and its type code must be unique, we can use the BResources class to check for this
when we add a resource. There is no point in duplicating this functionality in the compiler.
(However, if we are merging the new resources into an existing resource file, we will simply
overwrite duplicates.)

Misc remarks
############

As the grammar shows, the last field in an enum statement may be followed by a comma. This is
valid C/C++ syntax, and since the enum will typically end up in a header file, we support that
feature as well. For anything else between braces, the last item may not be followed by a comma.

The type code that follows the "resource" keyword may be enclosed by parens for historical reasons.
The preferred notation is without, just like in front of field names (where no parens are allowed).

Even though "ARCHIVE IDENT" is a valid data type, we simply ignore the identifier for now. Maybe
later we will support casting between different archives or whatever. For now, casting to an
archive is the same as casting to a message, since an archive is really a message.

User-defined types and defines have their own symbol tables. Due to the way the parser is
structured, we can only distinguish between types and defines by matching the identifier against
both symbol tables. Here types have priority, so you could 'shadow' a define with a type name if
you were really evil. Maybe it makes sense for rc to use one symbol table for all things in the
future, especially since we're using yet another table for enums. We'll see.
