The PowerPC port
================

PowerPC was the first non-x86architecture for which a port of Haiku was attempted. The initial
target was the (then recently released) Mac Mini, but of course the BeBox was in everyone's mind
as a possible target for this port.

This port went as far as starting the kernel, but then difficulties in implementing the Mac Mini
PCI bus driver stopped it.

Later on, some work as done on adding support for the Sam460ex development board, after a donation
of one to one of the Haiku developers.

Recently, the lack of easily available and affordable PowerPC hardware has reduced interest in this
port.

Platform specific details
-------------------------

.. toctree::

   /kernel/arch/ppc/bebox
   /kernel/arch/ppc/mac
   /kernel/arch/ppc/sam460ex
