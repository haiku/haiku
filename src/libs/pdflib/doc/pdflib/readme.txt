================================================
PDFlib - A library for generating PDF on the fly
================================================

Portable C library for dynamically generating PDF ("Adobe Acrobat") files,
with support for many other programming languages.

The PDFlib distribution is available from http://www.pdflib.com

PDFlib is a library for generating PDF files. It offers an API with
support for text, vector graphics, raster image, and hypertext. Call PDFlib
routines from within your client program and voila: dynamic PDF files!

PDFlib is available on a wide variety of operating system platforms,
and supports many programming languages and development environments:

- C
- C++
- Cobol
- COM (Visual Basic, ASP, Windows Script Host, Delphi, and many others)
- Java via the JNI, including servlets and JSP (but not EJB)
- .NET framework (VB.NET, ASP.NET, C# and others).
- Perl
- PHP Hypertext Processor
- Python
- RPG
- Tcl

An overview of PDFlib features can be found in the PDFlib reference manual
in the PDF file PDFlib-manual.pdf. Documentation for the COM and .NET
editions is available separately.


PDFlib flavors
==============
The PDFlib software is available in different flavors (see the PDFlib
manual for a detailed comparison):

- PDFlib Lite
  Open-source edition for basic PDF generation, free for personal use.
  PDFlib Lite does not support all languages, and is not available on
  EBCDIC platforms.

- PDFlib
  The commercial edition adds various features for advanced PDF generation.

- PDFlib+PDI
  Includes PDFlib plus the PDF Import library PDI which can be used to
  integrate pages from existing PDF documents in the generated output

- PDFlib Personalization Server (PPS)
  Includes PDFlib+PDI, plus advanced block processing functions for
  easily personalizing PDF documents. PPS also includes the Block
  plugin for Adobe Acrobat which can be used to create PDFlib blocks
  interactively. The Block plugin is distributed in a separate installer.


First Steps with a binary Package
=================================
PDFlib, PDFlib+PDI, and PPS are available in binary form, and require
a commercial license. All of these products are available in a single
library, and can be evaluated without any restrictions without any
license. However, unless a valid license key is applied a demo stamp
will be generated across all pages.
The binary packages support C plus various other language bindings.
Instructions for using these packages can be found in doc/readme-binary.txt.


First Steps with the PDFlib Lite Source Package
===============================================
PDFlib Lite is available in source form, and can be used for free under
certain conditions. Source code is also available for selected language
wrappers. If you are working with a source code package you need an ANSI C
compiler. Detailed instructions for building PDFlib from source code
can be found in doc/readme-source-*.txt


Quick Start and Documentation
=============================
For a jump-start, take a look at the PDFlib samples which are available
for all supported languages:

- The following examples work with all editions of PDFlib:
  The hello, pdfclock, and image samples generate PDF output with simple text,
  vector graphics, and images. The chartab examples contains a more advanced
  sample for using fonts and encodings in PDFlib. It can also be used to
  create handy character set reference tables.

- The invoice and quickreference examples require PDFlib+PDI, and demonstrate
  how to deal with existing PDF documents.

- The businesscard example requires the PDFlib Personalization Server (PPS),
  and contains a simple personalization example.

After reviewing these samples you should take a look at the main PDFlib
documentation: the "PDFlib Reference Manual" is included as PDF in all packages.
It is the definite reference for using PDFlib. The majority of questions
will be answered in this manual.


Other PDFlib resources
======================
In addition to the PDFlib reference manual the following resources
are available:

- The PDFlib FAQ collects information about known bugs, patches,
  and workarounds: http://www.pdflib.com

- The PDFlib mailing list discusses PDFlib deployment in a variety of
  environments. You can access the mailing list archives over the Web,
  and don't need to subscribe in order to use it:
  http://groups.yahoo.com/group/pdflib

- Commercial PDFlib licensees are eligible for professional product
  support from PDFlib GmbH. Please send your inquiry along with your
  PDFlib license number to support@pdflib.com.


Submitting Bug Reports
======================
In case of trouble you should always check the PDFlib Web site
in order to see whether your problem is already known, or a patch exists.
If not so, please observe the following:

If you have trouble with running PDFlib, please send the following
information to support@pdflib.com

- a description of your problem
- the platform in use
- the PDFlib version number you are using
- the language binding you are using, along with relevant version numbers
- relevant code snippets for reproducing the problem, or a small PDF file
  exhibiting the problem if you can't construct a code snippet easily
- sample data files if necessary (image files, for example)
- details of the PDF viewer (if relevant) where the problem occurs

If you have trouble compiling the PDFlib Lite source code, please send the
following information to support@pdflib.com:

- a description of your problem and the platform in use
- the PDFlib version number you are using
- the output of "./libtool --config" (Unix systems only)
- most welcome: suggested patches or solutions, other helpful information


A Shameless Plug
================
My book contains a lot of information on PostScript, Fonts, and PDF
(currently only available in German):

Die PostScript- & PDF-Bibel
Thomas Merz, Olaf Druemmer
654 Seiten, ISBN 3-935320-01-9, Euro 25,-
Kopublikation PDFlib GmbH/dpunkt Verlag
PDF available at http://www.pdflib.com
e-mail orders for the printed book: books@pdflib.com


Licensing and Copyright
=======================
THIS IS NOT PUBLIC DOMAIN OR FREEWARE SOFTWARE!

PDFlib Lite can freely be used for non-profit personal use.
The license text can be found in the file PDFlib-Lite-license.pdf.

PDFlib, PDFlib+PDI, and PPS can only be used under the terms of
a commercial license, and always require a license fee. Details
of the license can be found in the file PDFlib-license.pdf.
Licensing information is available in the file PDFlib-purchase-order.pdf,
and on our Web site www.pdflib.com.



Please contact us if you're interested in obtaining a commercial PDFlib license:

PDFlib GmbH
Tal 40, 80331 Munich, Germany
fax +49/89/29 16 46 86

License inquiries: sales@pdflib.com

Support for PDFlib licensees: support@pdflib.com


Technical inquiries if you have not licensed PDFlib:
mailing list and archives at http://groups.yahoo.com/group/pdflib

Copyright (c) 1997-2004 PDFlib GmbH and Thomas Merz.  All rights reserved.
PDFlib and the PDFlib logo are registered trademarks of PDFlib GmbH.
