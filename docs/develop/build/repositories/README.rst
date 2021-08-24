HaikuPorts build-packages repository
====================================

The ``build/jam/repositories/HaikuPorts`` directory contains
RemotePackageRepository files which detail packages and repositories
leveraged during Haiku’s build process.

   Warning: The URL packages are obtained from are determined by the
   sha256sum of the repository file.

Updating
--------

Each RemotePackageRepository jam file in this directory is processed by
src/tools/hardlink_packages.py on the HaikuPorts package server.

1) Latest RemotePackageRepository jam file in git is downloaded on
   package server.
2) Packages are added to HaikuPorts by automatic or manual means.
3) hardlink_packages is provided all the relevant directories and
   RemotePackageRepository file
4) hardlink_packages performs additional modification of the
   RemotePackageRepository and creates build repositories
   (https://eu.hpkg.haiku-os.org/haikuports/master/build-packages/)
5) The modified RemotePackageRepository file is copied back to the
   developers system and checked in to git.

Container Process
-----------------

Here is the fastest way to update this as of today. Improvements are
needed. Replace (ARCH) with architecture, (USER) with your non-root
user.

Prepare the build-packages repository
-------------------------------------

as root on limerick.ams3.haiku-os.org…

1) wget
   https://git.haiku-os.org/haiku/plain/build/jam/repositories/HaikuPorts/(ARCH)
   -O /var/lib/docker/volumes/ci_data_master_(ARCH)/_data/(ARCH)
2) Enter the buildmaster container: docker exec -it $(docker ps \| grep
   ci_buildmaster_master_(ARCH) \| awk ‘{ print $1 }’) /bin/bash -l
3) apt update; apt install -y vim python3 python3-pkg-resources
4) edit the repository define, add the needed packages, \_devel
   packages, and add base package to source section.
5) ln -s /var/buildmaster/package_tools/package_repo
   /usr/bin/package_repo
6) export
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/var/buildmaster/package_tools
7) ./package_tools/hardlink_packages.py (ARCH) ./(ARCH)
   /var/packages/repository/master/(ARCH)/current/packages/
   /var/packages/build-packages/master/
8) exit; cp /var/lib/docker/volumes/ci_data_master_(ARCH)/_data/(ARCH)
   /home/(USER)/(ARCH); chown (USER) /home/(USER)/(ARCH);

Pull the repostory file and commit it
-------------------------------------

From your local system…

1) scp -P2222 (USER)@limerick.ams3.haiku-os.org:./(ARCH) ./(ARCH)
2) commit the updated repostory define *without modifying it* in any way
