==============================
PDFlib Lite Source for Windows
==============================

Building PDFlib with MS Visual Studio 6
---------------------------------------

To compile PDFlib with MS Visual Studio 6, open the supplied workspace
file PDFlib.dsw which contains several projects for the core library
and language bindings. Select the "pdflib" project to build a static library
pdflib.lib.

The "pdflib_dll" project can be used to build a DLL version pdflib.dll.

This will set the PDFLIB_EXPORTS #define. Client programs must define
PDFLIB_DLL before including pdflib.h in order to use the DLL.


The following configurations are supported:
- "Debug"
- "Release" will include a static version of the C runtime.
- "Release mtDLL" (only for target pdflib) builds a static library for
   use with applications which use a multithreaded DLL version of the
   C runtime (msvcrt.dll).


Sample applications:

The examples_c.dsw and examples_cpp.dsw contain targets for a few
C/C++ sample applications.

Note that not all tests will succeed because they
need features which require commercial PDFlib products.


Building PDFlib with MS Visual Studio .NET
------------------------------------------

Although the PDFlib packages do not contain dedicated project files for use
with Visual Studio .NET (aka VS 7), the project files and workspaces supplied
for use with Visual Studio 6 can be used. Proceed as follows to compile PDFlib
with Visual Studio .NET:

- Open the workspace PDFlib.dsw with Visual Studio .NET
- A dialog box will pop up and ask
  "The project 'Java.dsp' must be converted to the current Visual C++ project
  format. ... Convert and open this project?"
  In this dialog click "Yes to All".
- Proceed as described above for Visual Studio 6
- Save the project files (will be done automatically upon exiting VS .NET)



Building PDFlib with Metrowerks CodeWarrior
-------------------------------------------

To compile PDFlib with Metrowerks CodeWarrior, open the supplied
project file PDFlib.mcp with the Metrowerks IDE. The project file
contains targets for building a static PPC library

Separate project files for building various C and C++ sample programs
can be found in bind/pdflib/c/samples.mcp and bind/pdflib/cpp/samples.mcp.
These can be used to test the newly created library. The tests create simple
command-line programs without any fancy user interface.

Note that not all tests will succeed because they
need features which require commercial PDFlib products.

In order to build a shared PDFlib library you'll have to define
the PDFLIB_EXPORTS preprocessor symbol (preferably in a new prefix
file).


Building PDFlib with other Windows compilers
--------------------------------------------

In order to build PDFlib with other compilers, observe the above
notes and make sure to define the preprocessor symbol WIN32.


Compiling the language wrappers
-------------------------------

In order to compile the C wrappers for the supported languages you
will have to install the relevant source code package
and adjust the include paths for these packages in the project files.
Since we supply prebuilt binaries for all supported languages this is
generally not required. Project files for the language wrappers are
only supplied for Microsoft Visual C++, but not any other compiler.
