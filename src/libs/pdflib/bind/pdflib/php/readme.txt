===================================
Notes on the PDFlib binding for PHP
===================================

The binary distribution of PDFlib supports using PDFlib as a loadable
module (DSO) in PHP. This implies some restrictions, but simplifies PDFlib
deployment a lot. Using PDFlib as a loadable module is the recommended
way for using PDFlib with PHP.

If the loadable module does not fit your needs, the binary distribution 
also includes a precompiled PDFlib library (bind/c/lib/libpdf.*), to be used in 
your custom build process for PHP. This will allow you to integrate PDFlib
(including the PDI functionality) into your own PHP build process.

Please note that only PDFlib Lite is available in source form.
PDFlib, PDFlib+PDI, and the PDFlib Personalization Server (PPS) are
available for commercial licensing only, and therefore require a
binary package.

If your distribution does not contain the expected binaries, please contact
bindings@pdflib.com. We only include binaries for the most common PHP versions
to not overload the download package. Please include the platform and the
exact PHP version you are looking for.

Of course you may also use PDFlib for PHP with the source distribution, but
since there are so many PHP configuration options we will only give a rough
overview how to do this.

Summary of this document:

- "Loadable Module (DSO)"
  The recommended way for using PDFlib with PHP.
  If you could not find a DSO for your PHP version/platform in the distribution
  please contact bindings@pdflib.com.

- "Rebuild PHP"
  If the loadable module does not fit your needs, or if DSO modules are not
  supported on your platform, you can rebuild PHP with PDFlib support.
  Caution: this requires substantially more effort than the loadable module.

- "Samples"
  How to set up the supplied PDFlib examples.


Loadable Module
===============

Use the PDFlib loadable module from the "bind/php/<phpversion>" directory
of the PDFlib binary distribution. The name of the loadable module depends
on the platform:

    - Windows: libpdf_php.dll
    - Linux: libpdf_php.so
    - Other Unix systems: libpdf_php.* with the appropriate shared
      library suffix     

The following conditions must be met in order to use PDFlib as a
loadable module:

    - PHP on your platform must support DSOs. This is the case on Windows
      and many Unix platforms, but not all. For example, PHP does not support
      DSOs on Mac OS X before PHP 4.3.0.
      
    - PDFlib support must not already have been compiled into your PHP version.
      If your PHP already includes PDFlib support (this is the case for
      versions of PHP distributed with many Linux distributions) you must
      rebuild PHP with the "-with-pdflib=no" configure option.
      (*HINT* for maintainers of Linux distributions:
       include PDFlib support for PHP as DSO, this allows easier updates)

    - Several properties of your PHP version must match the loadable module
      of PDFlib. The supplied binary modules for PDFlib have been built as
      follows:
      - nondebug version
      - for supported PHP version numbers see "bind/php/<phpversion>
      - thread-safe (only relevant for Windows)

      If you get an error message similar to the following your PHP version
      number does not match that of the PDFlib module:
	    Warning:  pdf: Unable to initialize module
	    Module compiled with debug=0, thread-safety=1 module API=20001214
	    PHP compiled with debug=0, thread-safety=1 module API=20001222
      Unfortunately new PHP version simply deny loading a wrong module
      whithout any specific error message.

      All of these options must match.
      

If you can't meet the above conditions you must choose to "Rebuild PHP" as
described below (or ask us whether your combination will be available soon).


Installing the module on Windows:

    - Our DLLs have been tested with the binary PHP distribution which is
      available from http://www.php.net.

    - The PDFlib binary distribution for Windows contains several DLLs
      for different versions of PHP. Currently we offer the following
      flavors:

      - "php-4.1.0\libpdf_php.dll" was built for PHP 4.1.0/4.1.1/4.2.0
      - "php-4.2.1\libpdf_php.dll" was built for PHP 4.2.1-4.3.5

    - For the PHP installation please follow the documentation of your
      PHP distribution and copy "bind/php/<your phpversion>/libpdf_php.dll"
      to the directory which is specified in the "extension_dir" line in 
      php.ini.      

