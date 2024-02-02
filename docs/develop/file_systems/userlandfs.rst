.. _Userland FS Page:

UserlandFS: filesystems in userspace
####################################

UserlandFS was initially designed as a development tool allowing to run existing filesystems in
userspace. However, it turns out being able to do that is pretty useful outside of debugging
sessions too.

UserlandFS can load 3 types of filesystems, which are loaded as add-ons in an userlandfs_server
process. The process communicates with the kernel side of userlandfs to get the filesystem requests
and send back the responses.

UserlandFS exposes three different APIs to filesystem add-ons: one compatible with the Haiku kernel,
one compatible with the BeOS one, and one compatible with FUSE.

The communication between the kernel and userlandfs_server is identical no matter which filesystem
API is being used. The only differences are in the server itself, which will create an instance of
the correct subclass of Volume and forward the requests to it.

The FUSE API
============

FUSE is a similar tool to UserlandFS. It was initially designed for Linux, but, because there are
many filesystems written for it, it was later ported to MacOS and FreeBSD. The FUSE project in
itself defines a network-like communication protocol between the kernel and the filesystem, however,
this is not what is implemented in UserlandFS (which already has its own way to do this part).
Instead, the provided API is compatible with libfuse (version 2.9.9, since most filesystems
available for FUSE have not migrated to libfuse 3.x yet).

This means only filesystems using libfuse can easily be ported. Those implementing the FUSE protocol
in other ways (for example because they are not written in C or C++ and need a different library)
will not be ported to Haiku as easily.

The libfuse API is actually two different APIs. One is called "fuse_operations". It is a quite
high level API, where all functions to access files receive a path to the file to operate on, and
the functions are synchronous. The other API (called "low level") is asynchronous and the files
are identified by their inode number. Of course, the low level API allows to get better performance,
because the kernel and userlandfs already work with inode numbers internally. With the high level
API, the inode number has to be converted back to a filepath everytime a function is called.

Each FUSE filesystem uses one of these two APIs. UserlandFS implements both of them and will
automatically detect which one to use.

Because of the Linux FUSE design, normally each filesystem is a standalone application, that
directly establishes communication with the kernel. When built for UserlandFS, the filesystems are
add-ons instead, so their main() function is called from userlandfs_server after loading the add-on.

We have found that this works reasonably well and it will be possible to run most filesystems
without any changes to their sourcecode.

When they initialize, FUSE filesystems provide userlandfs with a struct containing function pointers
for the various FUSE operations they implement. From this table, userlandfs_server detects which
features are supported, and reports this back to the kernel. Then the kernel-side of userlandfs
knows to not forward to userspace requests that wouldn't be handled by the filesystem anyway.

FUSE filesystems also usually need some command line options (both custom ones, and standard ones
forwarded to FUSE initialization options). In Haiku, these are passed as mount options which are
forwarded to the filesystem add-on when starting it. The add-on main function will be called only
when a filesystem of the corresponding type is being mounted.

Extensions to the FUSE API
--------------------------

The FUSE API is missing some things that are possible in Haiku native filesystems. Specifically,
there is no possibility to implement the "get_fs_info" call, and in particular use it to set
volume flags. This is especially annoying for networked filesystems, where it is important to mark
the filesystem as "shared".

Therefore, an extra FUSE "capability" has been added to mark support for this. Capabilities are the
way FUSE negociates features between the kernel and the filesystem. The kernel tells the filesystem
which capabilities are supported, and the filesystem tells the kernel which one it wants to use.

If the FUSE_CAP_HAIKU_FUSE_EXTENSIONS capability is enabled, userlandfs will query the filesystem
info using the ioctl hook of the filesystem, with a command value FUSE_HAIKU_GET_DRIVE_INFO. It
expects the filesystem to then fill the fs_info structure passed in the ioctl pointer.

Debugging userlandfs
====================

Because a lot of the code is in userspace, you can simply start userlandfs_server in a terminal to
get its output, or attach a Debugger to it. There is no timeout for userlandfs requests, so if you
are debugging something in userlandfs_server, all calls to the filesystem mounted through it will
simply block until the request thread is running again.
