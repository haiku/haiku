Notes on long double support
============================

The “long double” type is different on each architecture. Depending on
the available hardware and ABI conventions, performance compromises,
etc, there may be many implementations of it. Here is a summary for our
convenience.

128-bit IEEE
------------

Platforms: Sparc, ARM64, RISC-V

This is the standard long double type from IEEE754. It has 1 sign bit,
15 exponent bit, and 112 fractional part bits. It is the natural
extension of the 64bit double.

Sparc specifies this type in their ABI but no implementation actually
has the instructions, they instead trigger a trap which would software
emulate them. However, gcc short circuits this by default and calls C
library support functions directly.

.. _bit-ieee-1:

64-bit IEEE
-----------

Platforms: ARM

This is the same representation as plain “double”. ARM uses this for
simplicity.

80-bit
------

Platform: x86, x86_64, m68k

This intermediate format is used by x86 CPUs internally. It may end up
being faster than plain double there. It consists of a 64bit fractional
part, 15 exponent bits, and 1 sign bit. This is convenient because the
fractional part is a relatively easy to handle 64bit number.

m68k uses a similar format, but padded to 96 bits (the extra 16 bits are
unused).

double double
-------------

Platforms: PowerPC?

This is also a 128bit type, but the representation is just two 64bit
doubles. The value is the sum of the two halves. This format allows
faster emulation than a “true” 128bit long double, and the precision is
almost as good.
