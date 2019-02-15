# Haiku bootstrap in a container

The Haiku bootstrap process is highly dependant on what tools are installed
on the host machine.  Bootstraped haikuporter builds can pick up on things
like the locally installed clang vs the gcc toolchain we are providing.

By running bootstrap within a container, we can better isolate the process
from the end users host and create more-reproduceable bootstrap builds.

> This is designed for GCC bootstraps. In theory if Haiku changed to clang,
> the need for a crosstools toolchain is removed... however the clang work
> is too early to know exactly how this process will work.

## Requirements

1) docker
2) make
3) An internet connection

## Process

1) Build the docker container

```make```

2) Check out the required sources

```make init```


3) Build the crosstools (gcc only) for your target architecture

```TARGET_ARCH=arm make crosstools```

4) Begin the bootstrap (building Haiku + the required bootstrap hpkgs)

```TARGET_ARCH=arm make bootstrap```

5) If you need to enter the build environment, ```TARGET_ARCH=arm make enter``` will quickly let you do so.
6) profit!
