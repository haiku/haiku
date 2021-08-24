Haiku Network Stack Architecture
================================

The Haiku Network Stack is a modular and layered networking stack, very
similar to what you may know as BONE.

The entry point when talking to the stack is through a dedicated device
driver that publish itself in /dev/net. The userland library
libnetwork.so (which combines libsocket.so, and libbind.so) directly
talks to this driver, mostly via ioctl()\ `1 <#foot1>`__.

The driver either creates sockets, or passes on every command to the
socket module\ `2 <#foot2>`__. Depending on the address family and type
of the sockets, the lower layers will be loaded and connected.

For example, with a TCP/IP socket, the stack could look like this:

+------------------+--------------------------------------------------------+
| **Socket**                                                                |
+------------------+--------------------------------------------------------+
| TCP              | Protocols defined by the socket (address family, type) |
+------------------+                                                        |
| IPv4             | (session, transport, network layers)                   |
+------------------+--------------------------------------------------------+
| **Datalink**                                                              |
+------------------+--------------------------------------------------------+
| ARP              | Datalink Protocols defined by the interface            |
|                  | (IP address, device)                                   |
+------------------+                                                        |
| Ethernet framing | (datalink layer)                                       |
+------------------+--------------------------------------------------------+
| Ethernet device  | (physical layer)                                       |
+------------------+--------------------------------------------------------+

Where TCP, and IPv4 are net_protocol modules, and ARP, and the Ethernet
framing are net_datalink_protocol modules. All modules are connected in
a chain, even though the datalink layer introduces more than one path
(one for each interface).

When sending data through a socket, a net_buffer is created in the
socket module, and passed on to the lower levels where each protocol
processes it, before passing it on to the next protocol in the chain.
The last protocol in the chain is always a domain protocol - it will
directly forward the buffers to the datalink module. When the buffer
reaches the datalink level, an accompanied net_route object will
determine for which interface (which determines the datalink protocols
in the chain) the buffer is destined. The route has to be specified by
the upper protocols before the buffer gets into the datalink level - if
a buffer comes in without a valid route, it is discarded.

The protocol modules are loaded and unloaded as needed. The stack itself
stays loaded as long as there are interfaces defined - as soon as the
last interface is removed, the stack gets unloaded (which is, of course,
not yet implemented).

The Structures and Classes
~~~~~~~~~~~~~~~~~~~~~~~~~~

net_domain
^^^^^^^^^^

Every supported address family gets its own domain. A domain comprises
such a family, a net_protocol module that handles this domain, and a
list of interfaces and routes. It also gets a name: for example, the
IPv4 module registers the "internet" domain (AF_INET).

The domain protocol module is responsible for managing the domain; it
has to register it when it's loaded, and it has to unregister it when it
is unloaded by the networking stack.

net_interface
^^^^^^^^^^^^^

An interface makes an underlying net_device accessible by the stack.
When creating a new interface, you have to specify a domain, and a
device to be used. The stack will then look through the registered
datalink protocols, and builds a chain of them for that interface.

The interface usually gets a network address, and a route that directs
buffers to be sent to it. If there is no route to an interface, it will
never be used for outgoing data, but may well receive data from other
hosts.

An interface can be "up" (when ``IFF_UP`` is set in its ``flags``
member) in which case it accepts data - when that flag is not set, it
will discard all data it gets. The interface also specifies the maximum
buffer size that can be sent over this interface (the ``mtu`` member,
a.k.a. maximum transmission unit).

Interfaces are configured via ioctl()s (SIOCAIFADDR, ...). You can use
the command line tool "ifconfig" to do this for you.

net_device
^^^^^^^^^^

A networking device is used to actually send and receive the buffers. It
either points to an actual hardware device (in case of ethernet), or to
a virtual device (in case of loopback). Every device has a unique name
that identifies it. When creating a device, the name also decides which
net_device module will be chosen; for example, everything that starts
with "loop" will end up in the loopback device, while the ethernet
device accepts names that start with "/dev/net/".

A device can be shared by many interfaces at the same time. The device
to be used by an interface is specified at the time an interface is
created. It also has an ``mtu`` member that determines the upper limit
of an interface's ``mtu`` as well.

net_buffer
^^^^^^^^^^

