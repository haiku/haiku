Introduction
============

In the world of BeOS programming, a "resource" is data that is bundled with your application.
Typical examples are the application's icons and its signature, but you can attach any data you
want (bitmaps, text, cursors, etc). You stuff this data into a .rsrc file that will be linked to
your application when it is compiled.

Because .rsrc files have a binary file format, you need to use special tools to edit them, such as
FileTypes, QuickRes, or Resourcer. Alternatively, you can use a "resource compiler". This is a
command line tool that takes a text-based resource script and turns it into a .rsrc file.

With a resource compiler, you express your resources as ASCII text using a special definition
language, which makes the resource files much easier to edit and maintain. You no longer need
separate GUI tools to build your .rsrc files, and you can even automate the whole process by
calling the compiler from your Makefile or Jamfile. Resource scripts will also make your life
easier if you use version control, because version control doesn't handle .rsrc files very well.

BeOS R5 comes with an (experimental) resource compiler called "beres", and a corresponding
decompiler called "deres". rc is an open source replacement (and enhancement) of these tools. It
is (mostly) backwards compatible, so you should be able to compile your old .rdef files without any
problems.

How to install
==============

- Copy ``rc`` into ``/boot/home/config/non-packaged/bin``
- Copy ``librdef.so`` into ``/boot/home/config/non-packaged/lib``
- Let's party!

Note: rc comes preinstalled in Haiku already.

Writing resource scripts
========================

Writing resource scripts is not difficult, although the syntax may take some getting used to. A
resource script is a plain text file with one or more resource definition statements. In addition,
it may contain C or C++ style comments. By convention, resource script files have the extension ".rdef".

Here is an example of a simple resource script:

.. code-block:: c

    resource(1) true;      /* this is a comment */
    resource(2) 123;       // and so is this
    resource(3) 3.14;
    resource(4) "hello world";
    resource(5) $"0123456789ABCDEF";

When compiled, this script produces a resource file with five resources. The above example also
illustrates the types of data that resources are allowed to have: boolean, integer, floating point,
character string (UTF-8), and raw data buffer (hexadecimal).

By default, integer data is stored as a 32-bit int, and floating point data is stored as a 4-byte
float. If you want to change the way the data is stored, you have to cast it:

.. code-block:: c

    resource(6) (int8) 123;
    resource(7) (double) 3.14;

You can cast integer data to the following types: int8, uint8, int16, uint16, int32, uint32, int64,
uint64, ssize_t, size_t, off_t, time_t, float, double, raw. You can cast floating point data to:
float, double, raw. You are not allowed to cast boolean, string, or raw data to other types.

In addition to casting, you can also change the resource's type code. This does not change the way
the data is stored, only what it is called. To change the type code of a resource:

.. code-block::

    resource(8) #'dude' 123;

This changes the type of resource 8 to the four-character-code 'dude'. If you did not change it, it
would be 'LONG', which stands for 32-bit integer. By changing the type code, you assign a new
meaning to the resource. You can also specify type codes as decimal or hexadecimal numbers:

.. code-block::

    resource(9) #200 123;
    resource(10) #0xC8 123;

For historical reasons, you may also enclose the type code in parens, but that is not the preferred
notation. Type casts and type codes can be combined:

