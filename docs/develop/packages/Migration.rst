===============================
Migration to Package Management
===============================
This document gives an overview of what changes with the migration to package
management. It has sections for different groups of Haiku users. All applying
sections should be read in order.

Changes for Users
=================
- Almost all software lives in packages and is only virtually extracted. The
  virtually extracted package content is read-only.
- Software (i.e. packages) can be installed via the command line package manager
  ``pkgman`` -- ``pkgman search/install/uninstall/update ...`` searches for,
  installs, uninstalls, and updates packages respectively. Packages can also be
  installed manually by moving (not copying) them to the respective "packages"
  subdirectory in "/boot/system" or "/boot/home/config".
- The directory layout has changed and many directories have become read-only.
  Cf. `DirectoryStructure`_ for details.

  .. _DirectoryStructure: DirectoryStructure.rst

- The Deskbar menu works differently. It uses a new virtual directory
  Tracker/Deskbar feature to generate its content. Any package can contribute
  Deskbar menu entries by including respective symlinks in "data/deskbar/menu".
  The virtual directory merges the respective directories from all installation
  locations plus the user settings directory
  "/boot/home/config/settings/deskbar/menu". That means whenever a package is
  installed/removed its Desbar menu entries will be added/removed automatically.
  The user settings directory allows the user to add new entries manually. The
  whole behavior can be changed by overriding the default virtual directory.
  Before using the default virtual directory
  "/boot/system/data/deskbar/menu_entries" Deskbar first looks for
  "/boot/home/config/settings/deskbar/menu_entries". This can be a virtual
  directory as well, or a regular directory, or a symlink to either. E.g. making
  it a symlink to the "menu" directory will cause Deskbar to use only the
  contents of that directory, i.e. the menu contents is completely user-defined.
- The MIME type management works a bit differently now as well. The database
  entries for the default MIME types are included in the system package and
  those for application MIME types are included in the package containing the
  respective applications. Neither of those can therefore be removed. By editing
  them in the FileTypes preferences application they can be overridden, though.
  ATM there are still a few known bugs and missing features -- e.g. application
  MIME types aren't automatically added/removed when installing/removing a
  package, and the MIME type removal functionality in FileTypes needs to be
  reworked.
- Haiku's stage 1 boot loader (the boot block in the BFS partition) has changed.
  That means a Haiku partition made bootable from an old Haiku -- or more
  generally: with a ``makebootable`` that predates package management -- will
  not be able to boot a package management Haiku. You will have to run the new
  ``makebootable`` to make it bootable again. The new ``makebootable`` may or
  may not run on an old Haiku. The safest way is to do that from a running
  package management Haiku (e.g. booted off a USB stick or CD).

Changes for Application Developers
==================================
- All development files (headers, libraries, the tool chain) have moved to
  "develop" in the respective installation location. Headers live in
  "develop/headers", development libraries in "develop/lib". Development
  libraries means besides static libraries also symlinks to shared libraries.
  The shared libraries themselves as well as all symlinks required to run a
  program using the library (at most one symlink per library -- the soname) live
  in "lib".
- Commands, libraries, add-ons, and headers for the secondary architecture of a
  hybrid Haiku live in an "<arch>" subdirectory of their usual location. This
  doesn't hold for the system headers which exist only in the primary location.
- ``setgcc`` is gone. The commands of the tool chain for the secondary
  architecture (by default) live in "/boot/system/bin/<arch>". Prepending that
  path to the ``PATH`` environment variable would make them shadow the
  respective primary architecture commands -- the effect would be similar to the
  one ``setgcc`` had, but only for the current shell session. Executing the new
  command ``setarch <arch>`` will start a new shell with a respectively modified
  ``PATH``. The commands of the secondary tool chain are also available in the
  standard path with a name suffixed with "-<arch>" (e.g. "gcc-x86" for the
  gcc 4 executable on a gcc2/gcc4 hybrid).
- Software can be packaged using the ``package`` tool. Cf. `BuildingPackages`_
  for more information.

  .. _BuildingPackages: BuildingPackages.rst

- The ``find_directory()`` API has been partially deprecated. While there are
  still some use cases where it should be used, in many cases the new
  ``find_path*()`` API, respectively the ``BPathFinder`` class should be used
  instead (cf. the `API documentation`_).

  .. _API documentation: https://www.haiku-os.org/docs/api/FindDirectory_8h.html

Changes for Haiku Developers
============================
- Hybrid builds no longer require two separate generated directories. Instead
  the build is configured with both compilers and all output files are put in a
  single generated directory.
- The notion of a packaging architecture has been introduced. It is mostly
  synonymous with the architecture, save for x86 where "x86_gcc2" refers to
  x86 gcc 2 and "x86" to x86 gcc 4.
- Several ``configure`` script option have changed:

  - ``--build-cross-tools`` and ``--build-cross-tools-gcc4`` have been merged.
    The (packaging) architecture must always be specified.
  - ``--build-cross-tools`` and ``--cross-tools-prefix`` can be given multiple
    times to specify hybrid builds. Only for the first ``--build-cross-tools``
    the path to the build tools must be given.

  For example, for building the default configuration of Haiku from a file
  system with proper xattr support, your configure options could look like
  this::

    $ ./configure --build-cross-tools x86_gcc2 ../buildtools --build-cross-tools x86 --use-xattr

