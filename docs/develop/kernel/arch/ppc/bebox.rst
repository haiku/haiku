Notes on a possible BeBox Haiku port
====================================

Bootloader
----------

The BeBox ROM expects the bootloader to be in PEF format, as was produced by the CodeWarrior
compiler used by Be. However, support for this format in binutils seems incomplete.

references
----------

* http://www.netbsd.org/ports/bebox/
* http://netbsd.2816.n7.nabble.com/BeBox-memory-configuration-td278318.html

QEMU target
-----------

http://qemu-project.org/Features/BeBox
