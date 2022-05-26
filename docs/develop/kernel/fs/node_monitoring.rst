Node Monitoring
===============

Creation Date: January 16, 2003
Author(s): Axel Dörfler
                               

This document describes the feature of the BeOS kernel to monitor nodes.
First, there is an explanation of what kind of functionality we have to
reproduce (along with the higher level API), then we will present the
implementation in OpenBeOS.

Requirements - Exported Functionality in BeOS
---------------------------------------------

From user-level, BeOS exports the following API as found in the
storage/NodeMonitor.h header file:

::

       status_t watch_node(const node_ref *node, 
           uint32 flags, 
           BMessenger target);

       status_t watch_node(const node_ref *node, 
           uint32 flags, 
           const BHandler *handler,
           const BLooper *looper = NULL);

       status_t stop_watching(BMessenger target);

       status_t stop_watching(const BHandler *handler, 
           const BLooper *looper = NULL);
       

The kernel also exports two other functions to be used from file system
add-ons that causes the kernel to send out notification messages:

::

       int notify_listener(int op, nspace_id nsid,
           vnode_id vnida, vnode_id vnidb,
           vnode_id vnidc, const char *name);
       int send_notification(port_id port, long token,
           ulong what, long op, nspace_id nsida,
           nspace_id nsidb, vnode_id vnida,
           vnode_id vnidb, vnode_id vnidc,
           const char *name);
       

The latter is only used for live query updates, but is obviously called
by the former. The port/token pair identify a unique BLooper/BHandler
pair, and it used internally to address those high-level objects from
the kernel.

When a file system calls the ``notify_listener()`` function, it will
have a look if there are monitors for that node which meet the specified
constraints - and it will call ``send_notification()`` for every single
message to be send.

Each of the parameters ``vnida - vnidc`` has a dedicated meaning:

-  **vnida:** the parent directory of the "main" node
-  **vnidb:** the target parent directory for a move
-  **vnidc:** the node that has triggered the notification to be send

The flags parameter in ``watch_node()`` understands the following
constants:

-  **B_STOP_WATCHING**
   watch_node() will stop to watch the specified node.
-  **B_WATCH_NAME**
   name changes are notified through a B_ENTRY_MOVED opcode.
-  **B_WATCH_STAT**
   changes to the node's stat structure are notified with a
   B_STAT_CHANGED code.
-  **B_WATCH_ATTR**
   attribute changes will cause a B_ATTR_CHANGED to be send.
-  **B_WATCH_DIRECTORY**
   notifies on changes made to the specified directory, i.e.
   B_ENTRY_REMOVED, B_ENTRY_CREATED
-  **B_WATCH_ALL**
   is a short-hand for the flags above.
-  **B_WATCH_MOUNT**
   causes B_DEVICE_MOUNTED and B_DEVICE_UNMOUNTED to be send.

Node monitors are maintained per team - every team can have up to 4096
monitors, although there exists a private kernel call to raise this
limit (for example, Tracker is using it intensively).

The kernel is able to send the BMessages directly to the specified
BLooper and BHandler; it achieves this using the application kit's token
mechanism. The message is constructed manually in the kernel, it doesn't
use any application kit services.

| 

Meeting the Requirements in an Optimal Way - Implementation in OpenBeOS
-----------------------------------------------------------------------

If you assume that every file operation could trigger a notification
message to be send, it's clear that the node monitoring system must be
optimized for sending messages. For every call to ``notify_listener()``,
the kernel must check if there are any monitors for the node that was
updated.

Those monitors are put into a hash table which has the device number and
the vnode ID as keys. Each of the monitors maintains a list of listeners
which specify which port/token pair should be notified for what change.
Since the vnodes are created/deleted as needed from the kernel, the node
monitor is maintained independently from them; a simple pointer from a
vnode to its monitor is not possible.

The main structures that are involved in providing the node monitoring
functionality look like this:

::

       struct monitor_listener {
           monitor_listener    *next;
           monitor_listener    *prev;
           list_link       monitor_link;
           port_id         port;
           int32           token;
           uint32          flags;
           node_monitor        *monitor;
       };

       struct node_monitor {
           node_monitor        *next;
           mount_id        device;
           vnode_id        node;
           struct list     listeners;
       };
       

