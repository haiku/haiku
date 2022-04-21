Haiku compilers
===============

For legacy reason, Haiku uses gcc2 to build parts of the system. This only applies to the x86 32bit
version of Haiku, which maintains binary compatibility with BeOS applications. The other ports of
Haiku are not affected by this, and use a more modern version of GCC (version 10 at the time of the
writing).

The compilers are available in the separate "buildtools" repository. This repository contains gcc
as well as the binutils (assembler, linker, etc). These are modified versions of the GNU tools, to
handle the specifics of Haiku. The gcc2 compiler is still getting some maintenance updates and
bugfixes. For the modern tools, the goal is to eventually re-integrate these changes into the
upstream version.

There is also work in progress to be able to build Haiku with llvm/clang as an alternative to gcc.

Summary of changes made to gcc for Haiku support
------------------------------------------------

This is an incomplete list of the changes.

- The -pthread option does nothing. In Haiku, pthreads are always enabled.
- The -fPIE option does the same thing as -fPIC. In Haiku, executables are also loadable as shared
  libraries because they can be loaded using load_add_on for use as replicants. Using the standard
  gcc implementation of -fPIE would then break, because it generates code that can't be used in a
  shared library. For this reason, -fPIE in Haiku instead does the same thing as -fPIC.
- Header search path in Mac OS is case sensitive. Mac OS can handle both case sensitive and case
  insensitive filesystems. Building Haiku requires a case sensitive filesystem, otherwise it's not
  possible to tell apart headers like string.h and String.h. When compiling gcc for Mac OS, it
  normally has code to force case insensitive comparison of header filenames, no matter what the
  underlying filesystem offers. This is disabled in our version of gcc.
