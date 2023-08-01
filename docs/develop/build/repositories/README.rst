HaikuPorts build-packages repository
====================================

The ``build/jam/repositories/HaikuPorts`` directory contains
RemotePackageRepository files which detail packages and repositories
leveraged during Haiku’s build process. While in the standard Haiku/HaikuPorts
package repositories, older package builds are purged at intervals, the build
package repositories have a stable set of packages required during the building
and running of Haiku. This makes sure that non-recent source trees can be
continued to build and run.

   Warning: The URL packages are obtained from are determined by the
   sha256sum of the repository file. This means that when the
   `hardlink_packages.py` script is used to generate a new
   RemotePackageRepository jam file, it must not be modified in any way when
   committing it to the Haiku repository.

Prerequisites
-------------

The actions require server access to Haiku's kubernetes environment by a Haiku
system administrator. Please contact the Haiku system administrators when it is
necessary to create a new set of build packages.

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
needed. Replace (BUILDMASTER) and (ARCH) with the architecture you are creating
the build-packages packages for.

(BUILDMASTER) is one of:

* x86-64
* x86

(ARCH) corresponds with the architectures. Note that where an ARCH like x86_64
might use an underscore, the BUILDMASTER will use dashes.


Prepare the build-packages repository
-------------------------------------

Run the following steps from the shell.

#. Log into the remote haikuporter buildmaster for the target architecture.

   .. code-block:: bash

     kubectl exec -it deployment/haikuporter -c buildmaster-(BUILDMASTER) -- sh

#. Get the current repository jam file for the platform that you want to update

   .. code-block:: bash

     wget https://git.haiku-os.org/haiku/plain/build/jam/repositories/HaikuPorts/(ARCH)

#. If it is necessary to add or remove packages, then make changes to the
   downloaded package file.
#. Make sure that all the scripts and tools can find the libraries.

   .. code-block:: bash

     export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

#. Run the `hardlink_packages.py` script to create a new build-packages
   repository and to update the package file.

   .. code-block:: bash

      /var/sources/haiku/src/tools/hardlink_packages.py \
          (ARCH) \
          /var/packages/repository/master/(ARCH)/current/packages/ \
          /var/packages/build-packages/master/

#. Exit the container and return to your local machine.

   .. code-block:: bash

      exit


Pull the repostory file and commit it
-------------------------------------

When the build-packages repository is created, and the RemotePackageRepository
file is updated, it can be pulled to the local machine and be committed to the
Haiku repository.

Run the following steps from the shell.

#. Fetch the remote file generated in the previous stage and copy it to the
   local machine.

   .. code-block:: bash

     kubectl cp -c buildmaster-(BUILDMASTER) \
         $(kubectl get pods | grep haikuporter | awk ‘{ print $1 }’):./(ARCH) \
         (ARCH)

#. Commit the updated repostory define *without modifying it* in any way.
