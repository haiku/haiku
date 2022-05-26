# Haiku bootstrap in a container

In Haiku, "bootstrap" is the process of building an Haiku disk image without
any pre-compiled dependencies. Everything is built from sources instead. This
is useful in the following cases:
- Bringing up a new CPU architecture or ABI, for which no packages are yet
  available,
- You do not trust our precompiled binaries used for a normal build.

The resulting image is a very minimal one, containing just enough parts of
Haiku to be able to run haikuporter inside it and generate package files that
can then be used for a normal build of Haiku.

The Haiku bootstrap process is highly dependant on what tools are installed
on the host machine. Bootstraped haikuporter builds can pick up on things
like the locally installed clang vs the gcc toolchain we are providing,
the shell being used (bash works, mksh is known to cause problems), or even
the way the host distribution compiled Python. This makes the process
unreliable when running it outside of a well-knwon environment.

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

```make prep```


3) Build the crosstools (gcc only) for your target architecture

```TARGET_ARCH=arm make crosstools```

4) Begin the bootstrap (building Haiku + the required bootstrap hpkgs)

```TARGET_ARCH=arm make bootstrap```

5) If you need to enter the build environment, ```TARGET_ARCH=arm make enter``` will quickly let you do so.
6) profit!