The relevant part of the I/O context structure is this:

::

       struct io_context {
           ...
           struct list     node_monitors;
           uint32          num_monitors;
           uint32          max_monitors;
       };
       

If you call ``watch_node()`` on a file with a flags parameter unequal to
B_STOP_WATCHING, the following will happen in the node monitor:

#. The ``add_node_monitor()`` function does a hash lookup for the
   device/vnode pair. If there is no ``node_monitor`` yet for this pair,
   a new one will be created.
#. The list of listeners is scanned for the provided port/token pair
   (the BLooper/BHandler pointer will already be translated in
   user-space), and the new flag is or'd to the old field, or a new
   ``monitor_listener`` is created if necessary - in the latter case,
   the team's node monitor counter is incremented.

If it's called with B_STOP_WATCHING defined, the reverse operation take
effect, and the ``monitor`` field is used to see if this monitor don't
have any listeners anymore, in which case it will be removed.

Note the presence of the ``max_monitors`` - there is no hard limit the
kernel exposes to userland applications; the listeners are maintained in
a doubly-linked list.

If a team is shut down, all listeners from its I/O context will be
removed - since every listener stores a pointer to its monitor,
determining the monitors that can be removed because of this operation
is very cheap.

The ``notify_listener()`` also only does a hash lookup for the
device/node pair it got from the file system, and sends out as many
notifications as specified by the listeners of the monitor that belong
to that node.

If a node is deleted from the disk, the corresponding ``node_monitor``
and its listeners will be removed as well, to prevent watching a new
file that accidently happen to have the same device/node pair (as is
possible with BFS, for example).

| 

Differences Between Both Implementations
----------------------------------------

Although the aim was to create a completely compatible monitoring
implementation, there are some notable differences between the two.

BeOS reserves a certain number of slots for calls to ``watch_node()`` -
each call to that function will use one slot, even if you call it twice
for the same node. OpenBeOS, however, will always use one slot per node
- you could call ``watch_node()`` several times, but you would waste
only one slot.

While this is an implementational detail, it also causes a change in
behaviour for applications; in BeOS, applications will get one message
for every ``watch_node()`` call, in OpenBeOS, you'll get only one
message per node. If an application relies on this strange behaviour of
the BeOS kernel, it will no longer work correctly.

The other difference is that OpenBeOS exports its node monitoring
functionality to kernel modules as well, and provides an extra plain C
API for them to use.

| 

And Beyond?
-----------

The current implementation directly iterates over all listeners and
sends out notifications as required synchronously in the context of the
thread that triggered the notification to be sent.

If a node monitor needs to send out several messages, this could
theoretically greatly decrease file system performance. To optimize for
this case, the required data of the notification could be put into a
queue and be sent by a dedicated worker thread. Since this requires an
additional copy operation and a reserved address space for this queue,
this optimization could be more expensive than the current
implementation, depending on the usage pattern of the node monitoring
mechanism.

With BFS, it would be possible to introduce the possibility to
automatically watch all files in a specified directory. While this would
be very convenient at application level, it comes with several
disadvantages:

#. This feature might not be easily accomplishable for many file
   systems; a file system must be able to retrieve a node by ID only -
   it might not be feasible to find out about the parent directory for
   many file systems.
#. Although it could potentially safe node monitors, it might cause the
   kernel to send out a lot more messages to the application than it
   needs. With the restriction the kernel imposes to the number of
   watched nodes for a team, the application's designer might try to be
   much stricter with the number of monitors his application will
   consume.

While 1.) might be a real show stopper, 2.) is almost invalidated
because of Tracker's usage of node monitors; it consumes a monitor for
every entry it displays, which might be several thousands. Implementing
this feature would not only greatly speed up maintaining this massive
need of monitors, and cut down memory usage, but also ease the
implementation at application level.

Even 1.) could be solved if the kernel could query a file system if it
can support this particular feature; it could then automatically monitor
all files in that directory without adding complexity to the application
using this feature. Of course, the effort to provide this functionality
is much larger then - but for applications like Tracker, the complexity
would be removed from the application without extra cost.

However, none of the discussed feature extensions have been implemented
for the currently developed version R1 of OpenBeOS.
