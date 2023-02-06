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

   /build/index
   /release/index
   /libroot/index
   /midi/index
   /net/index
   /packages/README
   /servers/app_server/toc
   /servers/registrar/Protocols
   /kernel/index
   /file_systems/index
   /drivers/index
   /apps/haikudepot/server