.. code-block::

    resource(11) #'dude' (int8) 123;
    resource(11) (#'dude') (int8) 123;

In the above examples, we have given the resources numeric IDs. The combination of ID and type code
must be unique in the resource file; you cannot have two int32 resources with ID 1, for example.
However, it is perfectly fine (but not necessarily a good idea) to do the following, because the
data types are different:

.. code-block:: c

    resource(12) 123;
    resource(12) "w00t!";

For your own convenience, you can also name resources. Unlike the ID/type code combination, names
do not have to be unique.

.. code-block:: c

    resource(13, "Friday") "Bad Luck";

You can also do simple maths. The emphasis here is on simple because the number of operators is
limited, they only work on integer data (or anything that can be cast to integer), and the result
is always 32-bit integer as well. Still, the lazy amongst you may find it handy:

.. code-block:: c

    resource(14) 2 * (4 + 3);

Since it is likely that you will be using these resources from a C/C++ program, it may be
convenient to refer to them by symbolic names instead of hardcoded numeric ID's. The rdef format
allows you to do this:

.. code-block:: c

    {
        R_AppName = 1,
        R_SomeOtherThing = 2
    };

    resource(R_AppName) "MyKillerApp";

The compiler will automatically substitute the symbol R_AppName with the number 1. (You don't have
to start these symbol names with the prefix ``R_``, but it is somewhat of a convention.)

Now how do you tell your C/C++ app about these symbolic names? You simply put the enum into a
header file that you include both from your application's source code and your rdef file. The
header file, which we'll call "myresources.h", now looks like this:

.. code-block:: c

    {
        R_AppName = 1,
        R_SomeOtherThing = 2
    }

And the rdef file becomes this:

.. code-block:: c

    #include "myresources.h"

    resource(R_AppName) "MyKillerApp";

Don't let the .h suffix fool you: the header file is still considered to be an rdef file, and must
contain valid rdef syntax. If you add any other C/C++ code, your resource script will fail to
compile. Of course, you shouldn't add any other rdef syntax to the header either (unless you want
your C++ compiler to start complaining). Besides comments, the only safe thing to put in that
header file is the enum statement, because both rdef and C/C++ understand it.

Just like IDs, symbolic identifiers can be combined with a name:

.. code-block:: c

    resource(R_AppName, "AppName") "MyKillerApp";

If you don't specify a name, and invoke the compiler with the ``--auto-names`` option, it
automatically uses the symbolic ID for the name as well. So the ID of the following resource is 1
(because that is what R_AppName corresponds to) and its name becomes "R_AppName":

.. code-block:: c

    resource(R_AppName) "MyKillerApp";

Big fat resources
=================

The resources we have made so far consisted of a single data item, but you can also supply a
collection of data values. The simplest of these compound data structures is the array:

.. code-block:: c

    resource(20) array { 1234, 5678 };

An array is nothing more than a raw buffer. The above statement takes the two 32-bit integers 1234
and 5678 and stuffs them into a new 64-bit buffer. You can put any kind of data into an array, even
other arrays:

.. code-block:: c

    resource(21) array
    {
        "hello",
        3.14,
        true,
        array { "a", "nested", "array" },
        $"AABB"
    };

It is up to you to remember the structure of this array, because array resources don't keep track
of what kind of values you put into them and where you put these values. For that, we have messages.
A message resource is a flattened BMessage:

.. code-block::

    resource(22) message('blah')
    {
        "Name" = "Santa Claus",
        "Number" = 3.14,
        "Array" = array { "a", "nested", "array" },
        "Other Msg" = message { "field" = "value" }
    };

A message has an optional "what" code, in this case 'blah', and one or more fields. A field has a
name (between double quotes), a value, and a data type. By default, the field assumes the type of
its data, but you can also specify an explicit data type and type code in front of the field name:

.. code-block::

    resource(23) message('bla2')
    {
        "integer1" = (int8) 123,             // use cast to change data type
        int16 "integer2" = 12345,            // specify data type
        #'dude' "buffer1" = $"aabbccdd",     // specify a new type code
        #'dude' raw "buffer2" = $"aabbccdd"  // you can also combine them
    };

A special type of message is the "archive". The BeOS API allows you to take a BArchivable class an
flatten it into a BMessage. You can also add such archives to your resource scripts:

.. code-block::

    resource(24) #'BBMP' archive BBitmap
    {
        "_frame" = rect { 0.0, 0.0, 63.0, 31.0 },
        "_cspace" = 8200,
        "_bmflags" = 1,
        "_rowbytes" = 256,
        "_data" =  array
        {
            ... /* here goes the bitmap data */ ...
        }
    };

So what's this "rect" thing in the "_frame" field? Besides arrays and messages, the compiler also
supports a number of other data structures from the BeAPI:

+-----------+-------------------------+--------------------------------+
| Type      | Corresponds to          | Fields                         |
+===========+=========================+================================+
| point     | BPoint, B_POINT_TYPE    | float x, y                     |
+-----------+-------------------------+--------------------------------+
| rect      | BRect, B_RECT_TYPE      | float left, top, right, bottom |
+-----------+-------------------------+--------------------------------+
| rgb_color | rgb_color, B_COLOR_TYPE | uint8 red, greed, blue, alpha  |
+-----------+-------------------------+--------------------------------+

To add a color resource to your script, you can do:

.. code-block:: c

    resource(25) rgb_color { 255, 128, 0, 0 };

Or you can use the field names, in which case the order of the fields does not matter:

.. code-block:: c

    resource(26) rgb_color
    {
        blue = 0, green = 128, alpha = 0, red = 255
    };

You can also make your own data structures, or as we refer to them, "user-defined types". Suppose
that your application wants to store its GUI elements in the resources:

.. code-block::

    type #'menu' menu
    {
        string name,
        int32 count,  // how many items
        array items   // the menu items
    };

    type #'item' menuitem
    {
        string name,
        message msg,
        bool enabled = true  // default value is "true"
    };

A type has a name, an optional type code, and one or more fields. You are advised not to pick a
type code that already belongs to one of the built-in types, to avoid any confusion. Each field has
a data type, a name, and a default value. If you don't specify a default, it is typically 0 or
empty. To create a new menu resource using the types from the above example, you might do:

.. code-block::

    resource(27) menu
    {
        name = "File",
        count = 3,
        items = array
        {
           menuitem { "New...",   message('fnew') },
           menuitem { "Print...", message('fprt'), false },
           menuitem { "Exit",     message('_QRQ') }
        }
    };

Like an array, a type resource doesn't remember its internal structure. You can regard types as
fancy arrays that are easier to fill in, a template if you will. User-defined types work under the
same rules as the built-in types point, rect, and rgb_color, so you can specify the fields in order
or by their names. If you don't specify a field, its default value will be used.

Types can also have a default resource ID and/or name. If you omit to give the resource an ID or a
name, it uses the defaults from its data type. For example, this:

.. code-block:: c

    type myint { int32 i };
    resource(10, "MyName") myint { 123 };

Is equivalent to this:

.. code-block:: c

    type(10, "MyName") myint { int32 i };
    resource myint { 123 };

And to save you even more typing, simple types that have only one field can also be specified as:

.. code-block:: c

    resource myint 123;

Most data types have a fixed size; a uint16 is always 2 bytes long, a float always comprises 4
bytes, and so on. But the sizes of string and raw data resources depend on what you put in them.
Sometimes you may want to force these kinds of resources to have a fixed size as well. You can do
this as follows:

.. code-block:: c

    type fixed { string s[64] };

Any resources with type "fixed" will always contain a 64-byte string, no matter how many characters
you actually specify. Too much data will be truncated; too little data will be padded with zeroes.
Note that string resources are always terminated with a null character, so string "s" in the above
type only allows for 63 real characters. The number between the square brackets always indicates
bytes (unlike C/C++ arrays which use a similar notation).

If you have (large) binary files that you want to include in the resources, such as pictures of
Buffy, you don't need to convert the binary data to text form first. You can simply "import" the
file:

.. code-block::

    resource(22) #'PNG ' import "buffy.png";

Imported resources are always arrays (raw data), and you can specify the import statement
everywhere that array data is valid.

Application resources
=====================

All BeOS applications (except command line apps) have a basic set of resources, such as a MIME
signature, launch flags, icons, and a few others. Adding these kinds of resources is easy, because
rc also has a number of built-in types for that:

+---------------+------------------------------+--------------------------------------------------+
| Type          | Corresponds to               | Fields                                           |
+===============+==============================+==================================================+
| app_signature | the apps's MIME signature    | string signature                                 |
+---------------+------------------------------+--------------------------------------------------+
| app_flags     | the application launch flags | uint32 flags                                     |
+---------------+------------------------------+--------------------------------------------------+
| app_version   | version information          | uint32 major, middle, minor, variety, internal   |
|               |                              | string short_info, long_info                     |
+---------------+------------------------------+--------------------------------------------------+
| large_icon    | 32x32 icon                   | array of 1024 bytes                              |
+---------------+------------------------------+--------------------------------------------------+
| mini_icon     | 16x16 icon                   | array of 256 bytes                               |
+---------------+------------------------------+--------------------------------------------------+
| vector_icon   | HVIF vector icon (type VICN) | array of bytes                                   |
+---------------+------------------------------+--------------------------------------------------+
| file_types    | supported file types         | message                                          |
+---------------+------------------------------+--------------------------------------------------+

