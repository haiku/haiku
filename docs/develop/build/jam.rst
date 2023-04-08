The build tool: Jam
===================

Requirements for a build tool
-----------------------------

Building a complete operating system is a somewhat complex task. In the case of Haiku, the build
involves up to 3 different compilers: for the host system, and in the case of hybrid builds, two
different target compilers which are different versions of gcc. Multiple host systems are supported,
with a compatibility layer to be able to use some Haiku specific features like extended filesystem
attributes even on operating systems that don't support them. Various target architectures also
require us to generate versions of our bootloader that run on a wide variety of environments
(BIOS, EFI, OpenFirmware, ...) and uses several executable formats. The work doesn't stop there, as
the build system also includes support for downloading pre-built packages, generating complete
filesystem images, and also allows a large amount of customization.

Typical build systems usually fall in two categories: low-level systems like make or ninja focus
on handling dependencies and running jobs in the correct order, while high-level build systems like
cmake or meson provide pre-made rules for most common tasks, resulting in very simple builds for
typical projects.

Neither of these are quite satisfactory for Haiku: the scale of the project would make it difficult
to handle everything with low-level makefiles, while tools like cmake don't offer nearly enough
control on the compilation process for our needs.

We can summarize these requirements (this isn't a complete list) for an ideal build system:
- Automatic parsing of C++ source files to determine dependencies to header files, and rebuild
automatically if those have changed,
- Ability to build using multiple compilers, and different set of compiler flags for each target,
- Factorization of common commands in a single place, to build many similar targets using the same
process,
- Handling of tasks other than building code,
- Support for multiple steps to build a single target (for example, compiling an executable and
then adding resources to it), ideally with correct dependency handling (add the resources only
if the executable was rebuilt, or the resource file changed)
- Some level of configurability to allow modifying the exact content of the generated filesystem
image.

This resulted in the choice of a less known, but more appropriate tool in the form of Jam. This
was originally developped by Perforce, but today they have abandoned it. Just like Make, there are
various flavors of Jam now available, the most known ones being Boost Build and Freetype Jam.
Haiku provides its own variant, as we needed to extend Jam in various ways to make it work for our
needs.

Short overview of how Jam operates
----------------------------------

The core idea of Jam is to provide generic rules (for example "how to build an application from a
set of source files") and then apply these rules several times. The Haiku build system defines a
number of custom rules, allowing to build code both for Haiku and for the host operating system (to
be run during the compiling process).

For example, to build an application, one may use:

.. code-block:: jam

    Application MyExampleApp                      # Name of the application
        : MyExampleSource.cpp AnotherSource.cpp   # Source code of the app
        : be translation                          # Libraries needed
        : MyExampleApp.rdef                       # Resource file
        ;

This will invoke the "Application" rule which looks something like this (this is a simplified
version of it):

.. code-block:: jam

    rule Application
    {
    	AddResources $(1) : $(4) ;
    	Main $(1) : $(2) ;
    	LinkAgainst $(1) : $(3) ;
    	LINKFLAGS on $(1) = [ on $(1) return $(LINKFLAGS) ]
    		-Xlinker -soname=_APP_ ;
    }

This in turns invokes more rules: AddResources, Main and LinkAgainst, and also tweaks LINKFLAGS
variable for the target. You can see how Jam allows to set variables for a target, which allows
us to decide which compiler and flags to use in each case.

The low level rules will take care of calling the Depends rule to let Jam know how dependencies
are organized and what needs to be built first.

Rules can call each other until eventually we get to a rule that includes an "action" part.
This is where we finally get to actually running commands to do something. For example here is
(again, a simplified version for this example) the Cc rule that runs a C compiler:

.. code-block:: jam

    rule Cc
    {
    	# Make sure the target depends on the sources
    	Depends $(<) : $(>) ;
    
    	# Set variables on the target that will be used by the action
    	on $(1) {
    		local flags ;
    
    		# Enable optimizations when not building in debug
    		if $(DEBUG) = 0 {
    			flags += -O3 ;
    		} else {
    			flags += -O0 ;
    		}
    
    		# Select a compiler (the actual Cc rule will pick a different one depening on what is being
    		# built)
    		CC on $(1) = gcc ;
    
    		# Prepare the command line for building the target
    		CCFLAGS on $(<) = $(flags) ;
    		CCHDRS on $(<) = [ FIncludes $(HDRS) ] ;
    		CCDEFS on $(<) = [ FDefines $(DEFINES) ] ;
    	}
    }
    
    actions Cc
    {
    	# Actually invoke the compiler with the command line flags prepared by the rule
    	$(CC) $(CCFLAGS) -c "$(2)" $(CCDEFS) $(CCHDRS) -o "$(1)"
    }

For building Haiku, Jam is combined with a simple "configure" shell script to do the initial setup.
There is no need for something as complex as autotools, because of the relatively limited number
and diversirty of supported host operating systems (we can assume a reasonably modern UNIX style
system) and the fact that a large part of the build is made with our own compiler and libraries,
which we have full control on.

Jam rules used for building Haiku
---------------------------------

The rules are defined in the build/jam directory. They are spread accross multiple files, each
handling a specific part of the build.

Here is an overview of some of these files and what they are used for.

OverriddenJamRules
    Jam provides a default set of rules for building simple software. Unfortunately, Haiku isn't
    so simple, and a lot of these rules need to be redefined, in particular to handle our setup
    with multiple compilers.

BeOSRules
    Rules specific to BeOS-like operating systems, mainly management of extended attributes and
    executable resources.

MainBuildRules
    Rules for building Haiku applications. This file defines rules like Application, Addon,
    StaticLibrary. It also contains rules for building on the host: BuildPlatformSharedLibrary,
    BuildPlatformMain, etc.

ArchitectureRules
    Management of compiler flags for different CPU architectures and "hybrid builds" where two
    compilers are used.

BootRules
    Rules related to building the bootloaders.

KernelRules
    Rules for building the kernel and kernel add-ons.

