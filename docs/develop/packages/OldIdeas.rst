========================
Package Management Ideas
========================

.. contents::
   :depth: 2
   :backlinks: none

This page is a place to hash out ideas regarding Haiku's package management
(and creation). The following is a draft specification for the package
management system to be included in R1. It is based on (1), (2) and the
discussion in (5). The draft does not yet cover everything from (1), however.

Requirements
============
This section describes the intended user experience.

HaikuBits
---------
HaikuBits is a complete directory of software for the Haiku platform. It is the
one place a user needs when looking for Haiku software. While it does not host
all software binaries, it does list 99.9% of all software available for Haiku.

Software is classified into a number of categories. For each software, HaikuBits
provides a short description, one or more screenshots, a link to the author's
homepage and a download link. Additionally, a software's page has a community
rating and important information such as security notices.

Bundles
-------
A bundle_ is a single file that contains the binaries, data files and
documentation. This makes software self-contained and easy to handle. An
application can be run by double-clicking the bundle icon. Obvious exceptions
to this rule are drivers and libraries. These have to be installed to be of any
use. The contents of a bundle can be inspected by opening the bundle by means of
a context-menu option which opens the bundle in Tracker just like a directory.

.. _bundle: http://en.wikipedia.org/wiki/Application_Bundle

Optionally, an **application** bundle can be **installed** by moving it to
``/boot/apps`` (system-wide) or in ``/boot/home/<user>/apps`` (user-local).
Another option is to right-click the icon and select "install for everyone"
(only admins) or "install only for me". Any initial configuration (accepting a
license) can be performed the first time a bundle is being run.

**Libaries** can be installed the same way. The user normally does not have to
install libraries manually, as the package manager will do so when it is needed,
asking the user for permission.

When an application bundle has been installed, shortcuts to the application
appear in the Deskbar menu. This menu is subdivided into a number of a
predefined categories (games, graphics, internet, ...) that match those on
HaikuBits.

**Drivers** ... ?

  **brecht**: I don't like Waldemar's idea of having bundles spread all around
  the filesystem, hence the clear distinction between installed and
  non-installed bundles. This might be a necessity for multi-user too.

  **axeld**: I have to agree with brecht. And also, I like the package file
  system best, as it also solves on how to deal with ported software, and
  libraries as well. Only drivers would probably need special treatment
  (depends on how early the package file system is available, but I guess that
  could be made work as well, like having a "actually install on disk" option
  for driver packages).

  **wkornewald**: My original intention was to not force the user through any
  installation procedure. Simply download the app and run it directly from the
  downloads folder to see if it works well and if yes the user can move it to
  the "Apps" folder.

Management
----------
**Uninstallation** is performed by simply removing the bundle from
``/boot/apps`` or ``/boot/home/<user>/apps``.

A user's application **settings** are kept when a bundle is uninstalled or
deleted. The system however provides a comprehensive listing of applications for
which user settings exist. The user can choose to delete settings for each of
the bundles.

Application folders (``/boot/apps`` and ``/boot/home/<user>/apps``) display the
list of installed bundles including information (description, availability of
updates, security risk warning), just like the mail folder in BeOS displays
emails.

  **wkornewald**: The system should automatically remove settings of deleted
  apps after a certain amount of time. You don't really want the user to
  manually clean up his system. The cleanup delay should be long enough to allow
  the user to update an app by deleting the old version and downloading the new
  version and it should be long enough to allow the user to "undo" his action.
  Maybe 1-2 months is fine.

  **pulkomandy**: Deleting files without asking doesn't look right to me. Either
  delete them as soon as the app is removed, or keep them forever. I like
  Debian/apt way of doing it : when you delete a package, it's listed in the
  package manager as 'removed, residual config files' and you can remove these
  from there.

Dependencies
------------
The user does not have to worry about dependencies. If a bundle depends on other
bundles, the package manager should make sure they are fulfilled (on run). If a
dependency is not fulfilled, the system will ask the user for confirmation and
automatically install any required bundles. This requires an internet
connection.

Alternatively, a user can download a **fat bundle** that includes the
dependencies as well.

Updates
-------
All *installed software* can be checked for new versions. For each of the
installed applications, the changes with respect to the installed version can be
displayed. Updates are flagged as 'strongly recommended' when security problems
are known. The user can select which applications he/she would like to update
and have the system perform the updates.

By default, the updater tool does not show libraries in order to keep the list
of updates as short as possible and understandable by the non-technical user.
Libaries should only be updated when there are known problems with them.

