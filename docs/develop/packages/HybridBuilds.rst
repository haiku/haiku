=============
Hybrid Builds
=============
A hybrid build of Haiku used to be a regular Haiku built with gcc 2, also
including the versions of the system libraries built with gcc 4 (or vice versa),
so that programs built with either compiler could be run. Package management
extends that concept and makes it more modular. Since it assigns gcc 2 and gcc 4
different (packaging) architectures ("x86_gcc2" and "x86"), we can now speak of
a primary architecture -- the one the system has been built for -- and a
secondary architecture -- the one the additional set of system libraries has
been built for. This also fits fine with the x86_64+x86 hybrid option we might
see in the future.

Since the files for the secondary architecture will live in one or more separate
packages we gain some flexibility. E.g. one could start out with a non-hybrid
Haiku and install the packages for the secondary architecture later on, and
remove them when no longer needed.  In theory there's nothing preventing us from
supporting multiple secondary architectures (e.g. x86_64+x86+x86_gcc2), though
that might get somewhat confusing in practice.

The following sections list the requirements for the hybrid concept and the
packages built for a secondary architecture.

General Requirements
====================
1. Secondary architecture libraries and add-ons must live in respective
   "<secondary_arch>" subdirectory, where the runtime loader will only look when
   loading a secondary architecture executable.
#. Secondary architecture development libraries and headers must live in
   "<secondary_arch>" subdirectory, where only the secondary architecture
   compiler will look for them.
#. Secondary architecture executables must live in a "<secondary_arch>"
   subdirectory, which by default isn't in PATH. The executables can be
   symlinked to the primary architecture "bin" directory, using a symlink name
   that doesn't clash with the primary architecture executable's name (by
   appending to the name the secondary architecture name, e.g. "grep-x86"). If
   there isn't a corresponding package for the primary architecture, the
   executables may also live directly in the "bin" directory.
#. Application directories should live in a "<secondary_arch>" subdirectory,
   unless there isn't a corresponding package for the primary architecture.

Secondary Architecture Package Requirements
===========================================
1. A secondary architecture package must not conflict with the corresponding
   primary architecture package, so both can be active at the same time and in
   the same installation location.
#. Packages that require a command ("cmd:...") and don't need it to be for a
   particular architecture shouldn't need to be concerned with the
   primary/secondary architecture issue. E.g. "cmd:grep" should provide them
   with a working grep.
#. Secondary architecture packages providing a command should generally provide
   both "cmd:<command>" and "cmd:<command>_<secondary_arch>"
   (e.g. "cmd:grep" and "cmd:grep_x86_gcc2"). The former makes 2. possible. The
   latter allows for packages to explicitly require the command for the
   secondary architecture (3. of the general requirements actually implies that
   provides item). The former should be omitted, when the behavior/output of the
   command is architecture dependent (e.g. in case of compilers etc.).
#. Secondary architecture packages providing a library must provide
   "lib:<library>_<secondary_arch>" (e.g. "lib:libncurses_x86").
