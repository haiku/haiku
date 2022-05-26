Testing the Midi Kit
====================

Most of the OpenBeOS source code has unit tests in the current/src/tests
directory. I looked into building CppUnit tests for the midi2 kit, but
decided that it doesn't really make much sense. Unit tests work best if
you can test something in isolation, but in the case of the midi2 kit
this is very hard to achieve. Because the classes from libmidi2.so
always need to talk to the midi_server, the tests depend on too many
external factors. The available endpoints, for example, will differ from
system to system. The spray and hook functions are difficult to test
this way, too.

So instead of a CppUnit test suite, here is a list of manual tests that
I performed when developing the midi2 kit:

--------------

Registering the application
---------------------------

*Required:* Client app that calls BMidiRoster::MidiRoster()

-  When a client app starts, it should first receive mNEW notifications
   for all endpoints in the system (even unregistered remotes), followed
   by mCON notifications for all connections in the system (even those
   between two unregistered local endpoints from another app).

-  Send invalid Mapp message (without messenger). The midi_server
   ignores the request, and the client app blocks forever.

-  Fake a delivery error for the mNEW notifications and the mAPP reply.
   (Add a snooze() in the midi_server's OnRegisterApplication(). While
   it is snoozing, Ctrl-C the client app. Now the server can't deliver
   the message and will unregister the application again.)

-  Kill the server. Start the client app. It should realize that the
   server is not running, and return from MidiRoster(); it does not
   block forever.

-  Note: The server does not protect against sending two or more Mapp
   messages; it will add a new app_t object to the roster and it will
   also send out the mNEW and mCON notifications again.

-  Verify that when the client app quits, the BMidiRoster instance is
   destroyed by the BMidiRosterKiller. The BMidiRosterLooper is also
   destroyed, along with any endpoint objects from its list. We don't
   destroy endpoints with a refcount > 0, but print a warning message on
   stderr instead.

-  When the app quits before it has created a BMidiRoster instance, the
   BMidiRosterKiller should do nothing.

--------------

Creating endpoints
------------------

*Required:* Client app that creates a new BMidiLocalProducer and/or
BMidiLocalConsumer

-  Send invalid Mnew message (missing fields). The server will return an
   error code.

-  Don't send reply from midi_server. The client receives a B_NO_REPLY
   error.

