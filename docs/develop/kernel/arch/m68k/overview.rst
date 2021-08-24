The m68k port
#############

The Motorola 68000 is an old CPU and not a reasonable target for Haiku. However, later models
which are equipped with a memory management unit could work (slowly).

There is work in progress to target Atari, Amiga, and NeXT hardware platforms.

Todo list
=========

- optimization: remove M68KPagingStructures[*]::UpdateAllPageDirs() and just allocate all the kernel page root entries at boot and be done with it. It's not very big anyway.
- possibly other optimizations in the VM code due to not supporting SMP?

Target platforms information
============================

.. toctree::

   /kernel/arch/m68k/amiga
   /kernel/arch/m68k/atari