Implementation
==============
In this section the implementation of the system is discussed.

Bundles
-------
A bundle is a compressed disk image that contains:

- application executable(s) & data
- metadata

  - name
  - version
  - (revision?)
  - hash (integrity-check)
  - author
  - homepage
  - license
  - category (for grouping applications in the Deskbar menu)

- shortcuts to appear in the Deskbar menu

  - a default shortcut to run when the bundle is being "run"
  - right-clicking bundle could offer the option of opening a help document
    about the application

There are a number of different bundle types:

- application
- library
- driver
- font
- ...

Library, driver and font bundles have to be installed.

Install or not?
---------------
As bundles have to be compressed for distribution, they will need to be
uncompressed at some point. There are two options:

- Before using a bundle, it is decompressed. This is very similar to installing,
  which many wanted to avoid.
- The bundle's contents are decompressed on access. This is less efficient as
  decompression needs to be performed on each access, as opposed to only one
  time during installation. Performance of large applications and heavy games in
  particular will suffer.

It is possible to split application bundles into two sub-types; those that have
to be installed, and those that can be run as-is.

  **brecht**: While I initially liked the idea of not having to install
  software, I now feel that it is not suited for all types of applications and
  games. While we can make a differentiate between 'large' and 'small'
  applications and require installation of large apps (or suffer from poor
  performance), this feels like a bit of a kludge. Is is really that bad to
  'install' software once? Installing can be reduced to decompressing and should
  not bother the user much. If the user knows he has to install **all** bundles,
  there can be no confusion.

  **wkornewald**: I don't fully remember my original proposal (it's been several
  years :), but I think there's a middle-path: When the bundle is opened for the
  first time it's decompressed and cached automatically. When the bundle is
  deleted from the file system the cache is cleaned, too. That way you have the
  best of both worlds and the user only has a slow first start, but all
  subsequent app starts will be fast. Maybe the cache itself could be a single
  uncompressed bundle/image file if that's more efficient than having lots of
  small files spread over the main file system. A large coherent file can
  probably be read into memory much faster and should speed up app starts
  noticeably.

Merged
------
By means of a union pkgfs (3). All ports are mounted under ``/boot/common``.

It is not clear how multiple versions of libraries and applications can be
handled in this scheme.

Self-contained
--------------
By means of assignfs (4). Each port receives its own unique assign:
``/boot/apps/<port>-<version>-<revision>``

  **axeld**: I think the best solution would be a unionfs approach: the package
  file system would just blend in the packages where needed. User packages would
  be merged with the contents of config/, while system wide ones would be merge
  with the contents of /boot/common/.

Settings
--------
global settings/user settings

Multiple Application Version
````````````````````````````
how to handle

Dependency Hell
---------------
`Dependency hell`_ is a problem mostly for ports. That does not mean it can be
ignored. At least in the early years of Haiku, ports will be an important source
of software.

.. _Dependency hell: http://en.wikipedia.org/wiki/Dependency_hell

Avoiding
````````
Bundles are always fat bundles. All required libraries are included in the
bundle. Problem solved! However, this very area-inefficient. Nor is it a
realistic solution for bundles that depend on large packages like Python or
Perl.