Installing the module on Unix:

    - The PDFlib binary distribution for Unix contains several shared libraries
      for different versions of PHP. Currently we offer the following
      flavors:

      - "php-4.1.0\libpdf_php.*" was built for PHP 4.1.0, 4.1.1, 4.2.0
      - "php-4.2.1\libpdf_php.*" was built for PHP 4.2.1-4.3.5

    - Copy the file libpdf_php.* from the directory "bind/php/<phpversion>"
      of the PDFlib binary distribution to the directory which is specified in
      the "extension_dir" line in php.ini.


Using the module:

    - If you decide to load PDFlib each time PHP starts insert one line in
      php.ini:
	  extension=libpdf_php.dll	(on Windows)
      or
	  extension = libpdf_php.so	(on Unix)

      and restart apache, so that the changes are recognized.
      You may check <?phpinfo()?> whether the installation did work. If you
      don't find a PDF section please check your logfiles for the reason.

    - Without the "extension = ..." line in php.ini you must include the
      following line in your PHP scripts:
	  dl("libpdf_php.dll");		(on Windows)
      or
	  dl("libpdf_php.so");		(on Unix)

      In this case your php.ini needs these settings:
	- php.ini must include the line "safe_mode=Off".
	- php.ini must include the line "enable_dl=On".



Rebuild PHP
===========

If the loadable module does not fit your needs, or if DSO modules are not
supported on your platform, you can rebuild PHP with PDFlib support.

WARNING: this option requires substantial experience with building PHP.
It is not a matter of simply calling "make" and waiting for the result.
If you never built PHP from source code it is strongly recommended to
avoid this option.

When recompiling PHP using the library from the binary PDFlib distribution
you have to use the library located in the "bind/c" directory of the PDFlib
distribution.

Note: Building PDFlib for PHP from the source code of PDFlib is not supported
by PDFlib GmbH since we supply precompiled binaries (although it should work,
and therefore you will find hints how to do it below).

Unix
    - Unpack the PDFlib binary distribution to <pdflib-dir>,
      or unpack the PDFlib source distribution to <pdflib-dir>, and
      issue the command

      $ ./configure; make; make install
    
    - Copy some PDFlib support files for PHP to your PHP source tree (check
      the section "Support Files" below to see which files apply to your
      PHP version.
        
      $ cp <pdflib-dir>/bind/php/ext/pdf/<somefiles> <php-dir>/ext/pdf

      and rebuild the PHP configure script in the PHP directory (only
      neccesary if you decided to use our config.m4 script):

      $ ./buildconf
    
    - For rebuilding PHP add the following to your PHP configure options:
	  --with-pdflib=<pdflib-dir>/bind/c

      or if building PDFlib from source:
	  --with-pdflib[=<pdflib-install-directory>]

      where <pdflib-install-directory> will typically be something like
      /usr/local or similar (the directory where "lib" and "include" for PDFlib
      reside).

    - Now rebuild PHP as usual, and install it.

Windows
    - Create PDFlib.lib from the PDFlib sources. Change the project settings
      to create a "Multithreaded DLL" library named pdflib.lib.

    - Copy the required PDFlib support files (see below) for PHP to your
      PHP source tree.
        
      C:\> copy <pdflib-dir>\bind\php\ext\pdf\<somefiles> <php-dir>\ext\pdf
      
    - Now rebuild libpdf_php.dll.


Support files
    The following files have to be copied from bind/php/ext/pdf to the ext/pdf
    directory of your PHP source tree. Replacing config.m4 is optional, but
    the config.m4 supplied by us is much simpler and therefor makes less
    troubles if you manage to go through the buildconf process.

    Which files are needed depends on the PHP version you use:

    PHP 4.1.0-4.1.1
	    config.php-406+.m4	-> config.m4
	    pdf.c		-> pdf.c
	    php_pdf.h		-> php_pdf.h

    PHP 4.2.1-4.2.3
	    config.php-406+.m4	-> config.m4
	    pdf.c		-> pdf.c
	    php_pdf.h		-> php_pdf.h

    PHP 4.3.0-4.3.5
	    pdf.c		-> pdf.c
	    php_pdf.h		-> php_pdf.h


Samples
=======

To use the samples:

    - Copy some files:
      $ cp bind/php/*.php .../htdocs # (to your htdocs directory)
      $ cp bind/data/* .../htdocs/data # (to your htdocs)

    - point your browser to the sample files

    - enjoy the generated PDFs
