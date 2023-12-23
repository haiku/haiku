Haiku Git Repositories
======================

Haiku uses Git for source control, combined with Gerrit for review of code changes.

Most of the operating system sources are stored in a single repository at http://cgit.haiku-os.org/haiku .

Another repository at http://cgit.haiku-os.org/buildtools contains the build tools, that is, gcc,
binutils, and Jam, which are maintained by Haiku developers.

`Additional repositories <https://github.com/orgs/haiku/repositories>`_ are hosed on GitHub.

Finally, some pre-compiled packages are downloaded during the build, these are built using
Haikuporter from `recipes available here <https://github.com/orgs/haikuports/repositories>`_.

Getting the sourcecode
----------------------

 * https://www.haiku-os.org/guides/building/get-source-git

Sending change reviews
----------------------

 * https://dev.haiku-os.org/wiki/CodingGuidelines/SubmittingPatches
 * https://review.haiku-os.org/Documentation/user-upload.html

Source tree organization
------------------------

The source tree is organized so you can easily find what you look for. If you're already familiar
with Haiku, you will notice that the source directory generally mirrors the way the filesystem in
Haiku is organized.

At the top level, things that need to be "built" in some way are put in the ``src`` directory.
For example, the "data" folder at the root contains files that are used as-is in the disk image,
while "src/data" contain files that need to be compild or converted to different formats, such as
the MIME database.

* src - All files that have to be built

  * add-ons - Everything that will be installed to /boot/system/add-ons: kernel drivers, media codecs, translators, …
  * apps - GUI applications that are not preferences
  * bin - Command-line applications
  * build - Files to allow using the Haiku buildtools on non-Haiku platforms
  * data - Data files of any type: icons, MIME database, …
  * kits - The public C++ API of Haiku: libbe, libmedia, libgame, …
  * libs - Static and shared libraries used by Haiku applications
  * preferences - The preference applications
  * servers - The system servers: app_server, input_server, net_server, …
  * system - The low-level system that makes Haiku tick

    * boot - The bootloaders for all supported platforms
    * glue - The "glue code" that makes shared libraries execute their constructors and destructors, and programs start their execution at ``main()``
    * kernel - The kernel and all of its core services
    * ldscripts - Linker scripts for building various parts of Haiku
    * libnetwork - Files for building libnetwork.so, including the POSIX/BSD socket implementation and some extensions to it, as well as the DNS resolver
    * libroot - Files for building libroot.so, including the standard C and POSIX library implementation
    * runtime_loader: The special application that knows how to load and run other applications from ELF executable files

  * tests - This more or less mirrors the main source tree layout, and contains tests and debugging tools for each component. Some of the tests are run using cppunit, other can be run manually.
  * tools - Tools that can be built on non-Haiku platforms. Either needed for compiling Haiku itself, or otherwise useful outside of Haiku

* headers - All shared, private and public headers

  * build - Compatibility headers for building Haiku code on non-Haiku systems
  * compatibility - Compatbility layers allowing to build BSD and GNU code to run on Haiku
  * config - Platform-specific configuration, definition of standard types that can be used in other places
  * cpp - The C++ standard library headers (only for gcc2, for later gcc versions this is provided by the gcc package)
  * glibc - Headers from glibc, for configuration and definition of some glibc specific functions
  * libs - Headers for the libraries found in src/libs
  * os - The public headers that define the Haiku API
  * posix - The POSIX APIs supported by Haiku
  * private - Private headers that are shared between haiku components, including work-in-progress APIs that may become public in the future
  * tools - Headers for various tools and utilities

* docs - Documentation

  * develop - Internal documentation for developers working on Haiku itself (this is what you are reading now)
  * user - `API reference <https://api.haiku-os.org>`
  * Some other miscellaneous documentation

* build - Build files

  * jam - Jam rules used by the Haiku build, defining how to build an application, a library, a disk image, …
  * config_headers - Configurable headers for enabling various debug features
  * scripts - Various scripts used by the Haiku build process

* 3rd_party - Developers custom files. Used for various side projects from Haiku developers, useful personal scripts, and integration with other tools and projects such as virtualization software

Managing GCC and binutils updates using vendor branches
-------------------------------------------------------

The buidtools repository uses vendor branches. This concept originates from `the SVN Book <https://svnbook.red-bean.com/en/1.8/svn.advanced.vendorbr.html>`_
but applies just as well to Git. This organization allows to clearly separate the imported code
from upstream, and the changes we have made to it.

The idea is to import all upstream changes in a dedicated branch (there are currently two, called
vendor-gcc and vendor-binutils). These branches contains the sources of gcc and binutils as
distributed by the GNU project, without any Haiku changes.

The master branch can then merge new versions from the vendor branches. This allows to use Git
conflict resolution to make sure our patches are correctly ported from one version to the next.

It also makes it easy to compare the current state of our sourcecode with the upstream code, for
example to extract patches that could be upstreamed.

How to import upstream binutils changes
.......................................

Here is an example of the process used to update to a new version of binutils:

.. code-block:: bash

    git checkout vendor-binutils          # Move to the branch containing binutils
    git rm -rf binutils ; rm -rf binutils # Delete the existing version of binutils
    wget http://.../binutils-2.36.tar.xz  # Download the latest version
    tar xf binutils-2.36.tar.xz           # Extract the new binutils version
    mv binutils-2.36 binutils             # Move the extracted files to the right place
    git add -f binutils                   # Add the new files to git
    git commit -m "import binutils 2.36"  # Commit the files in the vendor branch
    git push origin vendor-binutils       # You can push this directly to the branch

Now this can easily be merged into the master branch:

.. code-block:: bash

    git checkout master
    git merge vendor-binutils

Review and fix the conflicts, if any, then push the changes for review on Gerrit.

How to import upstream gcc changes
..................................

Here is an example of the process used to update to a new version of binutils:

.. code-block:: bash

    git checkout vendor-gcc               # Move to the branch containing binutils
    git rm -rf gcc ; rm -rf gcc           # Delete the existing version of binutils
    wget http://.../gcc-13.2.0.tar.xz     # Download the latest version
    tar xf gcc-13.2.0.tar.xz              # Extract the new binutils version
    mv gcc-13.2.0 gcc                     # Move the extracted files to the right place
    pushd gcc
    ./contrib/download_prerequisites      # Download the required gmp, isl, mpfr and mpc dependencies
    rm gmp gmp-6.2.1.tar.bz2              # Remove gmp download and symbolic link
    mv gmp-6.2.1 gmp                      # Move the downloaded gmp dependency in place
    rm isl isl-0.24.tar.bz2
    mv isl-0.24 isl
    rm mpc mpc-1.2.1.tar.gz
    mv mpc-1.2.1 mpc
    rm mpfr mpfr-4.1.0.tar.bz2
    mv mpfr-4.1.0 mpfr
    popd
    git add -f gcc                        # Add the new files to git
    git commit -m "import gcc 13.2.0"     # Commit the files in the vendor branch
    git push origin vendor-binutils       # You can push this directly to the branch

Now this can easily be merged into the master branch:

.. code-block:: bash

    git checkout master
    git merge vendor-binutils

Review and fix the conflicts, if any, then push the changes for review on Gerrit.

Comparing our code with upstream
................................

Comparing the two versions is easy because you can refer to them by branch names:

.. code-block:: bash

    git diff vendor-binutils master -- binutils