- The new option ``--target-arch`` has been introduced for use on Haiku for
  builds with the native compiler. By default, if neither
  ``--build-cross-tools`` nor ``--cross-tools-prefix`` are specified the build
  is configured for a (hybrid) configuration matching the host system's (i.e.
  on a gcc2/gcc4 hybrid the build is configure for that configuration as well,
  on a pure gcc4 Haiku you'd get a gcc4 build). ``--target-arch`` overrides
  the default, allowing to specify the architecture to build for. The option
  can be given multiple times to specify a hybrid configuration. E.g.
  "--target-arch x86_gcc2 --target-arch x86" specifies a gcc2/gcc4 hybrid and
  can be used on a gcc2/gcc4 or gcc4/gcc2 Haiku.
- The new option ``--use-xattr-ref`` can be used when extended attributes are
  available, but their size limit prevents use of ``--use-xattr`` (e.g. with
  ext4). The build system will use a slightly different version of the generic
  attribute emulation via separate files that involves tagging the attributed
  files with a unique ID, so there cannot be any mixups between attributes or
  different files when the inode ID of a file changes or files with attributes
  get deleted without removing their attribute files.
- Configuring a gcc 2 build should now also work on a 64 bit system (without a
  32 bit environment). Tested only on openSUSE 12.3 so far, but should also work
  on other Linux distributions and Unixes. The ``--use-32bit`` should therefore
  be superfluous.
- build/jam has experienced some reorganization, particularly with respect to
  Haiku images and (optional) packages:

  - Most stuff that is built ends up in the "haiku.hpkg" and "haiku_devel.hpkg"
    packages (or the respective "haiku_<arch>.hpkg", "haiku_<arch>_devel.hpkg"
    packages for the secondary architecture). The contents of the packages is
    defined in the respective files in the "packages" subdirectory.
  - The files defining the contents of the Haiku images live now in the "images"
    subdirectory.
  - The "repositories" subdirectory defines external repositories. Most relevant
    for a regular build is the HaikuPorts repository. For each architecture
    there is a file defining the contents of the repository. Changes in that
    file require a respective version of the repository to be built. Currently
    that has to be done manually on the haiku-files.org server. The process will
    be automated soon.
  - ReleaseBuildProfiles is now DefaultBuildProfiles.

- The optional packages are mostly gone. There are only a few meta optional
  packages left. Adding regular packages to the image is done via the
  AddHaikuImagePackages rule. The parameters are package names (all lower case)
  without the version.
- All build variables that depend on the architecture and aren't only relevant
  to the primary architecture have been renamed to have a "_<arch>" suffix (e.g.
  TARGET_GCC_<arch>, TARGET_DEFINES_<arch>, etc.). The variables are mostly only
  used by rule implementations, so this has not that much of an impact on
  Jamfiles.
- There are new build variables HAIKU_PACKAGING_ARCHS and
  TARGET_PACKAGING_ARCH[S]. The plural versions are set to the list of all
  configured architectures, e.g. for a gcc2/gcc4 hybrid "x86_gcc2 x86".
  TARGET_PACKAGING_ARCH is set to the current architecture. Usually that means
  the primary architecture. In some cases (mostly for libraries) a target has to
  be built for all architectures. That is done in a loop which sets
  TARGET_PACKAGING_ARCH (and other variables) according to the architecture
  handled in that iteration. Cf. `src/kits/textencoding/Jamfile`_ for a small
  example.

  .. _src/kits/textencoding/Jamfile:
     https://github.com/haiku/haiku/blob/master/src/kits/textencoding/Jamfile

- Build features (as defined in "build/jam/BuildFeatures") work differently now.
  Instead of build variables there are dedicated rules to deal with build
  features (FIsBuildFeatureEnabled, UseBuildFeatureHeaders,
  BuildFeatureAttribute). Cf.
  `src/add-ons/mail_daemon/inbound_protocols/pop3/Jamfile`_ for an example.

  .. _src/add-ons/mail_daemon/inbound_protocols/pop3/Jamfile:
     https://github.com/haiku/haiku/blob/master/src/add-ons/mail_daemon/
     inbound_protocols/pop3/Jamfile
- The semantics of the "update" build profile action has changed somewhat, since
  due to the packages we now have two container levels, the image and the
  package. A ``jam -q @alpha-raw update libbe.so`` will first update libbe.so in
  the haiku.hpkg package and then update haiku.hpkg in the image. A
  ``jam -q @alpha-raw update haiku.hpkg`` will update "haiku.hpkg" in the image,
  but "haiku.hpkg" will not be rebuilt. If that is desired, it first has to be
  rebuilt explicitly -- via ``jam -q haiku.hpkg``. Note that this might be
  problematic as well, since which optional build features are active depends on
  the specified build profile.
- There's a new build profile action "update-packages". It updates all packages,
  empties "/boot/system/packages" in the image, and copies the updated packages
  there. It's a poor man's system update. Packages you have installed manually
  will be removed. The old "update-all" build profile action still exits. It has
  the effect of "update-packages" and additionally replaces all other files that
  are usually copied to the image.

Changes for Porters
===================
- The format of the recipe (formerly bep) files has changed. Many recipes have
  not been updated yet. haikuporter also has changed significantly. Cf. the
  `haikuporter documentation`_ for more information.

  .. _haikuporter documentation:
     https://github.com/haikuports/haikuports/wiki/HaikuPorterForPM
