Filesystem drivers
====================================

Filesystem drivers are in src/add-ons/kernel/file_system

A filesystem usually relies on an underlying block device, but that's not
required. For example, NFS is a network filesystem, so it doesn't need one.

Implementation notes
--------------------

Each filesystem driver must define a few structures which act as the
interface between the VFS and the filesystem implementation. These
structures contain function pointers, some of which are optional,
which should point to functions defined by the implementer that
perform the appropriate filesystem peration as defined by the
documentation.

See docs/user/drivers/fs_interface.dox for more detailed documentation
of this interface.

It's important to note that while there may be some similarities in
the interface with that of other operations systems, one should not
make any assumptions about the desired behavior based soley on the
function prototypes defined in fs_interface.h.

The following is a list of notes calling out some potential mistakes.

# fs_vnode_ops.read_symlink

Defining this function means that the filesystem driver supports
symbolic links, and the function that f_vnode_ops.read_symlink points
to should read the contents of a symlink from the specified node.

This may seem similar to the posix function readlink(), but it is
slightly different. Unlike readlink(), which returns the number of
bytes copied into the output buffer, fs_vnode_ops.read_symlink is
expected to always return the length of the symlink contents, even if
the provided buffer is not large enough to contain the entire symlink
contents.

Development tools
-----------------

# fs_shell

It is not convenient to test a filesystem by reloading its driver into a
running Haiku system (kernel debugging is often not as easy as userland).
Moreover, the filesystem interacts with other components of the system
(file cache, block cache, but also any application reading or writing files).

For the early development steps, it is much easier to run the filesystem code
in a more controlled environment. This can be achieved through the use of
a "filesystem shell": a simple application that runs the filesystem code, and
allows performing specific operations through a command line interface.

Example of fs_shell implementations are available under src/tests/add-ons/kernel/file_systems/
for the bfs and btrfs filesystems.

For example, to build the fs_shell for btrfs, use

   jam -q "<build>btrfs_shell"

To run it, use

   jam run objects/haiku_host/x86_gcc2/release/tests/add-ons/kernel/file_systems/btrfs/btrfs_shell/btrfs_shell [arguments]

You need to pass at least a file or device containing a filesystem image as an
argument. You need some tool to create one. It is possible to work using an
actual disk volume (but be careful, it's risky to use one with useful data in it),
a file, or a RAM disk, depending on what you are doing.

# userlandfs

As a second step, it's possible to use the filesystem as part of a runing
system, while still running it in userland. This allows use of Debugger,
memory protection, and in general any kind of userland debugging or tracing
tool. When the filesystem crashes, it does not bring down the whole system.

Userlandfs can run the filesystem code using the same interface as the kernel,
therefore, once everything is working with userlandfs, running the filesystem
as kernel code is usually quite easy (and provides a performance boost)

# Torture and performance tests

Once the basic operations are working fine, it is a good idea to perform more
agressive testing. Examples of scripts doing this are available in
src/tests/add-ons/kernel/file_systems/ for the fat and ext2 filesystems.

.. toctree::

   /file_systems/ufs2
   /file_systems/befs/resources
