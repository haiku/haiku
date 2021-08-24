The BeOS R5 Midi Kit protocol
=============================

In the course of writing the OpenBeOS Midi Kit, I spent some time
looking at how BeOS R5's libmidi2.so and midi_server communicate. Not
out of a compulsion to clone this protocol, but to learn from it. After
all, the Be engineers spent a lot of time thinking about this already,
and it would be foolish not to build on their experience. Here is what I
have found out.

Two kinds of communication happen: administrative tasks and MIDI events.
The housekeeping stuff is done by sending BMessages between the
BMidiRoster and the midi_server. MIDI events are sent between producers
and consumers using ports, without intervention from the server.

This document describes the BMessage protocol. The protocol appears to
be asynchronous, which means that when BMidiRoster sends a message to
the midi_server, it does not wait around for a reply, even though the
midi_server replies to all messages. The libmidi2 functions *do* block
until the reply is received, though, so client code does not have to
worry about any of this.

Both BMidiRoster and the midi_server can initiate messages. BMidiRoster
typically sends a message when client code calls one of the functions
from a libmidi2 class. When the midi_server sends messages, it is to
keep BMidiRoster up-to-date about changes in the roster. BMidiRoster
never replies to messages from the server. The encoding of the BMessage
'what' codes indicates their direction. The 'Mxxx' messages are sent
from libmidi2 to the midi_server. The 'mXXX' messages go the other way
around: from the server to a client.

--------------

Who does what?
--------------

The players here are the midi_server, which is a normal BApplication,
and all the client apps, also BApplications. The client apps have loaded
a copy of libmidi2 into their own address space. The main class from
libmidi2 is BMidiRoster. The BMidiRoster has a BLooper that communicates
with the midi_server's BLooper.

The midi_server keeps a list of *all* endpoints in the system, even
local, nonpublished, ones. Each BMidiRoster instance keeps its own list
of remote published endpoints, and all endpoints local to this
application. It does not know about remote endpoints that are not
published yet.

Whenever you make a change to one of your own endpoints, your
BMidiRoster notifies the midi_server. If your endpoint is published, the
midi_server then notifies all of the other BMidiRosters, so they can
update their local rosters. It does *not* notify your own app!
(Sometimes, however, the midi_server also notifies everyone else even if
your local endpoint is *not* published. The reason for this escapes me,
because the other BMidiRosters have no access to those endpoints
anyway.)

By the way, "notification" here means the internal communications
between server and libmidi, not the B_MIDI_EVENT messages you receive
when you call BMidiRoster::StartWatching().

--------------

BMidiRoster::MidiRoster()
-------------------------

The first time it is called, this function creates the one-and-only
instance of BMidiRoster. Even if you don't explicitly call it yourself,
it is used behind-the-scenes anyway by any of the other BMidiRoster
functions. MidiRoster() constructs a BLooper and gets it running. Then
it sends a BMessenger with the looper's address to the midi_server:

::

   OUT BMessage: what = Mapp (0x4d617070, or 1298231408)
       entry       be:msngr, type='MSNG', c=1, size=24,         