A buffer holds exactly one packet, and has a source as well as a
destination address. The addresses may be changed in every layer the
buffer passes through. For example, the datalink protocols usually use
sockaddr_dl structures with family AF_DLI, while the upper levels may
use sockaddr_in structures with family AF_INET. Every protocol only
supports a small number of address types, and it's the requirement of
the upper protocols to prepare the address for use in the lower
protocols (and that's also a reason why it wouldn't work to arbitrarily
stack protocols onto each other).

The net_buffer module can be used to access the data within the buffer,
append new data to the buffer, or remove chunks of data from it.
Internally, the buffer consists of usually fixed size (2048 byte)
buffers that can be shared or connected as needed.

net_socket
^^^^^^^^^^

The socket is only of interest for the net_protocol modules, as it
stores options that may have an effect on the protocol's performance.
It's the direct counterpart to a socket file descriptor in userland, but
it has only little logic bound to it.

When a socket is created, the networking stack creates a chain of
net_protocol modules for the socket that will then do the real work.
When the socket is closed, the net_protocol chain is freed, and the
modules are eventually unloaded (if they are no longer in use).

net_protocol
^^^^^^^^^^^^

The protocols are bound to a specific socket, process the outgoing
buffers as needed (ie. add or remove headers, compute checksums, ...),
and pass it on to the next protocol. The last protocol in the chain is
always a domain protocol that will forward the calls to the datalink
module directly, if needed.

A domain protocol is a net_protocol that registered a domain, ie. IPv4.
Other than usual protocols, domain protocols have some special
requirements:

-  they need to be able to execute send_data(), and get_domain() without
   a pointer to its net_protocol object, as those may be called outside
   of the socket context.
-  as mentioned, they also don't talk to the next protocol in the chain
   (as they are always the last one), but to the datalink module
   directly.

Similar to the need to perform send_data() outside of the socket
context, all protocols that can receive data need to handle incoming
data without the socket context: incoming data is always handled outside
of the socket context, as the actual target socket is unknown during
processing.

Only the top-most protocol will be able to forward the packet to the
target socket(s). To receive incoming data, a protocol must register
itself as receiving protocol with the networking stack. The domain
protocol is usually registered automatically by a net_datalink_protocol
module that knows about both ends (for example, the ARP module is both
IPv4 and ethernet specific, and therefore registers the AF_INET domain
to receive ethernet packets of type IP).

net_datalink_protocol
^^^^^^^^^^^^^^^^^^^^^

The datalink protocols are bound to a specific net_interface, and
therefore to a specific net_device as well. Outgoing data is processed
so that it can be sent via the net_device. For example, the ARP protocol
will replace sockaddr_in structures in the buffer with sockaddr_dl
structures describing the ethernet MAC address of the source and
destination hosts, the ethernet_frame protocol will add the usual
ethernet header, etc.

The last protocol in the chain is also a special device interface bridge
protocol, that redirects the calls to the underlying net_device.

Incoming data is handled differently again; when you want to receive
data directly coming from a device, you can either register a deframing
function for it, or a handler that will be called depending on what data
type the deframing module reported. For example, the ethernet_frame
module registers an ethernet deframing function, while the ARP module
registers a handler for ethernet ARP packets with the device. When the
deframing function reports a ``ETHER_TYPE_ARP`` packet, the ARP
receiving function will be called.

net_route
^^^^^^^^^

A route determines the target interface of an outgoing packet. A route
is always owned by a specific domain, and the route is chosen by
comparing the networking address of the outgoing buffer with the mask
and address of the route.

A protocol will usually not use the routes directly, but use a
net_route_info object (see below), that will make sure that the route is
updated automatically whenever the routing table is changed.

net_route_info
^^^^^^^^^^^^^^

A routing helper for protocol usage: it stores the target address as
well as the route to be used, and has to be registered with the
networking stack via ``register_route_info()``.

Then, the stack will automatically update the route as needed, whenever
the routing table of the domain changes; it will always matches the
address specified there. When the routing is no longer needed, you must
unregister the net_route_info again.

--------------

| 1 You can find the definition of the driver interface in
  `headers/private/net/net_stack_interface.h <https://git.haiku-os.org/haiku/tree/headers/private/net>`__,
  as well as the driver itself at
  `src/add-ons/kernel/drivers/network <https://git.haiku-os.org/haiku/tree/src/add-ons/kernel/drivers/network>`__
| 2\ `src/add-ons/kernel/network/stack/ <https://git.haiku-os.org/haiku/tree/src/add-ons/kernel/network/stack>`__