Tacking
```````
In order to solve conflicting dependencies, it is necessary to be able to have
multiple versions of a library installed. Even worse, some libraries can be
built with different options.

To make this work, it is obvious that a central bundle repository is required:
HaikuBits. Alongside offering a browsable directory of software like BeBits, it
stores information about dependencies. Dependency information (problems arising
from certain combinations of bundles) is updated by the community.

An example. When ABC-1.0 is released, its dependency libfoo is at version
1.2.10. Bundle ABC-1.0 specifies "libfoo >= 1.2.10" as a dependency. Later, when
libfoo 1.2.12 is released, it appears that this breaks ABC-1.0. HaikuBits is
updated to indicate this: "libfoo >= 1.2.10 && != 1.2.12". When ABC-1.0 is now
downloaded from HaikuBits, the bundle contains the updated information. A
software updating tool can also check HaikuBits to see whether dependencies are
still OK.

Because the act of porting can introduce additional incompatibilities, each port
should be tagged with a revision number to uniquely identify it. Revisions can
also be used to differentiate between ports with different build options.
Specifying build options in the dependency information seems overkill anyway, as
we should strive to have as few port revisions as possible (developers should
have dependencies installed as bundles before porting).

The bundle metadata needs to be extended to include information about the
dependencies:

- minimum/maximum version
- preferred version/revision
- non-working versions/revisions

While bundles will not be available for download for retail software, it still
makes sense to record dependency information about it on HaikuBits.

Having an application use a particular library version can be done by
manipulating LD_LIBRARY_PATH or by virtually placing the library in the
applications directory by means of assignfs or pkgfs.

Note the important difference with typical Linux package management systems. In
Linux, the repository typically offers only one version of a particular package.
This is the result of keeping all packages in the repository in sync, in order
to avoid conflicting dependencies. In the proposed system however, the user is
free to install any version of a bundle, as there is no need for any global
synchronization of all bundles.

  **axeld**: while having a central repository is a good thing, I don't think
  our package manager should be based on that idea. I would allow each package
  to define its own sources (the user can prevent that, of course). That way, we
  avoid the situation of having to choose between outdated repositories, and
  unstable software (or even having to build it on your own) like you usually
  have to do in a Linux distribution. The central repository should also be a
  fallback, though, and try to host most library packages.

  Since we do care about binary compatibility, and stable APIs, having a central
  repository is not necessary, or something desirable at all IMO.

  **brecht**: I agree. I see the repository more as a central entity keeping
  track of all software versions and the dependencies between them. This
  dependency information is updated based on user feedback. I don't think it is
  necessary to have the repository be the one and only source for bundles,
  however. It can keep instead a list of available mirrors. However, it is
  probably a good idea to have one large reliable mirror (hosting the most
  important bundles) managed by Haiku Inc. alongside the repository in order not
  to be too dependent on third parties.

libalpm
-------
(and it's tool: pacman)

libalpm is the package management library used on ArchLinux, most people know it
as "pacman" since that's the main tool to use, however, all the functionality is
part of the libalpm library which could be utilized to create a nice GUI
frontend for the package manager. It of course can also be adapted.

It uses libarchive to extract archives, and either libdownload or libfetch to
download files - although one can also have it use an external command, like
curl or wget.

The current status is this:

As far as libfetch is concerned: compiles and is linked to, but it doesn't
really work, so I'm using curl instead - it works like a charm.

The important part: libarchive needed some work to support zip files in a useful
way. Basically, it now supports seeking (which it didn't before), the
central-directory headers for ZIP files (so it supports stuff like symlinks),
and BeOS file attributes! Also, when reading from a source which doesn't allow
seeking (... which are... none - on our case) it simply reads the local headers,
but can also - if explicitly requested - provide "update"-entries to update the
raw data when the central directory is reached (but those are of no importance
anymore).

What's good about libalpm? Well, it provides useful configuring mechanisms, it
stores dependencies and can also give you a list of which packages require a
certain package. It keeps a database containing package information, including a
file list. Configuration files in packages can be listed as such, which causes
them to be installed as \*.pacnew when they are upgraded (unless the new and old
files equal - an md5 sum check is used there.) It provides the ability to use
different database directories which allows us to have an automated way of
creating package bundles. For instance, I can set the installdir to
/tmp/mypackage and install the game "einstein" including its dependencies there,
then move /tmp/mypackage/einstein/common/lib to /tmp/mypackage/einstein/lib,
remove the unnecessary manpages, share files (well, usually anything else which
is in the common/ folder), and then strip those dependencies from einstein's
.PKGINFO file and create a bundled package which I can then install normally to
say /boot/apps.

Another useful feature is the possibility to change the root directory. When a
package contains a .INSTALL script, libalpm chroot()s into the root directory,
cd()s into the installation directory, and then executes the .INSTALL script
(which means, that install-scripts can and should work relative to the
installation directory, although, if necessary, the absolute path is available
in $PWD)

Also, libalpm works similar to an actual database. It doesn't blindly attempt to
install a package, but first check for file conflicts, see if any files need
backups or configuration files need to be installed as .pacnew, and then
installs a package. If you install multiple packages at once, then it only
either installs all of them, or none. It allows you to find the owning package
of a file in the filesystem as well as listing all the files and dependencies of
a package.

Where does it get the packages from? Two possibilities: One can use package
files directly - which could be made in such a way that you could also just
unzip them. In fact, it might be useful to put the .PKGINFO into the zip file as
some extra data which is not unzipped when simply using ``unzip``, although
package creation is easier if it's just a file. The other one being
repositories. The pacman utility currently allows you to list repositories like
this in pacman.conf::

  [core]
  Server = file:///MyRepositories

  [devel]
  Server = file:///MyRepositories

  [public]
  Server = http://www.public-repository.com/

When you synchronize the repository databases, pacman downloads the file
<Server>/<Reponame>.db.tar.gz which contains a list of packages with
dependencies. When you install a package from such a repository, it downloads
them from the very same location: <Server>/<Package File> The repo.db.tar.gz
files are currently created using the tools ``repo-add`` and ``repo-remove``
provided in the pacman package. Those extract information from the .PKGINFO file
and put it into the database which can be used as a repository then.

Here's a little log of using pacman to install a package file, and bundle a
package with dependencies together into one package file.
http://stud4.tuwien.ac.at/~e0725517/using-pacman-on-haiku.log.txt

Pros
````
- It has been used on archlinux for a long time - so it works.
- It's obviously possibly to compile and use it on haiku
- Since most of its functionality is part of a library, it can be reused to
  build a GUI-application utilizing libalpm
- pacman also provides scripts for building packages using a PKGBUILD script.

Cons
````
- Likely to cause unwanted restrictions in the package management system.

..

  **brecht**: I don't have a detailed view of pacman yet, but as far as I can
  see it is very similar to your average Linux package manager. As we want
  software management to be fundamentally different from the way Linux
  distributions handle it (because it simply is not a very elegant solution), I
  don't think pacman is a good choice.

  **Blub**: Let me clarify: I was not suggesting to use 'pacman' as a package
  manager, I was just thinking that its library could be a useful codebase for a
  package-database, to keep track of dependencies, available packages and
  updates. It 'could' be used to unpack/install packages into a 'specified'
  folder, like /boot/apps (and even install dependencies into the same folder if
  wanted), or, it could just as well be used to simply keep track of where which
  package has been installed to without worrying about the actual contents.

  Although when stripping the code to unpack the archives and keeping track of
  their files it is indeed better to create something new.

Brainstorming Results from BeGeistert
=====================================
These features were discussed/written down at BeGeistert:

- Integrated app to add packages
- Knows about repositories
- Defined protocol to add repositories that everyone can use (for example
  through description files with a special MIME type)
- Multiple installations of the same package (if the software supports it)
- Maintain shared libraries
- runtime_loader uses package info to resolve libraries
- Install packages per user
- Repositories support keys to verify packages
- Package database of installed packages
- Packagemanagement API
- Binary diffing for packages
- Export/publish the set of installed packages to another system
- System updates
- Quality of packages (QA integrated into the process of releasing a package)
- Property of live update possible

Package Format
==============
A package format has to meet the following requirements:

- It must be able to store BeOS/Haiku file attributes.
- If the package shall be used directly (i.e. without prior extracting) by a
  package file system, fast random access to the file data must be possible.
  This disqualifies zipped TAR like formats.

[PackageFormat The Haiku Package format] specifies a format meeting these
requirements.

References
==========
\(1) http://www.haiku-os.org/glass_elevator/rfc/installer

\(2) http://www.freelists.org/post/haiku-development/software-management-proposal

\(3) http://www.freelists.org/post/haiku-development/software-organizationinstallation,8 and
http://www.freelists.org/post/haiku-development/software-organizationinstallation,55

  **jonas.kirilla**: I hope it's clear enough in reference 3 (above) that my
  ideas on package management approach it from a different angle. Which may or
  may not overlap with the use of libalpm. FWIW, I'm not ready to endorse all
  aspects of this proposal.

\(4) http://www.freelists.org/post/haiku-development/Pathrelocatable-software-and-assigns

\(5) http://www.freelists.org/post/haiku-development/software-organizationinstallation

Other package managers to steal ideas from:

- 0install_ - probably the most interesting
- klik_
- glick_
- Conary_

.. _0install: http://0install.net/injector-design.html
.. _klik: http://en.wikipedia.org/wiki/Klik_(packaging_method)
.. _glick: http://www.gnome.org/~alexl/glick/
.. _Conary: http://wiki.rpath.com/wiki/Conary

Useful articles:

- `OSNews: Decentralised Installation Systems`_ - article by the 0install author
- `Package management system`_

.. _OSNews\: Decentralised Installation Systems:
   http://www.osnews.com/story/16956/Decentralised-Installation-Systems/
.. _Package management system:
   http://en.wikipedia.org/wiki/Package_management_system

Misc.

- `Integrating OptionalPackages into Haiku's build system`_

.. _Integrating OptionalPackages into Haiku's build system:
   http://lists.ports.haiku-files.org/pipermail/
   haikuports-devs-ports.haiku-files.org/2009-June/000516.html
