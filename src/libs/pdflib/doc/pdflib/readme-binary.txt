==========================
PDFlib binary distribution
==========================

This is a binary package containing PDFlib, PDFlib+PDI, and
PDFlib Personalization Server (PPS) in a single binary.
It requires a commercial license to use it. However, the
library is fully functional for evaluation purposes.

Unless a valid license key has been applied the generated PDF
output will have a www.pdflib.com demo stamp across all pages.
See the PDFlib manual (chapter 0) to learn how to apply a
valid license key.


C and C++ language bindings
---------------------------
The main PDFlib header file pdflib.h plus a PDFlib library
is contained in the distribution.

- On Windows, the DLL pdflib.dll is supplied along with the
  corresponding import library pdflib.lib. In order to build
  and run the supplied C/C++ samples copy these files to the
  bind/pdflib/c or bind/pdflib/cpp directories.

  If you are working with Borland C++ Builder you must first
  generate a new import library pdflib.lib from pdflib.dll
  since Borland tools don't work with the Microsoft .lib format.
  To do so, use the following command:
  coff2omf pdflib.lib pdflib.lib

- On Unix systems a static library is supplied.

The bind/c and bind/cpp directories contain sample applications
which you can use to test your installation.


Other language bindings
-----------------------
Additional files and sample code for various languages
can be found in the bind directory. Note that not all
binary libraries for all language bindings may be present;
see our Web site for additional packages.