-  If something goes wrong creating a new local endpoint, you still get
   a new BMidiEndpoint object (but it is not added to
   BMidiRosterLooper's internal list of endpoints). Verify that its ID()
   function returns 0, and IsValid() returns false. Verify that you can
   Release() it without crashing into the debugger (i.e. the reference
   count of the new object should be 1).

-  Snooze in midi_server's OnCreateEndpoint() before sending reply to
   client to simulate heavy processor load. Client should timeout. When
   done snoozing, server fails to deliver the reply because the client
   is no longer listening, and it unregisters the app.

-  Note: if you kill the client app with Ctrl-C before the server has
   sent its reply, SendReply() still returns okay, and the midi_server
   adds the endpoint, even though the corresponding app is dead. There
   is not much we can do to prevent that (but it is not really a big
   deal).

-  Start the test app from two different Terminals. Verify that the new
   local endpoint of app1 is added to the BMidiRosterLooper's list of
   endpoints, and that its "isLocal" flag is true. Verify that when you
   start the second app, it immediately receives mNEW notifications for
   the first app's endpoints. It should also create BMidiEndpoint proxy
   objects for these endpoints with "isLocal" set to false, and add them
   its own list. Vice versa for the endpoints that app2 creates. Verify
   that the "registered" field in the mNEW notification is false,
   because newly created endpoints are not registered yet. The
   "properties" field should contain an empty message.

-  Start server. Start client app. The app makes new endpoints and the
   server adds them to the roster. Ctrl-C the app. Start client app
   again. The new client first receives mNEW notifications for the old
   app's endpoints. When the new app tries to create its own endpoints,
   the server realizes that the old app is dead, and sends mDEL
   notifications for the now-defunct endpoints.

-  The test app should now create 2 endpoints. Let the midi_server
   snooze during the second create message, so the app times out. The
   server now unregisters the app and purges its first endpoint (which
   was successfully created).

-  The test app should now create 3 endpoints. Let the midi_server
   snooze during the second create message, so the app times out. (It
   also times out when sending the create request for the 3rd endpoint,
   because the server is still snoozing.) Because it cannot send a reply
   for the 2nd create message, the server now unregisters the app and
   purges its first endpoint (which was successfully created). Then it
   processes the create request for the 3rd endpoint, but ignores it
   because the app is now no longer registered with the server.

-  Purging endpoints. The test app should now create 2 endpoints. Let
   the midi_server snooze during the \_fourth\_ create message. Run the
   server. Run the test app. Run the test app again in a second
   Terminal. The server times out, and unregisters the second app. The
   first app should receive an mDEL notification. Repeat, but now the
   test app should make 3 endpoints and the server fails on the
   \_sixth\_ endpoint. The first app now receives 2 mDEL notifications.

-  You should be allowed to pass NULL into the BMidiLocalProducer and
   BMidiLocalConsumer constructor.

-  Let the midi_server assign random IDs to new endpoints; the
   BMidiRosterLooper should sort the endpoints by their IDs when it adds
   them to its internal list.

--------------

Deleting endpoints
------------------

*Required:* client app that creates one or more endpoints and
Release()'s them

-  Verify that Acquire() increments the endpoint's refcount and
   Release() decrements it. When you Release() a local endpoint so its
   refcount becomes zero, the client sends an Mdel request to the
   server. When you Release() a local endpoint too many times, your app
   jumps into the debugger.

-  Send an Mdel request with an invalid ID to the server. Examples of
   invalid IDs: -1, 0, 1000 (or any other large number).

-  Start the test app from two different Terminals. Note that when one
   of the apps Release()'s its endpoints, the other receives
   corresponding mDEL notifications.

-  Snooze in midi_server's OnCreateEndpoint() before sending reply to
   "create endpoint" request. The client will timeout and the server
   will unregister the app. Now have the client Release() the endpoint.
   This sends a "delete endpoint" request to the server, which ignores
   the request because the app is no longer registered.

-  Override BMidiLocalProducer and BMidiLocalConsumer, and provide a
   public destructor. Call "delete prod; delete cons;" from your code,
   instead of using Release(). Your app should drop into the debugger.

-  Start the client app and let it make its endpoints. Kill the server.
   Release() the endpoints. The server doesn't run, so the Mdel request
   never arrives, but the BMidiEndpoint objects should be deleted
   regardless.

-  Start the test app from two different Terminals, and let them make
   their endpoints. Quit the apps (using the Deskbar's "Quit
   Application" menu item). Verify that both clean up and exit
   correctly. App1 removes its own endpoint from the BMidiRosterLooper's
   list of endpoints and sends an 'mDEL' message to the server, which
   passes it on to app2. In response, app2 removes the proxy object from
   its own list and deletes it. Again, vice versa for the endpoint from
   app2.

-  Start both apps again and wait until they have notified each other
   about the endpoints. Ctrl-C app1, and restart it. Verify that app1
   receives the 'mNEW' messages and creates proxies for these remote
   endpoints. Both apps should receive an 'mDEL' message for app1's old
   endpoint (because the midi_server realizes it no longer exists and
   purges it), and remove it from their lists accordingly.

--------------

Changing attributes
-------------------

*Required:* Client app that creates an endpoint and calls Register(),
Unregister(), SetName(), and SetLatency()

-  Send an Mchg request with an invalid ID to the server.

-  Register() a local endpoint that is already registered. This does not
   send a message to the server and always returns B_OK. Likewise for
   Unregister()ing a local endpoint that is not registered.

-  Register() or Unregister() a remote endpoint, or an invalid local
   endpoint. That should immediately return an error code.

-  Verify that BMidiRoster::Register() does the same thing as
   BMidiEndpoint::Register(). Also for BMidiRoster::Unregister() and
   BMidiEndpoint::Unregister().

-  If you pass NULL into BMidiRoster::Register() or Unregister(), the
   functions immediately return with an error code.

-  SetName() should ignore NULL names. When you call it on a remote
   endpoint, SetName() should do nothing. SetName() does not send a
   message if the new name is the same as the current name.

-  SetLatency() should ignore negative values. SetLatency() does not
   send a message if the new latency is the same as the current latency.
   (Since SetLatency() lives in BMidiLocalConsumer, you can never use it
   on remote endpoints.)

-  Kill the server after making the new endpoint, and call Register().
   The client app should return an error code. Also for Unregister(),
   SetName(), SetLatency(), and SetProperties().

-  Snooze in the midi_server's OnChangeEndpoint() before sending the
   reply to the client. Both sides will flag an error. No mCHG
   notifications will be sent. The server unregisters the app and purges
   its endpoints.

-  Verify that other apps will receive mCHG notifications when the test
   app successfully calls Register(), Unregister(), SetName(), and
   SetLatency(), and that they modify the corresponding BMidiEndpoint
   objects accordingly. Since clients are never notified when they
   change their own endpoints, they should ignore the notifications that
   concern local endpoints. Latency changes should be ignored if the
   endpoint is not a consumer.

-  Send an Mchg request with only the "midi:id" field, so no
   "midi:name", "midi:registered", "midi:latency", or "midi:properties".
   The server will still notify the other apps, although they will
   obviously ignore the notification, because it doesn't contain any
   useful data.

-  The Mchg request is overloaded to change several attributes. Verify
   that changing one of these attributes, such as the latency, does not
   overwrite/wipe out the others.

-  Start app1. Wait until it has created and registered its endpoint.
   Start app2. During the initial handshake, app2 should receive an
   'mNEW' message for app1's endpoint. Verify that the "refistered"
   field in this message is already true, and that this is passed on
   correctly to the new BMidiEndpoint proxy object.

-  GetProperties() should return NULL if the message parameter is NULL.

-  The properties of new endpoints are empty. Create a new endpoint and
   call GetProperties(). The BMessage that you receive should contain no
   fields.

-  SetProperties() should return NULL if the message parameter is NULL.
   It should return an error code if the endpoint is remote or invalid.
   It should work fine on local endpoints, registered or not.
   SetProperties() does not compare the contents of the new BMessage to
   the old, so it will always send out the change request.

-  If you Unregister() an endpoint that is connected, the connection
   should not be broken.

--------------

Consulting the roster
---------------------

*Required:* Client app that creates several endpoints, and registers
some of them (not all), and uses the BMidiRoster::FindEndpoint() etc
functions to examine the roster.

-  Verify that FindEndpoint() returns NULL if you pass it:

   -  invalid ID (localOnly = false)
   -  invalid ID (localOnly = true)
   -  remote non-registered endpoint (localOnly = false)
   -  remote non-registered endpoint (localOnly = true)
   -  remote registered endpoint (localOnly = true)

   | 

   Verify that FindEndpoint() returns a valid BMidiEndpoint object if
   you pass it:

   -  local non-registered endpoint (localOnly = false)
   -  local non-registered endpoint (localOnly = true)
   -  local registered endpoint (localOnly = false)
   -  local registered endpoint (localOnly = true)
   -  remote registered endpoint (localOnly = false)

   | 

-  Verify that FindConsumer() works just like FindEndpoint(), but that
   it also returns NULL if the endpoint with the specified ID is not a
   consumer. Likewise for FindProducer().

-  Verify that NextEndpoint() returns NULL if you pass it NULL. It also
   returns NULL if no more endpoints exist. Otherwise, it returns a
   BMidiEndpoint object, bumps the endpoint's reference count, and sets
   the "id" parameter to the ID of the endpoint. NextEndpoint() should
   never return local endpoints (registered or not), nor unregistered
   remote endpoints. Verify that negative "id" values also work.

-  Verify that you can safely call the Find and Next functions without
   having somehow initialized the BMidiRoster first (by making a new
   endpoint, for example). The functions themselves should call
   MidiRoster() and do the handshake with the server.

-  The Find and Next functions should bump the reference count of the
   BMidiEndpoint object that they return. However, they should not
   (inadvertently) modify the refcounts of any other endpoint objects.

-  Get a BMidiEndpoint proxy for a remote published endpoint. Release().
   Now it should not be removed from the endpoint list or even be
   deleted, even though its reference count dropped to zero.

-  Start app1. Start app2. App2 gets a BMidiEndpoint proxy for a remote
   endpoint from app1. Ctrl-C app1. Start app1 again. Now app2 receives
   an mDEL message for app1's old endpoint. Verify that the endpoint is
   removed from the endpoint list, but not deleted because its reference
   count isn't zero. If app2 now Release()s the endpoint, the
   BMidiEndpoint object should be deleted. Try again, but now Release()
   the endpoint before you Ctrl-C; now it should be deleted and removed
   from the list when you start app1 again.

--------------

Making/breaking connections
---------------------------

*Required:* Client app that creates a producer and consumer endpoint,
optionally registers them, consults the roster for remote endpoints, and
makes various kinds of connections.

-  Test the following for BMidiProducer::Connect():

   -  Connect(NULL)
   -  Connect(invalid consumer)
   -  Connect() using an invalid producer
   -  Send Mcon request with invalid IDs
   -  Kill the midi_server just before you Connect()
   -  Let the midi_server snooze, so the connect request times out
   -  Have the midi_server return an error result code
   -  On successful connect, verify that the consumer is added to the
      producer's list of endpoints
   -  Verify that you can make connections between 2 local endpoints, a
      local producer and a remote consumer, a remote producer and a
      local consumer, and two 2 remote endpoints. Test the local
      endpoints both registered and unregistered.
   -  2x Connect() on same consumer should give an error
   -  The other applications should receive an mCON notification, and
      adjust their own local rosters accordingly
   -  If you are calling Connect() on a local producer, its Connected()
      hook should be called. If you are calling Connect() on a remote
      producer, then its own application should call the Connected()
      hook.

   | 

-  Test the following for BMidiProducer::Disconnect():

   -  Disconnect(NULL)
   -  Disconnect(invalid consumer)
   -  Disconnect() using an invalid producer
   -  Send Mdis request with invalid IDs
   -  Kill the midi_server just before you Disconnect()
   -  Let the midi_server snooze, so the disconnect request times out
   -  Have the midi_server return an error result code
   -  On successful disconnect, verify that the consumer is removed from
      the producer's list of endpoints
   -  Verify that you can break connections between 2 local endpoints, a
      local producer and a remote consumer, a remote producer and a
      local consumer, and two 2 remote endpoints. Test the local
      endpoints both registered and unregistered.
   -  Disconnecting 2 endpoints that were not connected should give an
      error
   -  The other applications should receive an mDIS notification, and
      adjust their own local rosters accordingly
   -  If you are calling Disconnect() on a local producer, its
      Disconnected() hook should be called. If you are calling
      Disconnect() on a remote producer, then its own application should
      call the Disconnected() hook.

   | 

-  Make a connection on a local producer. Release() the producer. The
   other app should only receive an mDEL notification. Likewise if you
   have a connection with a local consumer and you Release() that.
   However, now all apps should throw away this consumer from the
   connection lists, invoking the Disconnected() hook of local
   producers. The same thing happens if you Ctrl-C the app and restart
   it. (Now the old endpoints are purged.)

-  BMidiProducer::IsConnected() should return false if you pass NULL or
   an invalid consumer.

-  BMidiProducer::Connections() should return a new BList every time you
   call it. The objects in this list are the BMidiConsumers that are
   connected to this producer; verify that their reference counts are
   bumped for every call to Connections().

--------------

Watching
--------

*Required:* Client app that creates local consumer and producer
endpoints, and calls Register(), Unregister(), SetName(), SetLatency(),
and SetProperties(). It should also make and break connections.

-  When you call StartWatching(), you should receive B_MIDI_EVENT
   notifications for all remote registered endpoints and the connections
   between them. You will get no notifications for local endpoints, or
   for any connections that involve unregistered endpoints. The
   BMidiRosterLooper should make a copy of the BMessenger, so when the
   client destroys the original messenger, you will still receive
   notifications. Verify that calling StartWatching() with the same
   BMessenger twice in a row will also send the initial set of
   notifications twice. StartWatching(NULL) should be ignored and does
   not remove the current messenger.

-  Run the client app from two different Terminals. Verify that you
   receive properly formatted B_MIDI_EVENT notifications when the other
   app changes the attributes of its *registered* endpoints with the
   various Set() functions. You should also receive notifications if the
   app Register()s or Unregister()s its endpoints. That app that makes
   these changes does not receive the notifications.

-  Run the client app from two different Terminals. Verify that you
   receive properly formatted B_MIDI_EVENT notifications when the apps
   make and break connections. Every app receives these connection
   notifications, whether the endpoints are published or not. The app
   that makes and breaks the connections does not receive any
   notifications.

-  StopWatching() should delete BMidiRosterLooper's BMessenger copy, if
   any. Verify that you no longer receive B_MIDI_EVENT notifications for
   remote endpoints after you have called StopWatching().

-  If the client is watching, and the BMidiRosterLooper receives an mDEL
   notification for a registered remote endpoint, it should also send an
   "unregistered" B_MIDI_EVENT to let the client know that this endpoint
   is no longer available. If the endpoint was connected to anything,
   you'll also receive "disconnected" B_MIDI_EVENTs.

-  If you get a "registered" event, and you do FindEndpoint() for that
   id, you'll get its BMidiEndpoint object. If you get an "unregistered"
   event, then FindEndpoint() returns NULL. So the events are send
   *after* the roster is modified.

--------------

Event tests
-----------

*Required:* Several client apps that create and register consumer
endpoints that override the various MIDI event hook functions, as well
as producer endpoints that spray MIDI events. Also useful is a tool that
lets you make connections between all these endpoints (PatchBay), and a
tool that lets you monitor the MIDI events (MidiMonitor).

-  BMidiLocalProducer's spray functions should only try to send
   something if there is one or more connected consumer. If the spray
   functions cannot deliver their events, they simply ignore that
   consumer until the next spray. (No connections are broken or
   anything.)

-  All spray functions except SprayData() should set the atomic flag to
   true, even SpraySystemExclusive().

-  When you send a sysex message using SpraySystemExclusive(), it should
   add 0xF0 in front of your data and 0xF7 at the back. When you call
   SprayData() instead, no bytes are added to the MIDI event data.

-  Verify that all events arrive correctly and that the latency is
   minimal, even when the load is heavy (i.e. many events are being
   sprayed to many different consumers).

-  Verify that the BMidiLocalConsumer destructor properly destroys the
   corresponding port and event thread before it returns.

-  BMidiLocalConsumer should ignore messages that are too small,
   addressed to another consumer, or otherwise invalid.

-  BMidiLocalConsumer's Data() hook should ignore all non-atomic events.
   The rest of the events, provided they contain the correct number of
   bytes for that kind of event, are passed on to the other hooks.

-  Hook a producer up to a consumer and call all SprayXXX() functions
   with a variety of arguments to make sure the correct hooks are being
   called with the correct values. Call SprayData() and
   SpraySystemExclusive() with NULL data and/or length 0.

-  Call GetProducerID() from one of BMidiLocalConsumer's hooks to verify
   that this indeed returns the ID of the producer that sprayed the
   event.

-  To test timeouts, first call SetTimeout(system_time() + 2000000),
   spray an event to the consumer, and wait 2 seconds. The consumer's
   Timeout() hook should now be called. Try again, but now spray
   multiple events to the consumer. The Timeout() hook should still be
   called after 2 seconds, measured from the moment the timeout was set.
   Replace the call to SetTimeout() with SetTimeout(0). After spraying
   the first event, you should immediately get the Timeout() signal,
   because the target time was set in the past. Verify that calling
   SetTimeout() only takes effect after at least one new event has been
   received.

--------------

Other tests
-----------

-  Kill the server. Now run a client app. It should recognize that the
   server isn't running, and return error codes on all operations. Also
   kill the server while the test app is running. From then on, the
   client app will return error codes on all operations. Also bring it
   back up again while the test app is still running. Now the client
   app's request messages will be delivered to the server again, but the
   server will ignore them, because our app did not register with this
   new instance of the server.

-  Start the midi_server and several client apps. Use PatchBay to make
   and break a whole bunch of connections. Quit PatchBay. Start it
   again. Now the same connections should show up. Run similar tests
   with MidiKeyboard. Also install VirtualMidi (and run the old
   midi_server for the time being) to get a whole bunch of fake MIDI
   devices.

-  *Regression bug:* After you quit one client app, another app fails to
   send request to the midi_server.

   *Required:* Client app that creates a new endpoint and registers it.
   In the app's destructor, it unregisters and releases the endpoint.

   *How to reproduce:* Run the app from two different Terminals. Ctrl-C
   app1. Start app1 again. From the Deskbar quit both apps at the same
   time (that is possible because app1 and app2 both have the same
   signature). When it tries to send the Unregister() request to the
   midi_server, app2 gives the error "Cannot send msg to server". The
   error code is "Bad Port ID", which means that the reply port is dead.
   The Mdel message from Release() is sent without any problems,
   however, because that expects no reply back. This is not the only way
   to reproduce the problem, but it seems to be the most reliable one.

   The reason this happens is because you kill app1. When app2 sends a
   synchronous request to the midi_server, the server re-used that same
   message to notify the other apps. (Because it already contained all
   the necessary fields.) But app1 is dead, the notification fails, and
   this (probably) wipes out the reply address in the message. I changed
   the midi_server to create new BMessages for the notifications, and
   was no longer able to reproduce the problem.
