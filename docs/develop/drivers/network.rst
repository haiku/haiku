Network drivers
===============

Network devices provide the usual functions: open, close, free, control (ioctl), read and write.
No additional methods are needed.

Open
----

This should prepare the device for sending and receiving packets.

Close
-----

This should cancel or terminate all pending network tranfers.

Read
----

The read function reads one network packet at a time. If the interface was opened in blocking mode
(either by the O_NONBLOCK flag passed to open, or by the ETHER_NONBLOCK ioctl), it should block
until a packet becomes available. Otherwise it should return 0 if there is no data.

Since the function can only return one packet at a time, the network driver is responsible for
buffering network packets if it receives multiple packets at a time, or receives them while the
Read function is not being called by the network stack (for example, by interrupts or DMA transfers).

The use of this function result in lower performance. If appropriate, the driver can make use of
the ETHER_RECEIVE_NET_BUFFER ioctl instead, to receive net_buffer objects and use them in
non-blocking, zero-copy operations.

Write
-----

The write function sends one packet at a time on the network. It should block until the packet is
sent, or it should copy the passed data into its own buffer management logic.

The use of this function result in lower performance. If appropriate, the driver can make use of
the ETHER_SEND_NET_BUFFER ioctl instead, to receive net_buffer objects and use them in
non-blocking, zero-copy operations.

Control
-------

The following ioctls are handled. They are all optional unless specified otherwise.

ETHER_INIT (required)
+++++++++++++++++++++

Intialize the network interface.

ETHER_GETADDR (required)
++++++++++++++++++++++++

Get the MAC address. The output buffer should be 6 bytes large.

ETHER_GETFRAMESIZE
++++++++++++++++++

Get the maximum frame size that the device can send (this is about the local device only, path MTU
discovery is done by other means).

ETHER_SET_LINK_STATE_SEM
++++++++++++++++++++++++

The parameter is a semaphore id. The driver should store it, and notify that semaphore whenever
there is a linkup or link down event.

ETHER_GET_LINK_STATE
++++++++++++++++++++

The parameter is an ether_link_state structure, that should be filled with the media type, quality,
link speed, and duplex state.

ETHER_SEND_NET_BUFFER
+++++++++++++++++++++

Called once at initialization with a NULL parameter. This must return B_BAD_DATA if the device
supports sending and receiving net_buffers directly.

If this is supported, the traditional read and write calls are not used. Instead, these ioctls
are used to work directly with net_buffers.

This allows the driver to do the sending in non-blocking mode more easily. The ioctl can return
before the buffer is actually consumed and sent. The driver is responsible for freeing the buffer
(using the free() hook from the net_buffer module) when it doesn't need it anymore.

net_buffers use scatter-gather; if the driver supports that, it allows to send a packet in
zero-copy mode. The traditional write hook requires the network stack to serialize the packet into
a linear buffer, which reduces performance in that case.

ETHER_RECEIVE_NET_BUFFER
++++++++++++++++++++++++

This is used as a replacement for write if net_buffer operations are supported by the driver.
Note that the driver must allow net_buffer usage for both sending and receiving, it's not possible
to implement one without the other.

The parameter is an output pointer to a net_bufer object. The driver should allocate
net_buffers (using the buffer manager kernel module) for incoming packets. The buffer_flags can
also indicate that the driver (or, more likely, the hardware) has already done some validity checks
(L3 and L4 checksums, for example). This avoids repeating these checks in software.

ETHER_SETPROMISC
++++++++++++++++

The parameter is a boolean, set to true to enable promiscuous mode. In this mode, all packets seen
on the network should be handed over to the network stack, even if they don't target the current
device MAC address. This is used for network capture tools such as tcpdump.

ETHER_NONBLOCK (optional, not used currently)
+++++++++++++++++++++++++++++++++++++++++++++

Allows to change the blocking mode of the read and write functions.

ETHER_ADDMULTI
++++++++++++++

Add a MAC address to listen to. Incoming packets targetted to that address should be forwarded
to the network stack (in addition to packets targetted to the device own MAC address).

This is used for multicast support, since multicast uses special MAC addresses that can target
several devices with a single network packet.

ETHER_REMMULTI
++++++++++++++

Stop receiving packets targetted to a MAC address previously enabled with ETHER_ADDMULTI.

SIOCGIFSTATS
++++++++++++

Get network interface statistics, such as the number of dropped packets and collisions.
