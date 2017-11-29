HaikuPorts repository files
==========================

This directory contains RemotePackageRepository files
which detail packages and repositories leveraged during
Haiku's build process.

> Warning: The URL packages are obtained from
> are determined by the sha256sum of the repository
> file.

Updating
-------

Each RemotePackageRepository jam file in this directory
is processed by src/tools/hardlink_packages.py on the
HaikuPorts package server.

1) Latest RemotePackageRepository jam file in git is downloaded on package server.
2) Packages are added to HaikuPorts by automatic or manual means.
3) hardlink_packages is provided all the relevant directories and RemotePackageRepository file
4) hardlink_packages performs additional modification of the RemotePackageRepository and creates
   build repositories (https://eu.hpkg.haiku-os.org/haikuports/master/build-packages/)
5) The modified RemotePackageRepository file is copied back to the developers system and checked in to git.
