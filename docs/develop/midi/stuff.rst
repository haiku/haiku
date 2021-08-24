Misc notes
==========

-  **MPU401 kernel module.** If your soundcard supports MIDI input and
   output, chances are that it is powered by an MPU401 chip. Because
   this interface is so popular, BeOS comes with a kernel module that
   makes it easy to write drivers for the MPU401. Thanks to Greg Crain,
   we now have an open source version of this kernel module.

   The mpu401 module lives in ``src/add-ons/kernel/generic/mpu401``. It
   supports both the v1 and (undocumented) v2 protocols, although v2 is
   not complete since we don't really know how it works. Unfortunately,
   almost no existing drivers use v1; most of the drivers provided by Be
   require v2. Currently, the module returns B_ERROR when a MIDI device
   is opened with v2.

   For an example on how to use the MPU401 module in your own driver,
   see the source code for the "emuxki" driver elsewhere in the source
   tree.

-  **Clients without a BApplication.** Sometimes the midi_server's debug
   output shows an "Application -1 not registered" error message. This
   means it cannot figure out which app an incoming BMessage came from.
   The server ignores those messages.

   How can this happen? libmidi2 has two ways of sending messages to the
   midi_server: it either expects a reply back or not. In the first
   case, it is obvious to the midi_server what the reply address of the
   message is. In the second case, even though it is not necessary for
   the server to send a message back, it still uses the reply address to
   determine which app the message came from. For this, BMessenger uses
   be_app_messenger of the client app.

   However, if the client app has no BApplication object, there is no
   be_app_messenger either. Now, the midi_server cannot determine where
   the message came from and will ignore it. Is this important? For
   example, when such a client app Release()'s its endpoints, it sends a
   message to the server without a return address. Now the server
   ignores that message and does not remove the endpoint from the
   roster. Of course, after the client app has died, the endpoints will
   be removed eventually. Does all of this matter? Not really, because
   only trivial apps will have no BApplication object.
