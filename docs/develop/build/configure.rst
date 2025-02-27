The configure script
====================

The configure script is a simple shell script that will set up the required configuration for
building Haiku. It does not use autotools or any other similar system, mainly because the Haiku
build supports only a limited set of host architectures, and is largely self-contained.

The configure script is also responsible for building the GCC cross-compiler, a compiler that runs
on your host machine, but can create executables that will eventually run on Haiku. During later
parts of the build, both this and the host compiler will be used, as several tools need to be run
on the host machine during the build process.
