The build system
================

Building a complete operating system is a somewhat complex task. Simple tools like GNU make would
result in a lot of problems and hard to maintain scripts if used this way.

Haiku uses a slightly more elaborate tool called Jam. Jam was initially developed by Perforce, but
they have now abandoned the product. As a result, Haiku currently maintains its own fork of Jam
with several customizations.

The core idea of Jam is to provide generic rules (for example "how to build an application from a set
of source files") and then apply these rules several times. The Haiku build system defines a number
of custom rules, allowing to build code both for Haiku and for the host operating system (to be run
during the compiling process).

The build system also offers various ways to configure and customize your Haiku disk image.


.. toctree::

   /build/sourcecode
   /build/compilers
   /build/repositories/README
