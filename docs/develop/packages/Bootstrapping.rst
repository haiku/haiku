===================
Bootstrapping Haiku
===================

Even a very basic Haiku requires a set of third-party packages (ICU, zlib,...),
a Haiku sufficiently complete to build software even more
(binutils, gcc, make,...). So whenever something fundamental in Haiku
(the architecture ABI, the ABI of libroot) changes in a binary incompatible way,
or when Haiku is ported to a new architecture, it is necessary to bootstrap
Haiku and a basic set of required third-party packages. This document describes
how this process works.

Prerequisites
=============

- So far the bootstrap build has only been tested on Linux. It will probably
  also work on FreeBSD and other Unixes. Haiku is not yet supported as a build
  platform.
- A required prerequisite for the bootstrap build is a checkout of the
  haikuporter, the haikuports, and the haikuports.cross repositories from the
  `Github HaikuPorts site`_.

  .. _Github HaikuPorts site: https://github.com/haikuports/

- Additional build tools are required for the build platform:

  - autoconf
  - automake
  - cmake (For compiling python_bootstrap)
  - ncurses_development (For compiling texinfo_bootstrap)

- All the usual prerequisites for building Haiku.

Configuring and Building
========================

1. Configure the Haiku build with all usual parameters, but add the
   ``--bootstrap`` option with its three parameters, the paths to the checked
   out haikuporter *script* (eg. ``../haikuporter/haikuporter``),
   haikuports.cross, and haikuports repositories folders::

     .../configure ... --bootstrap path/to/haikuporter path/to/haikuports.cross path/to/haikuports

   It is important that you build outside the tree, as otherwise the build will
   fail!
#. Build a bootstrap Haiku image::

     jam -q @bootstrap-raw

#. Boot the bootstrap Haiku (e.g. in a virtual machine), edit the file
   "/boot/home/haikuports/haikuports.config" -- entering your email address in
   the "PACKAGER" field -- and finally open a Terminal and build the third-party
   packages::

     cd haikuports
     haikuporter --do-bootstrap

In the "packages" subdirectory the built packages are collected. This is the
initial set of packages for the HaikuPorts packages repository, plus the source
packages the Haiku build system put in your generated directory under
"objects/haiku/<arch>/packaging/repositories/HaikuPorts-sources-build/packages"
(ignore the "rigged" source packages). With these packages a HaikuPorts package
repository can be built and in turn a regular Haiku can be built using it.
Further packages can then be built on the regular Haiku.

Further hints:

- Of course, as usual, "-j<number>" can be passed to the jam building the
  bootstrap Haiku. Building the bootstrap third-party packages, which is part of
  this process, will take quite some time anyway. Since those packages are built
  sequentially, the jam variable "HAIKU_PORTER_CONCURRENT_JOBS" can be defined
  to the number of jobs that shall be used to build a package.
- Instead of "bootstrap-raw" the build profile "bootstrap-vmware" can be used as
  well. You can also define your own build profile, e.g. for building to a
  partition. As long as its name starts with "bootstrap-" that will result in a
  bootstrap Haiku.
- ``haikuporter`` also supports the "-j<number>" option to specify the number of
  jobs to use. Even on real hardware this step will nevertheless take a long
  time.
- The jam variable HAIKU_PORTER_EXTRA_OPTIONS can be defined to any options that
  should be passed to ``haikuporter`` (``--debug`` is handy for showing python
  strack traces, for instance).

How it works
============
Building the bootstrap Haiku image is in principle quite similar to building a
regular Haiku image, save for the following differences:

- Some parts of a regular Haiku that aren't needed for building packages are
  omitted (e.g. the Demos, MediaPlayer, the OpenGL API,...).
- Certain third-party packages that aren't needed for building packages are
  omitted as well.
- The third-party packages are not downloaded from some package repository.
  Instead for each package a bootstrap version is built from the sources using
  ``haikuporter`` and the respective build recipe from haikuports.cross.
- ``haikuporter`` itself and ready-to-build ("rigged") source packages for all
  needed final third-party packages are copied to the image.

Obviously the last two points are the juicy parts. Building a bootstrap
third-party package -- unless it is a pure data package -- requires certain
parts of Haiku; usually the headers, libroot and other libraries, and the glue
code. For some Haiku libraries we do already need certain third-party packages.
So there's a bit of ping pong going on:

- Initially the build system builds a package
  "haiku_cross_devel_sysroot_stage1_<arch>.hpkg". It contains the essentials for
  cross-compiling bootstrap third-party packages, but nothing that itself
  depends on a third-party package.
- Once all third-party packages required for it have been built, a more complete
  "haiku_cross_devel_sysroot_<arch>.hpkg" is built. It is used to cross-compile
  the remaining third-party packages.

The rigged source packages (and regular source packages) are built via
``haikuporter`` from the regular haikuports repository checkout. haikuports
contains build recipes for a lot of software. Which source packages should be
built is determined by the build system by checking what packages are needed for
the target build profile used by the bootstrap process. This defaults to
"minimum-raw", but it can be changed by setting the jam variable
"HAIKU_BOOTSTRAP_SOURCES_PROFILE"
(i.e. ``jam -sHAIKU_BOOTSTRAP_SOURCES_PROFILE=@release-raw -q @bootstrap-raw``
will include source packages for all packages needed by release image).

Haiku Architecture Ports
========================
When preparing a new Haiku architecture port for the bootstrap build the
following things need to be considered:

- There need to be repository definitions
  "build/jam/repositories/HaikuPorts/<arch>" and
  "build/jam/repositories/HaikuPortsCross/<arch>". The former lists the packages
  available for a regular Haiku, i.e. it must include at least the packages
  needed for a basic Haiku image that can build third-party packages. The latter
  lists the available bootstrap third-party packages.
- There needs to be "src/data/package_infos/<arch>/haiku", a package info for
  the Haiku system package (currently also used for the bootstrap package).
- In the haikuports.cross repository all build recipes need to support the
  architecture (the architecture must be listed in the "ARCHITECTURES"
  variable). Some software may need to be patched for cross-building to work for
  the architecture.
- In the haikuports repository all build recipes for required software need to
  support the architecture.

If the Haiku architecture port doesn't support a working userland yet, the
process obviously cannot go further than building the bootstrap Haiku image.
