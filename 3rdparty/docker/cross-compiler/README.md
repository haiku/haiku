# A docker image suitable for cross-compiling Haiku applications

This docker image provides an environment suitable to build Haiku applications
inside a Linux compiler. It can be used by projects willing to integrate an
Haiku build inside their CI system, for example.

The docker build of this image prepares the environment by:
- Building our toolchain
- Building the haiku hpkg files and downloading dependencies
- Extracting headers and libraries from the hpkg files to generate a sysroot
  directory
- Setting up environment variables for the toolchain to be usable

You can then use the $ARCH-unknown-haiku compiler (for example
arm-unknown-haiku-gcc) to build your application. All the required files are
installed in /tools/cross-tools-$ARCH.