Here are some examples on how to use these resources. These things are also documented in the
Storage Kit section of the BeBook, so refer to that for more information.

The signature:

.. code-block:: c

    resource app_signature "application/x-vnd.YourName.YourApp";

The application flags determine how your application is launched. You must 'OR' together a combination of the following symbols:

- ``B_SINGLE_LAUNCH``
- ``B_MULTIPLE_LAUNCH``
- ``B_EXCLUSIVE_LAUNCH``
- ``B_BACKGROUND_APP``
- ``B_ARGV_ONLY``

For example:

.. code-block:: c

    resource app_flags B_SINGLE_LAUNCH | B_BACKGROUND_APP;

The version information resource contains a number of fields for you to fill in. Most are pretty
obvious, except maybe for the "variety" field. It can take one of the following values:

``B_APPV_DEVELOPMENT``
    development version

``B_APPV_ALPHA``
    alpha version

``B_APPV_BETA``
    beta version

``B_APPV_GAMMA``
    gamma version

``B_APPV_GOLDEN_MASTER``
    golden master

``B_APPV_FINAL``
    release version

For example:

.. code-block:: c

    resource app_version
    {
        major      = 1,
        middle     = 0,
        minor      = 0,
        variety    = B_APPV_BETA,
        internal   = 0,
        short_info = "My Cool Program",
        long_info  = "My Cool Program - Copyright Me"
    };

The supported file types resource contains a list of MIME types, not unlike this:

.. code-block:: c

    resource file_types message
    {
        "types" = "text/plain",
        "types" = "text"
    };

Compiling
=========

rc is a command line tool, which means you must run it from a Terminal window. Typical usage example:

.. code-block:: sh

    rc -o things.rsrc things.rdef

This tells rc that you wish to compile the script "things.rdef" to the resource file "things.rsrc".
The default name for the output file is "out.rsrc", but you can change that with the ``-o``
or ``--output`` switch, just like we did here.

You can specify multiple rdef files if you wish, and they will all be compiled into one big
resource file. If your rdef files #include files that are not in the current working directory,
you can add include paths with the ``-I`` or ``--include`` option. For a complete list of options,
type ``rc --help``.

If your project uses a Makefile, you can have rc automatically generate the resource file for you:

.. code-block:: make

    things.rsrc: things.rdef
    	rc -o $@ $^</PRE></BLOCKQUOTE>

.. TODO: also explain how to integrate rc in jamfiles

Decompiling
===========

Of course you can write the resource scripts by hand, but if you already have a .rsrc file you can
tell rc to decompile it. This will produce a ready-to-go rdef script, and save you some trouble.
(Although in some cases it may be necessary to edit the script a little to suit your needs.) Note
that rc isn't limited to just .rsrc files; you can decompile any file that has resources,
including applications.

For example, to decompile the file "things.rsrc" into "things.rdef", do:

.. code-block:: sh

    rc --decompile -o things.rdef things.rsrc

The decompiler produces an rdef resource script with the name "out.rdef", but you can change that
name with the ``-o`` or ``--output`` switches. If you specify the ``--auto-names`` option, rc will also
write a C/C++ header file. Any resources whose name is a valid C/C++ identifier will be added to
the header file. Now your program can access the resource using this symbolic name.

Note: Even though rc can decompile multiple .rsrc files into one script, it does not detect
conflicts in resource names or IDs. In such cases, the resulting .rdef and/or .h files may give
errors when you try to compile them.

Authors
=======

The rc resource compiler and its companion library librdef were written by `Matthijs Hollmans <mailto:mahlzeit@users.sourceforge.net>`_ for the `Haiku <http://www.haiku-os.org>`_ project. Thanks to Ingo Weinhold for the Jamfile and the Jam rules. Comments, bug reports, suggestions, and patches are welcome!
