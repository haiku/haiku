=================
Building Packages
=================

This page provides information regarding the package building process. The first
section documents building a package with the low level command ``package``. The
second section refers to building packages with the ``haikuporter`` tool.

Building a Package with the "package" Command
=============================================
The package file format is specified in detail in a `separate document`_. This
section presents information from the perspective of how to build a package file
with the ``package`` command.

.. _separate document: FileFormat.rst

An hpkg file is an archive file (just like tar or zip files) that additionally
contains package meta information in a separate section of the file. When
building an hpkg file via the ``package`` command the meta information must be
provided via a ``.PackageInfo`` file. For convenience, the file itself is added
to the archive as well and can be extracted later, but it will be ignored by
packagefs.

The ``.PackageInfo`` file must be located in the top directory that is archived.
A ``package`` invocation usually looks like that::

  package create -C foo-4.5.26-1 foo-4.5.26-1-x86.hpkg

or (packaging a gcc2 build from within the folder)::

  cd foo-4.5.26-1
  package create ../foo-4.5.26-1-x86_gcc2.hpkg

The argument of the ``-C`` option specifies the directory whose contents to
archive (by default the current directory), the remaining argument is the path
of the package file to be built.

The .PackageInfo
----------------
The contents of the ``.PackageInfo`` adheres to a restricted driver settings
syntax. It consists of name-value pairs, following this simple grammar::

  package_info	::= attribute*
  attribute	::= name value_list
  value_list	::= value | ( "{" value* "}" )
  value		::= value_item+ ( '\n' | ';' )

``name`` can be one of the attribute names defined below. ``value_item`` is
either an unquoted string not containing any whitespace characters or a string
enclosed in quotation marks (``"`` or ``'``) which can contain whitespace and
also escaped characters (using ``\``).

The supported attributes are:

- ``name``: The name of the package, not including the package version. Must
  only contain ``entity_name_char`` characters.

  ::

    entity_name_char	::= any character but '-', '/', '=', '!', '<', '>', or whitespace

- ``version``: The version of the package. The string must have the ``version``
  format (see the `Version Strings`_ section).
- ``architecture``: The system architecture the package has been built for. Can
  be either of:

  - ``any``: Any architecture (e.g. a documentation package).
  - ``x86``: Haiku x86, built with gcc 4.
  - ``x86_gcc2``: Haiku x86, built with gcc 2.

- ``summary``: A short (one-line) description of the package.
- ``description``: A longer description of the package.
- ``vendor``: The name of the person/organization publishing this package.
- ``packager``: The name and e-mail address of person that created this package
  (e.g. "Peter Packman <peter.packman@example.com>").
- ``copyrights``: A list of copyrights applying to the software contained in
  this package.
- ``licenses``: A list of names of the licenses applying to the software
  contained in this package.
- ``urls``: A list of URLs referring to the packaged software's project home
  page. The list elements can be regular URLs or email-like named URLs (e.g.
  "Project Foo <http://foo.example.com>").
- ``source-urls``: A list of URLs referring to the packaged software's source
  code or build instructions. Elements have the same format as those of
  ``urls``.
- ``flags``: A list of boolean flags applying to the package. Can contain any
  of the following:

  - ``approve_license``: This package's license requires approval (i.e. must be
    shown to and acknowledged by user before installation).
  - ``system_package``: This is a system package (i.e. lives under
    "/boot/system").

- ``provides``: A list of entities provided by this package. The list elements
  must have the following format::

    entity	::= entity_name [ "=" version_ref ] [ ( "compat" | "compatible" ) ">=" version_ref ]
    entity_name	::= [ entity_type ":" ] entity_name_char+
    entity_type	::= "lib" | "cmd" | "app" | "add_on"

  See the `Version Strings`_ section for the ``version_ref`` definition.
  The first ``version_ref`` specifies the version of the provided entity. It
  can be omitted e.g. for abstract resolvables like "web_browser". The
  ``version_ref`` after the "compat"/"compatible" string specifies the oldest
  version the resolvable is backwards compatible with.
  The ``entity_type`` specifies the type of entity provided: a library ("lib"),
  a command line program ("cmd"), an application ("app"), or an add-on
  ("add-on").
