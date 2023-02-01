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

## Building the image

The Dockerfile accepts four arguments when building the image:

* `BUILDTOOLS_REV` is the branch/tag of buildtools that should be built. It defaults to `master`.
* `HAIKU_REV` is the branch/tag of the haiku repository that should be built. It defaults to `master`.
* `ARCHITECTURE` is the primary architecture to build the tools and library for. It defaults to `x86_64`
* `SECONDARY_ARCHITECTURE` is the secondary architecture. Leave it empty in case it is not necessary.

For example, the r1beta4 images are build with the following commands:

```bash
# r1beta4 on x86 hybrid, with gcc2 as the primary architecture, and modern gcc as the secondary
podman build --build-arg BUILDTOOLS_REV=r1beta4 --build-arg HAIKU_REV=r1beta4 --build-arg ARCHITECTURE=x86_gcc2 --build-arg SECONDARY_ARCHITECTURE=x86 --tag docker.io/haiku/cross-compiler:x86_gcc2h-r1beta4 .

# r1beta4 on x86_64
podman build --build-arg BUILDTOOLS_REV=r1beta4 --build-arg HAIKU_REV=r1beta4 --build-arg ARCHITECTURE=x86_64  --tag docker.io/haiku/cross-compiler:x86_64-r1beta4 .
```
