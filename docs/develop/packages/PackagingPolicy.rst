================
Packaging Policy
================
This document defines the policy for creating packages.

Multiple Packages per Software
==============================
Installation files of a software shall be put into separated packages according
to their purpose. For a software "foo":

- Package "foo": Contains all runtime files, i.e. everything needed to "run" the
  software. This may include executables (e.g. executable "bin/foo"), shared
  libraries (e.g. "lib/libfoo.so"), data files
  (e.g. "data/foo/foo-runtime-data").
- Package "foo_devel": Contains only the files needed for development. This
  includes header files and static libraries. Shared libraries are not to be
  included. Instead the package must declare package "foo" with the matching
  version as a requirement.
- Package "foo_doc": Contains the documentation for using "foo".
- Package "foo_devel_doc": Contains the documentation for "foo" development,
  like API documentation etc.
- Package "foo_debuginfo": Contains the debug information for the package "foo".

If multiple packages are defined, they must not contain any common files.

If a software contains client and server software that can be used independently
from each other, two separate sets of packages shall be created.

Provides Declaration
====================
A package "foo" must declare the following provided resolvables:

- "foo=<version>" with <version> naming the exact version of the package.
- "cmd:<name>=<version>" for each executable <name> with version <version>
  installed in "bin/". This includes a declaration for "cmd:foo", if there is an
  executable named like the package.
- "lib:<name>=<version>" for each shared library <name> (not including a suffix,
  e.g. "libfoo") with version <version> installed in "lib/".
- "devel:<name>=<version>" for each library <name> (not including a suffix,
  e.g. "libfoo") with version <version> for which development files (library
  symlinks in "develop/lib" and header files in "develop/headers") are included.

Any instance of '-' in <name> shall be replaced by '_'. If the backward
compatibility of a resolvable is known, a "compat >= <compatibleVersion>" shall
be added accordingly.

Documentation
=============
If a package "foo" provides documentation (which it should, of course), in many
cases that can be provided in different formats:

- Any kind of user documentation belongs in a subdirectory of "documentation"

  - Man pages is the preferred format for terminal and should be installed into
    the corresponding folders in the subdirectory "man".
  - Info files are provided by many packages. If at all desirable, they should
    be installed into the subdirectory "info". One problem with info files is
    that all packages currently contain a file named "documentation/info/dir",
    which supposedly is the list of all available info files, but since each
    package provides an instance of this file containing only its own info
    files, an arbitrary dir file is made visible via packagefs. The file should
    therefore not be include in a package.
  - Other documentation for a package foo -- HTML, a simple ReadMe, sample
    documents, PDFs, etc. -- goes into subdirectory "packages/foo". If it is
    likely that multiple versions of a package may be installed, then a version
    string (as appropriate just major, major and minor, or even full (but no
    revision)) should be appended, e.g. "package/foo-2" or "package/foo-2.13".

- For a package foo_devel developer documentation, except man and info pages,
  should go into "develop/documentation/foo". A version string may be appended
  to the directory name as well. When it is unclear what is developer
  documentation or it isn't really possible to separate it from user
  documentation "documentation/foo" should be used.

Data Files
==========
Data files for a package foo shall generally be placed in a directory
"data/foo". If it is likely that multiple versions of a package may be
installed, then a version string shall be appended. Data (but not
settings/configuration) files generated at run-time shall be placed in
"cache/foo" or "var/foo", depending on the kind of data the files contain. For
data files, both read-only or generated, that are shared between different
packages/software a differently named subdirectory may be used as appropriate
(e.g. font files are placed in "data/fonts").

Writable and Settings Files and Directories
===========================================
All global writable files and directories as well as user settings files and
directories that the package includes or the packaged software creates or
requires the user to create shall be declared by the package (via
GLOBAL_WRITABLE_FILES respectively USER_SETTINGS_FILES in the build recipe) in
the following way:

- A user specific settings file shall never be installed on package activation.
  Usually user specific settings files are completely optional. In the rare case
  that a software requires a user specific settings, the user will have to
  create it manually. In either case, if the package includes a template user
  settings file, that should be declared::

    USER_SETTINGS_FILES="settings/foo template data/foo/user-settings-template"

  If no template file is included, the settings file shall still be declared::

    USER_SETTINGS_FILES="settings/foo"

- Since many ported software requires a global settings file or other writable
  files, a default version of such a file can be provided and is automatically
  installed on package activation. In that case the package must also declare
  what shall be done with a user-modified file when the package is updated.
  E.g.::

    GLOBAL_WRITABLE_FILES="settings/foo keep-old"

  "keep-old" indicates that the software can read old files and the
  user-modified file should be kept. "manual" indicates that the software may
  not be able to read an older file and the user may have to manually adjust it.
  "auto-merge" indicates that the file format is simple text and a three-way
  merge shall be attempted. If a default settings file is not included in the
  package, the settings file shall still be declared, just without the
  additional keyword.

In both cases, user settings files and global writable files, the "directory"
keyword can be used to indicate that the given path actually refers to a
directory.

Post-Installation Scripts
=========================
A package may include one or more post-installation scripts. The scripts are
executed whenever the package is activated (for the first time, but also after
package updates and first boot of a newly installed OS). They shall be placed
in "boot/post-install" and declared explicitly by the package (via
POST_INSTALL_SCRIPTS in the build recipe). A post-install script should be
considered the last resort. It should only be used, if there's no reasonable
alternative.  A typical use would be to create a desktop icon that the user
can move around or delete.

Pre-Uninstallation Scripts
=========================
These undo the effects of a post-installation script and usually are put
into "boot/pre-uninstall".  A typical use is to remove desktop icons.