- ``requires``: A list of entities required by this package. The list elements
  must have the following format::

    required_entity	::= entity_name [ version_operator version_ref [ "base" ] ]
    version_operator	::= "<" | "<=" | "==" | "!=" | ">=" | ">"

  See the `Version Strings`_ section for the ``version_ref`` definition. If
  "base" is specified, the specified entity is the base package for this
  package. The package manager shall ensure that this package is installed in
  the same installation location as its base package.
- ``supplements``: A list of entities that are supplemented by this package
  (i.e. this package will automatically be selected for installation if the
  supplemented entities are already installed). The list elements must have the
  ``required_entity`` format.
- ``conflicts``: A list of entities that this package conflicts with (i.e. only
  one of both can be installed at any time). The list elements must have the
  ``required_entity`` format.
- ``freshens``: A list of entities that are being freshened by this package
  (i.e. this package will patch one or more files of the package(s) that provide
  this entity). The list elements must have the ``required_entity`` format.
- ``replaces``: A list of entities that are being replaced by this package (used
  if the name of a package changes, or if a package has been split). The list
  elements must have the ``entity_name`` format.
- ``global-writable-files``: A list of global writable file infos. The list
  elements must have the following format::

    global_writable_file_info	::= path [ "directory" ] [ "keep-old" | "manual" | "auto-merge" ]

  ``path`` is the relative path of the writable file or directory, starting with
  "settings/" or any other writable directory. If the "directory" keyword is
  given, the path refers to a directory. If no other keyword is given after the
  path respectively after the "directory" keyword, the file or directory is not
  included in the package. It will be created by the software or by the user.
  If a keyword is given, the file or directory (a default version) is included
  in the package and it will be extracted on package activation. The keyword
  specifies what shall happen when the package is updated and a previous default
  version of the file or directory has been modified by the user:

  - "keep-old": Indicates that the software can read old files and the
    user-modified file or directory should be kept.
  - "manual": Indicates that the software may not be able to read an older file
    and the user may have to manually adjust it.
  - "auto-merge": Indicates that the file format is simple text and a three-way
    merge shall be attempted (not applicable for directories).

- ``user-settings-files``: A list of user settings file infos. The list elements
  must have the following format::

    user_settings_file_info	::= path [ "directory" | "template" template_path ]

  ``path`` is the relative path of the settings file or directory, starting with
  "settings/". It is not included in the package. However, if ``template_path``
  is specified, it is a path to a file included in the package that can serve as
  a template for the settings file. It doesn't imply any automatic action on
  package activation, though. If the "directory" keyword is given, the path
  refers to a settings directory (typical when a program creates multiple
  settings files).
- ``users``: A list of specifications for Unix users the packaged software
  requires. The list elements must have the following format::

    user:	::= name [ "real-name" real_name ] "home" home_path [ "shell" shell_path ] [ "groups" group+ ]

  ``name`` is the name of the Unix user, ``real_name``, if specified, the real
  name of the user, ``home_path`` the path to the user's home directory,
  ``shell_path`` the path to the user's shell, and ``group+`` is a list of the
  names of Unix groups the user is a member of (first one is their primary
  group). If the respective components are not specified, ``name`` is also
  used as the user's real name, "/bin/bash" is the path of the user's shell,
  and the user will belong to the default user group.
- ``groups``: A list of names of Unix groups the packaged software requires.
- ``post-install-scripts``: A list of paths of files included in the package,
  which shall be executed on package activation. Each path must start with
  "boot/post-install/". All the files in that directory are also run on first
  boot after installing or copying the OS to a new disk.
- ``pre-uninstall-scripts``: A list of paths of files included in the package,
  which shall be executed on package deactivation. For consistency, each path
  should start with "boot/pre-uninstall/".

