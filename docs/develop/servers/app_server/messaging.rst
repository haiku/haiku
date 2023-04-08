Messaging
=========

Inter-Application Messaging
---------------------------

The details of messaging are depicted under Process
Management::BApplication.

Drag-and-drop
-------------

Methods
-------

Messaging with the app_server is not done using BMessages because of the
overhead required to send them costs time and speed. Instead, ports are
utilized indirectly by means of the PortLink class, which simply makes
attaching data to a port message easier, but requires very little
overhead.


