Notes on the PDFlib Tcl binding
===============================

Unix
====

- You may have to change the name of the tclsh binary in the
  example scripts if you want to run them directly. Alternatively,
  you may want to create a symbolic link pointing to the installed
  version of tclsh.


Mac OS X
========

- The default build procedure for the PDFlib Tcl wrapper does not
  work for Tcl 8.3 and above on Mac OS X.
  
  These Tcl versions need a "Mach-O dynamically linked shared library ppc",
  whereas our build process creates a "Mach-O bundle ppc"

  You can build a PDFlib extension for Tcl by applying the following changes:

  - remove the "-module" option for the linking step (you find this in
    ~/config/mkbind.inc)

    Note: Undo this change to build other PDFlib language bindings.

  - now libtool only accepts libraries starting with "lib", therefore:
    -> apply the following change in bind/pdflib/tcl/Makefile:
         LIBNAME        = pdflib_tcl$(LA)
       to
         LIBNAME        = libpdf_tcl$(LA)

  - change in pkgIndex.tcl:
    pdflib_tcl -> libpdf_tcl

Alternatively, you can request a precompiled binary for Tcl on OS X from
bindings@pdflib.com.  Note that this service is only available for users
with a commercial PDFlib license, and not PDFlib Lite.


How to build the PDFlib wrapper for Tcl 8.0 or 8.1
==================================================

The precompiled versions of the PDFlib wrapper for Tcl require Tcl 8.2 or
above. The wrapper code can also be compiled with Tcl 8.0 or 8.1, although
is is not supported by the configure and Makefile process.
Do the following for building PDFlib with Tcl 8.0 or 8.1:

- #undef USE_TCL_STUBS in pdflib_tcl.c

- Add -ltcl to the linker command, and an appropriate -L option.
