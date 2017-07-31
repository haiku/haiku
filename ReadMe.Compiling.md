Building Haiku
==========================
This is a overview into the process of building HAIKU from source.
An online version is available at <https://haiku-os.org/guides/building/>.

Official releases of Haiku are at <https://haiku-os.org/get-haiku>.
The (unstable) nightly builds are available at <https://download.haiku-os.org/>.

We currently support the following platforms:
 * Haiku
 * Linux
 * FreeBSD
 * Mac OS X

Required Software
----------------------------
Tools provided within Haiku's repositories:
 * `jam` (Jam 2.5-haiku-20111222)
 * Haiku's cross-compiler (needed only for non-Haiku platforms)

The tools to compile Haiku will vary, depending on the platform that you are
using to build Haiku. When building from Haiku, all of the necessary
development tools are included in official releases (e.g. R1 alpha4) and in the
nightly builds.

 * `git`
 * `ssh` (for developers with commit access)
 * `gcc`/`g++` and binutils (`as`, `ld`, etc., required by GCC)
 * (GNU) `make`
 * `bison` (2.4 or better)
 * `flex` and `lex` (usually a mini shell script invoking `flex`)
 * `makeinfo` (part of `texinfo`, only needed for building GCC 4)
 * `autoheader` (part of `autoconf`, needed for building GCC)
 * `automake`
 * `gawk`
 * `nasm`
 * `wget`
 * `[un]zip`
 * `cdrtools` (preferred) or `genisoimage`
 * case-sensitive file system

Whether they are installed can be tested by running them in a shell with
the `--version` parameter.

The following libraries (and their respective headers) are required:
 * `curl`
 * `zlib`

### Haiku for ARM
If you want to compile Haiku for ARM, you will also need:

 * `mkimage` (<http://www.denx.de/wiki/U-Boot/WebHome>)
 * Mtools (<https://gnu.org/software/mtools/intro.html>)

### On Mac OS X

Disk Utility can create a case-sensitive disk image of at least 3 GiB in size.
The following ports need to be installed:
 * `expat`
 * `gawk`
 * `gettext`
 * `libiconv`
 * `gnuregex`
 * `gsed`
 * `cdrtools`
 * `nasm`
 * `wget`
 * `less`
 * `mpfr`
 * `gmp`
 * `libmpc`
 * `bison` (updated to the latest version)

More information about individual distributions of Linux and BSD can be found
at <https://haiku-os.org/guides/building/pre-reqs>.

Downloading Haiku's sources
--------------------------------------------------
There are two parts to Haiku's sources &mdash; the code for Haiku itself and a set
of build tools for compiling Haiku on an operating system other than Haiku.
The buildtools are needed only for non-Haiku platforms.

Anonymous checkout:
```
git clone https://git.haiku-os.org/haiku
git clone https://git.haiku-os.org/buildtools
```
(You can also use the `git://` protocol, but it is not secure).

If you have commit access:
```
git clone ssh://git.haiku-os.org/haiku
git clone ssh://git.haiku-os.org/buildtools
```

Building Jam
-------------------------------------------
(*This step applies only to non-Haiku platforms.*)

Change to the `buildtools` folder and run the following commands to
generate and install `jam`:
```
cd buildtools/jam
make
sudo ./jam0 install
```
Or,  if you don't want to install `jam` systemwide:
```
./jam0 -sBINDIR=$HOME/bin install
```

Configuring the build
-------------------------------------
The `configure` script generates a file named `BuildConfig` in the
`generated/build` directory. As long as `configure` is not modified (!) and the
cross-compilation tools have not been updated, there is no need to call it again.
For rebuilding, you only need to invoke `jam` (see below). If you don't
update the source tree very frequently, you may want to execute `configure`
after each update just to be on the safe side.

Depending on your goal, there are several different ways to configure Haiku.
You can either call configure from within your Haiku trunk folder. That will
prepare a folder named 'generated', which will contain the compiled objects.
Another option is to manually created one or more `generated.*` folders and run
configure from within them. For example, imagine the following directory setup:
```
buildtools-trunk/
haiku-trunk/
haiku-trunk/generated.x86gcc2
```

### Configure a GCC 2.95 Hybrid, from a non-Haiku platform
```bash
cd haiku-trunk/generated.x86gcc2
../configure --use-xattr-ref \
	--build-cross-tools x86_gcc2 ../../buildtools/ \
	--build-cross-tools x86
```

### Configure a GCC 2.95 Hybrid, from Haiku
```
cd haiku-trunk/generated.x86gcc2
../configure --target-arch x86_gcc2 --target-arch x86
```

Additional information about GCC Hybrids can be found on the website,
<https://haiku-os.org/guides/building/gcc-hybrid>.

### Configure options
The various runtime options for configure are documented in its onscreen help
```bash
./configure --help
```

Building via Jam
----------------------------

Haiku can be built in either of two ways, as disk image file (e.g. for use
with emulators, to be written directly to a usb stick, burned as a compact
disc) or as installation in a directory.

### Running Jam

There are various ways in which you can run `jam`:

 * If you have a single generated folder, you can run 'jam' from the top level of Haiku's trunk.
 * If you have one or more generated folders, (e.g. generated.x86gcc2),
   you can `cd` into that directory and run `jam`.
 * In either case, you can `cd` into a certain folder in the source tree (e.g.
   src/apps/debugger) and run jam -sHAIKU_OUTPUT_DIR=<path to generated folder>

Be sure to read `build/jam/UserBuildConfig.ReadMe` and `UserBuildConfig.sample`,
as they contain information on customizing your build of Haiku.

### Building a Haiku anyboot file
```
jam -q @anyboot-image
```

This generates an image file named `haiku-anyboot.image` in your output
directory under `generated/`.

### Building a VMware image file
```
jam -q @vmware-image
```
This generates an image file named `haiku.vmdk` in your output
directory under `generated/`.

### Directory Installation
```
HAIKU_INSTALL_DIR=/Haiku jam -q @install
```

Installs all Haiku components into the volume mounted at "/Haiku" and
automatically marks it as bootable. To create a partition in the first place
use DriveSetup and initialize it to BFS.

Note that installing Haiku in a directory only works as expected under Haiku,
but it is not yet supported under Linux and other non-Haiku platforms.

### Building individual components
If you don't want to build the complete Haiku, but only a certain
app/driver/etc. you can specify it as argument to jam, e.g.:
```
jam -q Debugger
```
Alternatively, you can `cd` to the directory of the component you want to
build and run `jam` from there. **NOTE:** if your generated directory is named
something other than `generated/`, you will need to tell `jam` where it is:
```
jam -q -sHAIKU_OUTPUT_DIR=<path to generated folder>
```
You can also force the rebuild of a component by using the `-a` parameter:
```
jam -qa Debugger
```

Running
----------------
Generally there are two ways of running Haiku: on real hardware using a
partition, and on emulated hardware using an emulator (like VirtualBox, or QEMU).

### On Real Hardware
If you have installed Haiku to its own partition you can include this
partition in your bootmanager and try to boot Haiku like any other OS you
have installed. To include a new partition in the Haiku bootmanager, start
the BootManager configurator by running:
```
BootManager
```

### On Emulated Hardware
For emulated hardware you should build disk image (see above). How to set up
this image depends on your emulator. If you use QEMU, you can usually just
provide the path to the image as command line argument to the `qemu`
executable.
