ActivePerl versions
===================

- ActivePerl 5.6 is _not_ binary compatible to older versions of ActivePerl
  with respect to extensions. For this reason the same PDFlib DLL can _not_
  be used with ActivePerl 5.6 and older versions. The "oldperl" directory
  of the PDFlib binary distribution for Windows contains a PDFlib DLL for
  ActivePerl versions before 5.6.


Compiling PDFlib for ActivePerl on Win32
========================================
(not relevant for users of the binary distribution)

The distribution is set up to be compiled with ActivePerl 5.6.

With some minor modification you can also compile with ActivePerl
versions older than 5.6 (the MSVC project contains an "oldperl" configuration
with all of this):

- Run the compiler in C++ mode (required by ActiveState). In MSVC++ 6 there
  doesn't seem to be a GUI option for C++ mode, but the compiler switch
  /Tp works.

- Change the Perl include and lib paths appropriately.

- Change perl56.lib to perlcore.lib.
