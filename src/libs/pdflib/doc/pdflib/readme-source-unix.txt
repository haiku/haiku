===========================
PDFlib Lite Source for Unix
===========================

Building PDFlib Lite
--------------------

To start the PDFlib Lite build process on Unix, type

./configure
make

Several options can be used with the configure script in order to
override some default configuration options, or to assist configure
in finding some software locations on your machine. Type

./configure --help

before the make command in order to see a list of available configure
options.

IMPORTANT: make sure to use only absolute path names for all custom
directories. Also, wildcards should not be used. This requirement is
necessary because otherwise the paths won't work as include paths with
compiler calls.

If you want to use PDFlib on another machine, do not simply copy
the PDFlib source tree over. Instead, copy the distribution fileset
and re-run the configure script. Otherwise compiler, shared library
and installation settings could erroneously be taken from the first
machine instead of from the actual one.


Testing and installing the library
----------------------------------

Optionally, to run sample PDFlib applications in several programming
environments (including the scripting languages which have been
detected by configure), type:

make test

Note that not all tests will succeed. The failing tests require features
which are only available in the binary PDFlib distributions.

In order to install the library and the support files for all detected
scripting languages, type

make install

Note that installing will usually require root privileges.

If you want to install only selected parts (e.g., only the PDFlib
C library or the Perl support), type "make install" in the
respective subdirectory (e.g, bind/pdflib/perl).


configure troubleshooting
-------------------------

The configure script helps to keep PDFlib portable across a wide variety
of systems, and to keep track of many different configurations and
the availability of features. Generally the script does a good job.
Given the huge number of different systems, configure may occasionally
fail in one of several ways:

- failing to detect installed software

- failing to complete all tests due to errors during script execution

In the first case, you can help configure by finding out the necessary
paths etc. yourself, and supplying any required --with-... option on
the configure command line.

In the second case, you either also can try to supply --with-... options
in order to prevent the failing test from being called, or abandon
the feature if you don't need it by supplying the value "no" to the
respective configure option, e.g., --with-tcl=no.

If you can determine the cause of a failing configure script, we will
be happy to hear from you. Please supply your system details, the
feature/option in question, and a workaround or improvement if possible.

The configure script may fail under Cygwin when trying to probe for languages
if path names contain space characters.
It's safer to disable language probing using --with-perl=no etc.


Forcing a certain compiler or compiler flags
--------------------------------------------

You can set several environment variables before running the configure
script. These variables will be used in the generated Makefiles. The more
important ones are:

CC       The C compiler to use
CXX      The C++ compiler to use
CFLAGS   C compiler flags
LDFLAGS	 linker flags, such as additional libraries

For example, the following works well on Solaris (in a csh environment):

setenv CC /opt/SUNWspro/bin/cc
setenv CXX /opt/SUNWspro/bin/CC
setenv CFLAGS '-fast -xO3 -xtarget=generic'
./configure


Shared library support
----------------------

Language bindings other than C/C++ require shared library support for
PDFlib to work. By default, the PDFlib core library will be built as both
a static and a shared library if possible.

C or C++ language clients must deploy libtool for using PDFlib, or install
the generated PDFlib library using "make install".

PDFlib relies on GNU libtool for shared library support. libtool
shadows the object files and libraries with a layer of .lo and .la
files. The actual library files are placed in a subdirectory called
".libs". The PDFlib Makefiles and libtool will take care of correctly
building, testing, and installing these libraries. If anything goes
wrong on your system, read the manual section on shared libraries,
take a look at the contents of the .libs subdirectory, and observe
what the supplied Makefiles do for compiling, linking, testing, and
installing.


Auxiliary libraries
-------------------

PDFlib includes portions of the libtiff, zlib, and libpng auxiliary
libraries as part of the source code package. These libraries have been
modified for use with PDFlib in several ways:

- all function names are prefixed with a PDFlib-private tag
- code which is not required for PDFlib has been removed
- a number of portability changes have been applied

Our build process directly links these libraries into the PDFlib binary,
regardless of whether a shared or static PDFlib is generated.
External versions of these libraries are not supported.

Due to the prefixed function names an application can link against both
PDFlib (including all auxiliary libraries) and standard
versions of these libs without any name conflicts.


Querying PDFlib configuration info
----------------------------------

In order to find out details about PDFlib's version, configuration,
and use, the pdflib-config shell script can be used. It is built during
the configure run, and returns all information you'll need for PDFlib
deployment. Running the script without any options lists the supported
command line options.


Library version numbers
-----------------------

Libtool-generated libraries such as PDFlib number their interfaces
with integer interface numbers (no subversions!). In addition to the
interface number, a revision number can be used. A particular library
supports a range of interface numbers, where the range can have a length
of one or more. In particular, libtool defines the following:

CURRENT   The most recent interface number that this library implements.
REVISION  The implementation number of the CURRENT interface.
AGE       The length of the range of supported interfaces (i.e., CURRENT
          numbers).

The following table relates PDFlib version numbers to the C:R:A library
versioning scheme used by libtool. Note that these numbers will not show
up in the PDFlib shared library file name directly, but in some modified form
which is system-dependent:

PDFlib    C:R:A   comments
-----------------------------------------------------------------------------
3.00      0:0:0   first release based on libtool
3.01      0:1:0   maintenance release (bug: should have increased C since
                  undocumented functions were removed)
3.02      1:0:0   cleans up the non-incrementing glitch in 3.01
3.03      1:1:1   maintenance release (bug: should not have increased A)
4.0.0b    2:0:2   new API functions (inherits the 3.03 bug)
4.0.0     2:0:1   cleanup for major release: repairs the 3.03 "age" bug
4.0.1     2:1:1   maintenance release
4.0.2     2:2:1   maintenance release
4.0.3     2:3:1   maintenance release
5.0.0     3:0:2   new API functions (bug: should have been 3:0:0 since
                  PDF 1.2 compatibility has been dropped)
5.0.1     3:1:1   maintenance release (trying to fix the 5.0.0 bug)
5.0.2     4:0:2   maintenance release, but also a few minor new features
5.0.3     4:1:2   maintenance release

When the PDFlib core is built as a static library version numbers will not
be visible. However, since language bindings other than C and C++ are always
built as shared libraries, they will have version numbers visible on most
systems.

Many thanks to Evgeny Stambulchik for leading me on the right track
with respect to libtool and library versioning schemes!
