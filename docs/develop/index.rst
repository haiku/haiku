Welcome to Haiku internals's documentation!
===========================================


Target audience
---------------

This documentation is aimed at people who want to contribute to Haiku by modifying the operating
system itself. It covers various topics, both technical (how things work) and organizational
(patch submission process, for example).

This document might also be useful to application developers trying to understand the behavior of
the operating system in some specific cases, however, the `API documentation <https://api.haiku-os.org>`_ should answer most of
the questions in this area already.

This documentation assumes basic knowledge of C++ and the Be API, if you need more information
about that, please see the `Learning to program with Haiku <https://github.com/theclue/programming-with-haiku/releases/tag/v1.1>`_ book.

Status of this document
-----------------------

The work on this book has just started, many sections are incomplete or missing. Here is a list of
other resources that could be useful:

* The `Haiku website <https://www.haiku-os.org>`_ has several years of blog posts and articles
  documenting many aspects of the system,
* The `Coding guidelines <https://www.haiku-os.org/development/coding-guidelines/>`_ describes how code should be formatted,
* The `User guide <https://www.haiku-os.org/docs/userguide/en/contents.html>`_ documents Haiku from the users' point of view and can be useful to understand how things are supposed to work,
* The `Haiku Interface Guidelines <https://www.haiku-os.org/docs/HIG/index.xml>`_ document graphical user interface conventions,
* The `Haiku Icon Guidelines <https://www.haiku-os.org/development/icon-guidelines>`_ gives some rules for making icons fitting with the style of the existing ones.

Table of contents
-----------------

* :ref:`search`

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   /build/repositories/README
   /apps/haikudepot/server
   /midi/index
   /net/NetworkStackOverview
   /net/HowTo-Synchronize_with_NetBSD
   /packages/README
   /servers/app_server/toc
   /servers/registrar/Protocols
   /kernel/device_manager_introduction
   /kernel/obsolete_pnp_manager
   /kernel/vm/swap_file_support
   /kernel/arch/long_double
   /kernel/arch/arm/overview
   /kernel/arch/m68k/overview
   /kernel/arch/ppc/overview
   /kernel/arch/sparc/overview
   /kernel/fs/node_monitoring
   /file_systems/overview
   /partitioning_systems/sun
   /drivers/disk/ioctls
   /drivers/intel_extreme/generations
   /busses/agp_gart/ReadMe
   /busses/bluetooth/overview
   /busses/sdhci/sdhci_mmc_driver
   /busses/usb/USB_stack_design
   /kernel/boot/boot_process_specs.rst
   /kernel/boot/Debugging_Bootloaders_GEF
   /kernel/pci_serial_debug