Version Strings
---------------
Versions strings are used in three contexts: For the package version, for
resolvable versions (``provides``), and in dependency version expressions
(``requires``, ``supplements``, ``conflicts``, ``freshens``). They are
structurally identical, with the exception that the former requires a revision
component (``version``), while the latter two don't (``version_ref``)::

  version	::= major [ "." minor [ "." micro ] ] [ "~" pre_release ] "-" revision
  version_ref	::= major [ "." minor [ "." micro ] ] [ "~" pre_release ] [ "-" revision ]
  major		::= alphanum_underline+
  minor		::= alphanum_underline+
  micro		::= alphanum_underline_dot+
  pre_release	::= alphanum_underline_dot+
  revision	::= positive_non_zero_integer

The meaning of the major, minor, and micro version parts is vendor specific. A
typical, but not universal (!), convention is to increment the major version
when breaking binary compatibility (i.e. version a.d.e is backwards compatible
to version a.b.c for all b.c <= d.e), to increment the minor version when adding
new features (in a binary compatible way), and to increment the micro version
for bug fix releases. There are, however, projects that use different
conventions which don't imply that e.g. version 1.4 is backwards compatible with
version 1.2. Which convention is used is important for the packager to know, as
it is required for a correct declaration of the compatibility versions for the
provided resolvables. The compatibility version specifies the oldest version the
provided resolvable is backwards compatible with, thus implying the version
range requested by a dependent package the resolvable can satisfy. When
following the aforementioned convention a resolvable of version 2.4.3 should
have compatibility version 2 (or, semantically virtually identical, 2.0.0).
Not following the convention 2.4 may be correct instead. If no compatibility
version is specified, the resolvable can only satisfy dependency constraints
with an exactly matching version.

The pre-release part of the version string has special semantics for comparison.
Unlike minor and micro its presence makes the version older. E.g. version
R1.0~alpha1 is considered to be older than version R1.0. When both version
strings have a pre-release part, that part is compared naturally after the micro
part (R1.0.1~alpha1 > R1.0 > R1.0~beta1 > R1.0~alpha2).

The revision part of the version string is assigned by the packager (not by the
vendor). It allows to uniquely identify updated packages of the same vendor
version of a software.

Package File Names
------------------
A package file name should have the following form::

  file_name	::= name "-" version "-" architecture ".hpkg"

Example package file
--------------------
::

  name			example
  version		42.17-12
  architecture		x86_gcc2
  summary		"This is an example package file"
  description		"Haiku has a very powerful package management system. Really, you should try it!
  it even supports muliline strings in package descriptions"
  packager		"John Doe <test@example.com>"
  vendor		"Haiku Project"
  licenses {
  	"MIT"
  }
  copyrights {
  	"Copyright (C) 1812-2013 by John Doe <test@example.com>"
  }
  provides {
  	example = 42.17-12
  	cmd:example = 3.1
  }
  requires {
  	haiku >= r1~alpha4_pm_hrev46213-1
  	lib:libpython2.6 >= 1.0
  }
  urls {
  	"http://example.com/"
  }
  global-writable-files {
  	"settings/example/configurationFile" keep-old
  	"settings/example/servers" directory keep-old
  }
  source-urls {
  	"Download <http://example.com/source.zip>"
  }

Building a Package with "haikuporter"
=====================================
``haikuporter`` is a high level tool for building packages. As input it reads a
build recipe file for a certain version of a software (aka port) and produces
one or more packages, as declared in the recipe. A recipe specifies package
requirements similar to how it is done in a ``.PackageInfo`` file. When asked to
build a port, ``haikuporter`` resolves the respective dependencies and
recursively builds all not-yet-built ports required for the requested port.
``haikuporter`` itself and a large library of recipe files are hosted at
HaikuPorts_. A detailed `documentation for haikuporter`_ and the
`recipe format`_ can also be found there.

.. _HaikuPorts: https://github.com/haikuports/

.. _documentation for haikuporter:
   https://github.com/haikuports/haikuports/wiki/HaikuPorterForPM

.. _recipe format:
   https://github.com/haikuports/haikuports/wiki/HaikuPorter-BuildRecipes
