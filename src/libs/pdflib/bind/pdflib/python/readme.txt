Python binding on Mac OS X
==========================

The build process creates pdflib_py.dylib (as usual on Mac OS X), but Python
wants to have pdflib_py.so. Therefore you must rename pdflib_py.dylib to
pdflib_py.so before you can use the binaries.
