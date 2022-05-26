==================
Package Management
==================
This is a short index of the available package management related documentation.

- `Package Management Infrastructure`_ provides an overview of what components
  belong to Haiku's package management infrastructure and how they work and
  interact.

  .. _Package Management Infrastructure: Infrastructure.rst

- `Building Packages`_ gives information on various aspects of the package
  building process.

  .. _Building Packages: BuildingPackages.rst

- `Boot Volume Directory Structure`_ outlines the directory structure of a
  package management powered Haiku boot volume.

  .. _Boot Volume Directory Structure: DirectoryStructure.rst

- `Haiku Package File Format`_ specifies in detail the file format of Haiku
  package files (HPKG) and Haiku package repository files (HPKR).

  .. _Haiku Package File Format: FileFormat.rst

- `Packaging Policy`_ defines the policy for creating Haiku packages.

  .. _Packaging Policy: PackagingPolicy.rst

- `Packages Status`_ gives an overview of the packages needed for a Haiku
  standard build and their current status.

  .. _Packages Status: PackagesStatus.rst

- `Hybrid Builds`_ provides some information regarding hybrid builds.

  .. _Hybrid Builds: HybridBuilds.rst

- HaikuDepot_ explains the concept and ideas behind HaikuDepot, the GUI package
  manager in the making.

  .. _HaikuDepot: HaikuDepot.rst

- Migration_ lists the changes that users should expect when migrating to a
  package management Haiku.

  .. _Migration: Migration.rst

- `Bootstrapping Haiku`_ explains the process of bootstrapping Haiku and third-party packages.

  .. _Bootstrapping Haiku: Bootstrapping.rst

- TODO_ is a list of package management related work still to be done.

  .. _TODO: TODO.rst

- `Blog posts`_ on package management (the Batisseur ones are only indirectly
  package management related).

  .. _Blog posts: https://www.haiku-os.org/tags/package-management/

- `(Old) Package Management Ideas`_ is a collection of thoughts and discussions
  regarding package management. It has been partially obsoleted by the progress
  on the package management implementation.

  .. _(Old) Package Management Ideas: OldIdeas.rst

- http://www.youtube.com/watch?v=rNZQQM5zU-Q&list=PL3FFCD4C6D384A302 is a video
  playlist of Ingo and Oliver explaining and demonstrating the package
  management branch at BeGeistert 2011.

Below are links to source code related to Haiku's package management.

- Package management has been merged into "master", so see the Haiku_ and
  Buildtools_ repositories for that

  .. _Haiku: http://cgit.haiku-os.org/haiku/
  .. _Buildtools: http://cgit.haiku-os.org/buildtools/

- HaikuPorts_ contains the build recipes of various ports.

  .. _HaikuPorts: https://github.com/haikuports/haikuports

- haikuports.cross_ contains the minimal set of build recipes to bootstrap a new
  Haiku architecture.

  .. _haikuports.cross: https://github.com/haikuports/haikuports.cross

- haikuporter_ is the tool to create binary packages from build recipes.

  .. _haikuporter: https://github.com/haikuports/haikuporter


.. toctree::
   /packages/BuildingPackages
   /packages/DirectoryStructure
   /packages/FileFormat
   /packages/HybridBuilds
   /packages/Infrastructure
   /packages/Migration
   /packages/PackagingPolicy
   /packages/Bootstrapping
   /packages/TODO
   /packages/OldIdeas
