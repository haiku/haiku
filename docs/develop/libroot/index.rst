The standard C library
######################

Library organization
====================

The C library is, unlike in other POSIX systems, called libroot.so. It contains functions typically
found in libc, libm, libpthread, librt, as well as a few BeOS extensions.

It does not contain functions related to sockets networking, which are instead available in the
separate libnetwork.so.

POSIX, BSD and GNU extensions
=============================

overview
--------

The C library tries to follow the specifications set by the C standard as well as POSIX. These
provide a complete list of functions that should be implemented in the standard C library. The
library should not export any other functions, or if it does, they should be in a private namespace,
for example by prefixing their names with an underscore.

The idea is to avoid symbol name conflicts: applications are allowed to use any function names they
want, and there is a risk that the standard library accidentally uses the same name.

However, because the standards are a bit conservative, they often don't include functions that
would be very useful. Historically, other operating systems have provided non-standard extensions
in their C libraries. In particular, this is common in both glibc and BSD C libraries.

In Haiku, such extensions will be protected by a preprocessor ifdef guard, declared in separate
headers, and, whenever possible, made available in separate libraries. This allows the main C
library (libroot) to be more strictly conforming to the C and POSIX standards, while still making
popular additions available to applications.

features.h
----------

The file headers/compatibility/bsd/features.h is used to enable the declaration of these additions
in system header files.

It does so by defining the _DEFAULT_SOURCE variable (for "reasonable defaults"), which is enabled
if either of the following conditions are satisfied:

- The _GNU_SOURCE or _BSD_SOURCE preprocessor define is defined.
- The compiler is NOT in strict mode (__STRICT_ANSI__ is not defined) and _POSIX_C_SOURCE is not defined

Typically the following common cases will be used:

- The compiler is run without specific compiler flags: the default in gcc is to enable GNU extensions,
  so __STRICT_ANSI__ is not defined. As a result, the extended functions are available.
- The compiler is run with -D_POSIX_C_SOURCE: compiler GNU extensions to C and C++ are enabled,
  but extended functions are not available.
- The compiler is set to use a strict standard (for example -std=c11). In this case, __STRICT_ANSI__
  is defined, and the extended functions are not available.
- The compiler is set to use a strict standard, but either _BSD_SOURCE or _GNU_SOURCE is defined.
  In this case, the C and C++ language extensions are disabled, but the extra functions are available.

header files organization
-------------------------

In addition to the _DEFAULT_SOURCE guard, the nonstandard functions are declared in separate headers,
found in headers/compatibility/bsd and headers/compatibility/gnu, depending on where the function
was first introduced.

These directories are inserted before the standard ones (such as headers/posix). Since the extensions
are usually added in existing headers, these headers are overridden in these directories.

The overriden header uses #include_next (a gcc extension) to include the original one. It then
defines any extensions available.

There is a problem with this: #include_next is itself a nonstandard feature. So, in order to use a
fully standard compiler which would not recognize it, the compatibility headers directories should
be removed from the include paths.

libgnu and libbsd
-----------------

Moving the declarations out of the system headers is fine, but it is not enough. The function
definitions should also be moved out of libroot. So they are implemented in separate libraries,
libgnu and libbsd. Applications using these extended functions will need to link with these.

weak symbols
------------

In some cases, the code can't easily be moved out of libroot. There are various cases where this
can happen, for example:

- Other code in libroot depends on the nonstandard function
- The functions was in libroot in BeOS and can't be removed without breaking BeOS apps

In these cases, the function will be provided with an underscore at the start of its name. This
moves it to a namespace where libroot is allowed to define anything it needs. Then, a non-prefixed
version can be exported as a weak symbol. This allows applications to define their own version of
the function, since the one in libroot is "weak", the application one will be used instead for the
application code. Since code in libroot references the underscore-prefixed function only, it will
not be affected by this, and will still call the libroot-provided function.

This creates extra complexity, so, whenever possible, the functions should be moved to separate
libraries instead.

BeOS and Haiku specific functions
=================================

In addition to the standard C library, libroot also contains functions specific to BeOS and Haiku.
Unfortunately, no precautions were taken with these, and they can conflict with application defined
symbols.

We can't change this for symbols existing from BeOS, as it would break binary compatibility. We
may need to change this in R2. However, we should keep this in mind when adding new functions to
libroot. There is no well-defined solution for this currently, but the strategies documented above
can be used if needed.
