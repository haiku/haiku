===============================
PDFlib Lite Source for Mac OS 9
===============================

Note:
please see readme-source-unix.txt for information on using PDFlib on Mac OS X.

To compile PDFlib with Metrowerks CodeWarrior, open the supplied
project file PDFlib.mcp with the Metrowerks IDE. The project file
contains targets for building a static PPC library

Separate project files for building various C and C++ sample programs
can be found in bind/pdflib/c/samples.mcp and bind/pdflib/cpp/samples.mcp.
These can be used to test the newly created library. The tests create simple
command-line programs without any fancy user interface.

Note that not all tests will succeed because they
need features which require commercial PDFlib products.

In order to build a shared PDFlib library you'll have to define the
PDFLIB_EXPORTS preprocessor symbol (preferably in a new prefix file).

In order to make the C and C++ samples work on OS 9 you must change the
SearchPath to use a Mac volume name instead of a relative path name, e.g.:

"../data" ==> "Classic:software:pdflib-5.0.x:bind:pdflib:data"


Note that on OS 9 only C and C++ are available; other language
wrappers are no longer supported. All language wrappers are fully
supported on Mac OS X, though.