The server now responds with mOBJ messages for all *remote* *published*
producers and consumers. (Obviously, this list only contains remote
objects because by now you can't have created any local endpoints yet.)

For a consumer this message looks like:

::

   IN  BMessage: what = mOBJ (0x6d4f424a, or 1833910858)
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x1 (1, '')
       entry     be:latency, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry        be:port, type='LONG', c=1, size= 4, data[0]: 0x1dab (7595, '')
       entry        be:name, type='CSTR', c=1, size=16, data[0]: "/dev/midi/vbus0"

(Oddness: why is be:latency a LONG and not a LLNG? Since latency is
expressed in microseconds using a 64-bit bigtime_t, you'd expect the
midi_server to send all 64 of those bits... In the 'Mnew' message, on
the other hand, be:latency *is* a LLGN.)

And for a producer:

::

   IN  BMessage: what = mOBJ (0x6d4f424a, or 1833910858)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x2 (2, '')
       entry        be:name, type='CSTR', c=1, size=16, data[0]: "/dev/midi/vbus0"

Note that the be:name field is not present if the endpoint has no name.
That is, if the endpoint was constructed by passing a NULL name into the
BMidiLocalConsumer() or BMidiLocalProducer() constructor.

Next up are notifications for *all* connections, even those between
endpoints that are not registered:

::

   IN  BMessage: what = mCON (0x6d434f4e, or 1833127758)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x13 (19, '')
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x14 (20, '')

These messages are followed by an Msyn message:

::

   IN  BMessage: what = Msyn (0x4d73796e, or 1299413358)

And finally the (asynchronous) reply:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

Only after this reply is received, MidiRoster() returns.

The purpose of the Msyn message is not entirely clear. (Without it, Be's
libmidi2 blocks in the MidiRoster() call.) Does it signify the end of
the list of endpoints? Why doesn't libmidi2 simply wait for the final
reply?

--------------

BMidiLocalProducer constructor
------------------------------

BMidiRoster, on behalf of the constructor, sends the following to the
midi_server:

::

   OUT BMessage: what = Mnew (0x4d6e6577, or 1299080567)
       entry        be:type, type='CSTR', c=1, size=9, data[0]: "producer"
       entry        be:name, type='CSTR', c=1, size=21, data[0]: "MIDI Keyboard output"

The be:name field is optional.

The reply includes the ID for the new endpoint. This means that the
midi_server assigns the IDs, and any endpoint gets an ID whether it is
published or not.

::

   IN  BMessage: what =  (0x0, or 0)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x11 (17, '')
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

Unlike many other Be API classes, BMidiLocalProducer and
BMidiLocalConsumer don't have an InitCheck() method. But under certain
odd circumstances (such as the midi_server not running), creating the
endpoint might fail. How does client code check for that? Well, it turns
out that upon failure, the endpoint is assigned ID 0, so you can check
for that. In that case, the endpoint's refcount is 0 and you should not
Release() it. (That is stupid, actually, because Release() is the only
way that you can destroy the object. Our implementation should bump the
endpoint to 1 even on failure!)

If another app creates a new endpoint, your BMidiRoster is not notified.
The remote endpoint is not published yet, so your app is not supposed to
see it.

--------------

BMidiLocalConsumer constructor
------------------------------

This is similar to the BMidiLocalProducer constructor, although the
contents of the message differ slightly. Again, be:name is optional.

::

   OUT BMessage: what = Mnew (0x4d6e6577, or 1299080567)
       entry        be:type, type='CSTR', c=1, size=9, data[0]: "consumer"
       entry     be:latency, type='LLNG', c=1, size= 8, data[0]: 0x0 (0, '')
       entry        be:port, type='LONG', c=1, size= 4, data[0]: 0x4c0 (1216, '')
       entry        be:name, type='CSTR', c=1, size=13, data[0]: "InternalMIDI"  

And the reply:

::

   IN  BMessage: what =  (0x0, or 0)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x11 (17, '')
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

Before it sends the message to the server, the constructor creates a new
port with the name "MidiEventPort" and a queue length (capacity) of 1.

--------------

BMidiEndpoint::Register()
BMidiRoster::Register()
-------------------------

Sends the same message for producers and consumers:

::

   OUT BMessage: what = Mreg (0x4d726567, or 1299342695)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')     

The reply:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

If you try to Register() an endpoint that is already registered,
libmidi2 still sends the message. (Which could mean that BMidiRoster
does not keep track of this registered state.) The midi_server simply
ignores that request, and sends back error code 0 (B_OK). So the API
does not flag this as an error.

If you send an invalid be:id, the midi_server returns error code -1
(General OS Error, B_ERROR). If you try to Register() a remote endpoint,
libmidi2 immediately returns error code -1, and does not send a message
to the server.

If another app Register()'s a producer, your BMidiRoster receives:

::

   IN  BMessage: what = mOBJ (0x6d4f424a, or 1833910858)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x17 (23, '')
       entry        be:name, type='CSTR', c=1, size=7, data[0]: "a name"

If the other app registers a consumer, your BMidiRoster receives:

::

   IN  BMessage: what = mOBJ (0x6d4f424a, or 1833910858)
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x19 (25, '')
       entry     be:latency, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry        be:port, type='LONG', c=1, size= 4, data[0]: 0xde9 (3561, '')
       entry        be:name, type='CSTR', c=1, size=7, data[0]: "a name"

These are the same messages you get when your BMidiRoster instance is
constructed. In both messages, the be:name field is optional again.

If the other app Register()'s the endpoint more than once, you still get
only one notification. So the midi_server simply ignores that second
publish request.

--------------

BMidiEndpoint::Unregister()
BMidiRoster::Unregister()
---------------------------

Sends the same message for producers and consumers:

::

   OUT BMessage: what = Munr (0x4d756e72, or 1299541618)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')       

The reply:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

If you try to Unregister() and endpoint that is already unregistered,
libmidi2 still sends the message. The midi_server simply ignores that
request, and sends back error code 0 (B_OK). So the API does not flag
this as an error. If you try to Unregister() a remote endpoint, libmidi2
immediately returns error code -1, and does not send a message to the
server.

When another app Unregister()'s one of its own endpoints, your
BMidiRoster receives:

::

   IN  BMessage: what = mDEL (0x6d44454c, or 1833190732)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17 (23, '')             

When the other app deletes that endpoint (refcount is now 0) and it is
not unregistered yet, your BMidiRoster also receives that mDEL message.
Multiple Unregisters() are ignored again by the midi_server.

If an app quits without properly cleaning up, i.e. it does not
Unregister() and Release() its endpoints, then the midi_server's roster
contains a stale endpoint. As soon as the midi_server recognizes this
(for example, when an application tries to connect that endpoint), it
sends all BMidiRosters an mDEL message for this endpoint. (This message
is sent whenever the midi_server feels like it, so libmidi2 can receive
this message while it is still waiting for a reply to some other
message.) If the stale endpoint is still on the roster and you (re)start
your app, then you receive an mOBJ message for this endpoint during the
startup handshake. A little later you will receive the mDEL.

--------------

BMidiEndpoint::Release()
------------------------

Only sends a message if the refcount of local objects (published or not)
becomes 0:

::

   OUT BMessage: what = Mdel (0x4d64656c, or 1298425196)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')

The corresponding reply:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

If you did not Unregister() a published endpoint before you Release()'d
it, no 'Munr' message is sent. Of course, the midi_server is smart
enough to realize that this endpoint should be wiped from the roster
now. Likewise, if this endpoint is connected to another endpoint,
Release() will not send a separate 'Mdis' message, but the server *will*
disconnect them. (This, of course, only happens when you Release() local
objects. Releasing a proxy has no impact on the connection with the real
endpoint.)

When you Release() a proxy (a remote endpoint) and its refcount becomes
0, libmidi2 does not send an 'Mdel' message to the server. After all,
the object is not deleted, just your proxy. If the remote endpoint still
exists (i.e. IsValid() returns true), the BMidiRoster actually keeps a
cached copy of the proxy object around, just in case you need it again.
This means you can do this: endp = NextEndpoint(); endp->Release(); (now
refcount is 0) endp- >Acquire(); (now refcount is 1 again). But I advice
against that since it doesn't work for all objects; local and dead
remote endpoints *will* be deleted when their refcount reaches zero.

In Be's implementation, if you Release() a local endpoint that already
has a zero refcount, libmidi still sends out the 'Mdel' message. It also
drops you into the debugger. (I think it should return an error code
instead, it already has a status_t.) However, if you Release() proxies a
few times too many, your app does not jump into the debugger. (Again, I
think the return result should be an error code here -- for OpenBeOS R1
I think we should jump into the debugger just like with local objects).
Hmm, actually, whether you end up in the debugger depends on the
contents of memory after the object is deleted, because you perform the
extra Release() on a dead object. Don't do that.

--------------

BMidiEndpoint::SetName()
------------------------

For local endpoints, both unpublished and published, libmidi2 sends:

::

   OUT BMessage: what = Mnam (0x4d6e616d, or 1299079533)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')
       entry        be:name, type='CSTR', c=1, size=7, data[0]: "b name"

And receives:

::

   IN BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

You cannot rename remote endpoints. If you try, libmidi2 will simply
ignore your request. It does not send a message to the midi_server.

If another application renames one of its own endpoints, all other
BMidiRosters receive:

::

   IN  BMessage: what = mREN (0x6d52454e, or 1834108238)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x5 (5, '')
       entry        be:name, type='CSTR', c=1, size=7, data[0]: "b name"

You receive this message even if the other app did not publish its
endpoint. This seems rather strange, because your BMidiRoster has no
knowledge of this particular endpoint yet, so what is it to do with this
message? Ignore it, I guess.

--------------

BMidiEndpoint::GetProperties()
------------------------------

For *any* kind of endpoint (local non-published, local published,
remote) libmidi2 sends the following message to the server:

::

   OUT BMessage: what = Mgpr (0x4d677072, or 1298624626)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x2b2 (690, '')
       entry       be:props, type='MSGG', c=1, size= 0,

(Why this "get properties" request includes a BMessage is a mistery to
me. The midi_server does not appear to copy its contents into the reply,
which would have made at least some sense. The BMessage from the client
is completely overwritten with the endpoint's properties.)

::

   IN  BMessage: what =  (0x0, or 0)
       entry       be:props, type='MSGG', c=1, size= 0,
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

This means that endpoint properties are stored in the server only, not
inside the BMidiEndpoints, and not by the local BMidiRosters.

--------------

BMidiEndpoint::SetProperties()
------------------------------

For local endpoints, published or not, libmidi2 sends the following
message to the server:

::

   OUT BMessage: what = Mspr (0x4d737072, or 1299411058)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')
       entry       be:props, type='MSGG', c=1, size= 0,

And expects this back:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

You cannot change the properties of remote endpoints. If you try,
libmidi2 will ignore your request. It does not send a message to the
midi_server, and it returns the -1 error code (B_ERROR).

If another application changes the properties of one of its own
endpoints, all other BMidiRosters receive:

::

   IN  BMessage: what = mPRP (0x6d505250, or 1833980496)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x13 (19, '')
       entry  be:properties, type='MSGG', c=1, size= 0,

You receive this message even if the other app did not publish its
endpoint.

--------------

BMidiLocalConsumer::SetLatency()
--------------------------------

For local endpoints, published or not, libmidi2 sends the following
message to the server:

::

   OUT BMessage: what = Mlat (0x4d6c6174, or 1298948468)
       entry     be:latency, type='LLNG', c=1, size= 8, data[0]: 0x3e8 (1000, '')
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x14f (335, '')

And receives:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

If another application changes the latency of one of its own consumers,
all other BMidiRosters receive:

::

   IN  BMessage: what = mLAT (0x6d4c4154, or 1833714004)
       entry          be:id, type='LONG', c=1, size= 4, data[0]: 0x15 (21, '')
       entry     be:latency, type='LLNG', c=1, size= 8, data[0]: 0x3e8 (1000, '')

You receive this message even if the other app did not publish its
endpoint.

--------------

BMidiProducer::Connect()
------------------------

The message:

::

   OUT BMessage: what = Mcon (0x4d636f6e, or 1298362222)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x17f (383, '')
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x376 (886, '')

The answer:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

The server sends back a B_ERROR result if you specify wrong ID's. When
you try to connect a producer and consumer that are already connected to
each other, libmidi2 still sends the 'Mcon' message to the server (even
though it could have known these endpoints are already connected). In
that case, the server responds with a B_ERROR code as well.

When another app makes the connection, your BMidiRoster receives:

::

   IN  BMessage: what = mCON (0x6d434f4e, or 1833127758)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x13 (19, '')
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x14 (20, '')

Note: your BMidiRoster receives this notification even if the producer
or the consumer (or both) are not registered endpoints.

--------------

BMidiProducer::Disconnect()
---------------------------

The message:

::

   OUT BMessage: what = Mdis (0x4d646973, or 1298426227)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x309 (777, '')
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x393 (915, '')

The answer:

::

   IN  BMessage: what =  (0x0, or 0)
       entry      be:result, type='LONG', c=1, size= 4, data[0]: 0x0 (0, '')
       entry     _previous_, ...

The server sends back a B_ERROR result if you specify wrong ID's. When
you try to disconnect a producer and consumer that are not connected to
each other, libmidi2 still sends the 'Mdis' message to the server (even
though it could have known these endpoints are not connected). In that
case, the server responds with a B_ERROR code as well.

When another app breaks the connection, your BMidiRoster receives:

::

   IN  BMessage: what = mDIS (0x6d444953, or 1833191763)
       entry    be:producer, type='LONG', c=1, size= 4, data[0]: 0x13 (19, '')
       entry    be:consumer, type='LONG', c=1, size= 4, data[0]: 0x14 (20, '')

Note: your BMidiRoster receives this notification even if the producer
or the consumer (or both) are not registered endpoints.

--------------

Watchin'
--------

BMidiRoster::StartWatching() and StopWatching() do not send messages to
the midi_server. This means that the BMidiRoster itself, and not the
midi_server, sends the notifications to the messenger. It does this
whenever it receives a message from the midi_server.

The relationship between midi_server messages and B_MIDI_EVENT
notifications is as follows:

   +---------+---------------------------+
   | message | notification              |
   +=========+===========================+
   | mOBJ    | B_MIDI_REGISTERED         |
   +---------+---------------------------+
   | mDEL    | B_MIDI_UNREGISTERED       |
   +---------+---------------------------+
   | mCON    | B_MIDI_CONNECTED          |
   +---------+---------------------------+
   | mDIS    | B_MIDI_DISCONNECTED       |
   +---------+---------------------------+
   | mREN    | B_MIDI_CHANGED_NAME       |
   +---------+---------------------------+
   | mLAT    | B_MIDI_CHANGED_LATENCY    |
   +---------+---------------------------+
   | mPRP    | B_MIDI_CHANGED_PROPERTIES |
   +---------+---------------------------+

For each message on the left, the watcher will receive the corresponding
notification on the right.

--------------

Other observations
------------------

Operations that do not send messages to the midi_server:

-  BMidiEndpoint::Acquire(). This means reference counting is done
   locally by BMidiRoster. Release() doesn't send a message either,
   unless the refcount becomes 0 and the object is deleted. (Which
   suggests that it is actually the destructor and not Release() that
   sends the message.)

-  BMidiRoster::NextEndpoint(), NextProducer(), NextConsumer(),
   FindEndpoint(), FindProducer(), FindConsumer(). None of these
   functions send messages to the midi_server. This means that each
   BMidiRoster instance keeps its own list of available endpoints. This
   is why it receives 'mOBJ' messages during the startup handshake, and
   whenever a new remote endpoint is registered, and 'mDEL' messages for
   every endpoint that disappears. Even though the NextXXX() functions
   do not return locally created objects, this "local roster" *does*
   keep track of them, since FindXXX() *do* return local endpoints.

-  BMidiEndpoint::Name(), ID(), IsProducer(), IsConsumer(), IsRemote(),
   IsLocal() IsPersistent(). BMidiConsumer::Latency().
   BMidiLocalConsumer::GetProducerID(), SetTimeout(). These all appear
   to consult BMidiRoster's local roster.

-  BMidiEndpoint::IsValid(). This function simply looks at BMidiRoster's
   local roster to see whether the remote endpoint is still visible,
   i.e. not unregistered. It does not determine whether the endpoint's
   application is still alive, or "ping" the endpoint or anything fancy
   like that.

-  BMidiProducer::IsConnected(), Connections(). This means that
   BMidiRoster's local roster, or maybe the BMidiProducers themselves
   (including the proxies) keep track of the various connections.

-  BMidiLocalProducer::Connected(), Disconnected(). These methods are
   invoked when any app (including your own) makes or breaks a
   connection on one of your local producers. These hooks are invoked
   before the B_MIDI_EVENT messages are sent to any watchers.

-  Quitting your app. Even though the BMidiRoster instance is deleted
   when the app quits, it does not let the midi_server know that the
   application in question is now gone. Any endpoints you have
   registered are not automatically unregistered. This means that the
   midi_server is left with some stale information. Undoubtedly, there
   is a mechanism in place to clean this up. The same mechanism would be
   used to clean up apps that did not exit cleanly, or that crashed.

Other stuff:

-  libmidi2.so exports an int32 symbol called "midi_debug_level". If you
   set it to a non-zero value, libmidi2 will dump a lot of interesting
   debug info on stdout. To do this, declare the variable in your app
   with "extern int32 midi_debug_level;", and then set it to some high
   value later: "midi_debug_level = 0x7FFFFFFF;" Now run your app from a
   Terminal and watch libmidi2 do its thing.

-  libmidi2.so also exports an int32 symbol called
   "midi_dispatcher_priority". This is the runtime priority of the
   thread that fields MIDI events to consumers.
