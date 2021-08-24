
AppServer class
###############

The AppServer class sits at the top of the hierarchy, starting and
stopping services, monitoring for messages, and so forth.

Member Functions
================

- AppServer(void)
- ~AppServer(void)                  
- static int32 Poller(void \*data)  
- static int32 Picasso(void \*data) 
- thread_id Run(void)               
- void MainLoop(void)               
- bool LoadDecorator(const char\*path)                           
- void DispatchMessage(int32 code,int8 \*buffer)                    
- void Broadcast(int32 code)        
- void HandleKeyMessage(int32 code, int8 \*buffer)                    

Global Functions
================

Decorator \* instantiate_decorator(Layer \*owner, uint32 wflags, uint32 wlook)

AppServer(void)
===============

1. Create the message and input ports
2. Create any necessary semaphores for regulating the 3 main threads
3. Initialize all member variables
4. Allocate the application BList
5. Read in and process all configuration data
6. Initialize the desktop
7. Spawn the Picasso and Poller threads

~AppServer(void)
================

1. Shut down the desktop
2. Empty and delete the application list
3. Wait for Picasso and Poller to exit
4. Free any allocated heap space

void MainLoop(void)
===================

MainLoop is one large loop used to monitor the main message port in
the app_server thread. This is a standard port-monitoring loop code:


1. Call port_buffer_size - which will block if the port is empty
2. Allocate a buffer on the heap if the port buffer size is greater than 0
3. Read the port
4. Pass specified messages to DispatchMessage() for processing, spitting
   out an error message to stderr if the message's code is unrecognized
5. Return from DispatchMessage() and free the message buffer if one was
   allocated
6. If the message code matches the B_QUIT_REQUESTED definition and the
   quit_server flag is true, fall out of the infinite message-monitoring
   loop

void DispatchMessage(int32 code, int8 \*buffer)
===============================================

DispatchMessage implements all the code necessary to respond to a
given message sent to the app_server on its main port. This allows for
clearer and more manageable code.


CREATE_APP
----------

Sent by a new BApplication object via synchronous PortLink messaging.
Set up the corresponding ServerApp and reply to the BApplication with
the new port to which it will send future communications with the App
Server.

Attached Data:

+-----------------------------------+-----------------------------------+
| port_id reply_port                | port to which the server is to    |
|                                   | reply in response to the current  |
|                                   | message                           |
+-----------------------------------+-----------------------------------+
| port_id app_port                  | message port for the requesting   |
|                                   | BApplication                      |
+-----------------------------------+-----------------------------------+
| int16 sig_length                  | length of the following           |
|                                   | application signature             |
+-----------------------------------+-----------------------------------+
| const char \*signature            | Signature of the requesting       |
|                                   | BApplication                      |
+-----------------------------------+-----------------------------------+


1. Get all attached data
2. Acquire the application list lock
3. Allocate a ServerApp object and add it to the list
4. Release application list lock
5. Acquire active application pointer lock
6. Update active application pointer
7. Release active application lock
8. Send the message SET_SERVER_PORT (with the ServerApp's receiver port
   attached) to the reply port
9. Run() the new ServerApp instance

DELETE_APP
----------

Sent by a ServerApp when told to quit either by its BApplication or
the Server itself (during shutdown). It is identified by the unique ID
assigned to its thread.

Attached Data:

+-----------------------------------+-----------------------------------+
| thread_id app_thread              | Thread id of the ServerApp        |
|                                   | sending this message              |
+-----------------------------------+-----------------------------------+

1. Get app's thread_id
2. Acquire application list lock
3. Iterate through the application list, searching for the ServerApp
   object with the sent thread_id
4. Remove the object from the list and delete it
5. Acquire active application lock
6. Check to see if the application is active
7. If application is/was active, set it to the previous application in
   the list or NULL if there are no other active applications
8. Release application list lock
9. Release active application lock

GET_SCREEN_MODE
---------------

Received from the OpenBeOS Input Server when requesting the current
screen settings via synchronous PortLink messaging. This is a
temporary solution which will be deprecated as soon as the BScreen
class is complete.

Attached Data:

+-----------------------------------+-----------------------------------+
| port_id reply_port                | port to which the server is to    |
|                                   | reply in response to the current  |
|                                   | message                           |
+-----------------------------------+-----------------------------------+

1. Get height, width, and color depth from the global graphics driver
   object
2. Attach via PortLink and reply to sender

B_QUIT_REQUESTED
----------------

Encountered only under testing situations where the Server is told to
quit.

Attached Data: None

1. Set quit_server flag to true
2. Call Broadcast(QUIT_APP)

SET_DECORATOR
-------------

Received from just about anything when a new window decorator is
chosen

Attached Data:

+-----------------------------------+-----------------------------------+
| const char \*path                 | Path to the proposed new          |
|                                   | decorator                         |
+-----------------------------------+-----------------------------------+

1. Get the path from the buffer
2. Call LoadDecorator()

void Run(void)
==============

Run() exists mostly for consistency with other regular applications.

1) Call MainLoop()

bool LoadDecorator(const char \*path)
=====================================

Allows for a simple way to change the current window decorator
systemwide simply by specifying the path to the desired Decorator
addon.

1. Load the passed string as the path to an addon.
2. Load all necessary symbols for the decorator
3. Return false if things didn't go so well
4. Call Broadcast(UPDATE_DECORATOR)
5. Return true

static int32 Picasso(void \*data)
=================================

Picasso is a function, despite its name, dedicated to ensuring that
the server deallocates resources to a dead application. It consists of
a while(!quit_server) loop as follows:

1) Acquire the appliction list lock
2) Iterate through the list, calling each ServerApp object's
   PingTarget() method.
3) If PingTarget returns false, remove the ServerApp from the list and
   delete it.
4) Release the appliction list lock
5) snooze for 3 seconds

static int32 Poller(void \*data)
================================

Poller is the main workhorse of the AppServer class, polling the
Server's input port constantly for any messages from the Input Server
and calling the appropriate handlers. Like Picasso, it, too, is mostly
a while(!quit_server) loop.

1. Call port_buffer_size_etc() with a timeout of 3 seconds.
2. Check to see if the port_buffer_size_etc() timed out and do a
   continue to next iteration if it did.
3. Allocate a buffer on the heap if the port buffer size is greater than 0
4. Read the port
5. Pass specified messages to DispatchMessage() for processing, spitting
   out an error message to stderr if the message's code is unrecognized
6. Return from DispatchMessage() and free the message buffer if one was
   allocated

Decorator \* instantiate_decorator(Layer \*owner, uint32 wflags, uint32 wlook)
==============================================================================

instantiate_decorator returns a new instance of the decorator
currently in use. The caller is responsible for the memory allocated
for the returned object.

1. Acquire the decorator lock
2. If create_decorator is NULL, create a new instance of the default
   decorator
3. If create_decorator is non-NULL, create a new decorator instance by
   calling AppServer::create_decorator().
4. Release the decorator lock
5. Return the newly allocated instance

void Broadcast(int32 code)
==========================

Broadcast() provides the AppServer class with an easy way to send a
quick message to all ServerApps. Primarily, this is called when a font
or decorator has changed, or when the server is shutting down. It is
not intended to do anything except send a quick message which requires
no extra data, such as for some upadate signalling.

1. Acquire application list lock
2. Create a PortLink instance and set its message code to the passed
   parameter.
3. Iterate through the application list, targeting the PortLink instance
   to each ServerApp's message port and calling Flush().
4. Release application list lock

void HandleKeyMessage(int32 code, int8 \*buffer)
================================================

Called from DispatchMessage to filter out App Server events and
otherwise send keystrokes to the active application.

B_KEY_DOWN
----------

Sent when the user presses (or holds down) a key that's been mapped to
a character.

Attached Data:

+-----------------------------------+-----------------------------------+
| int64 when                        | event time in seconds since       |
|                                   | 1/1/70                            |
+-----------------------------------+-----------------------------------+
| int32 rawcode                     | code for the physical key pressed |
+-----------------------------------+-----------------------------------+
| int32 repeat_count                | number of times a key has been    |
|                                   | repeated                          |
+-----------------------------------+-----------------------------------+
| int32 modifiers                   | flags signifying the states of    |
|                                   | the modifier keys                 |
+-----------------------------------+-----------------------------------+
| int32 state_count                 | number of bytes to follow         |
|                                   | containing the state of all keys  |
+-----------------------------------+-----------------------------------+
| int8 \*states                     | array of the state of all keys at |
|                                   | the time of the event             |
+-----------------------------------+-----------------------------------+
| int8 utf8data[3]                  | UTF-8 data generated              |
+-----------------------------------+-----------------------------------+
| int8 charcount                    | number of bytes to follow         |
|                                   | containing the string generated   |
|                                   | (usually 1)                       |
+-----------------------------------+-----------------------------------+
| const char \*string               | null-terminated string generated  |
|                                   | by the keystroke                  |
+-----------------------------------+-----------------------------------+
| int32 raw_char                    | modifier-independent ASCII code   |
|                                   | for the character                 |
+-----------------------------------+-----------------------------------+

1. Get all attached data
2. If the command modifier is down, check for Left Ctrl+Left Alt+Left
   Shift+F12 and reset the workspace to 640 x 480 x 256 @ 60Hz and return
   if true
3. If the command modifier is down, check for Alt+F1 through Alt+F12 and
   set workspace and return if true
4. If the control modifier is true, check for B_CONTROL_KEY+Tab and, if
   true, find and send to the Deskbar.
5. Acquire the active application lock
6. Create a PortLink instance, target the active ServerApp's sender
   port, set the opcode to B_KEY_DOWN, attach the buffer en masse, and send
   it to the BApplication.
7. Release the active application lock

B_KEY_UP
--------

Sent when the user releases a key that's been mapped to a character.

Attached Data:

+-----------------------------------+-----------------------------------+
| int64 when                        | event time in seconds since       |
|                                   | 1/1/70                            |
+-----------------------------------+-----------------------------------+
| int32 rawcode                     | code for the physical key pressed |
+-----------------------------------+-----------------------------------+
| int32 modifiers                   | flags signifying the states of    |
|                                   | the modifier keys                 |
+-----------------------------------+-----------------------------------+
| int32 state_count                 | number of bytes to follow         |
|                                   | containing the state of all keys  |
+-----------------------------------+-----------------------------------+
| int8 \*states                     | array of the state of all keys at |
|                                   | the time of the event             |
+-----------------------------------+-----------------------------------+
| int8 utf8data[3]                  | UTF-8 data generated              |
+-----------------------------------+-----------------------------------+
| int8 charcount                    | number of bytes to follow         |
|                                   | containing the string generated   |
|                                   | (usually 1)                       |
+-----------------------------------+-----------------------------------+
| const char \*string               | null-terminated string generated  |
|                                   | by the keystroke                  |
+-----------------------------------+-----------------------------------+
| int32 raw_char                    | modifier-independent ASCII code   |
|                                   | for the character                 |
+-----------------------------------+-----------------------------------+

1. Get all attached data
2. Acquire the active application lock
3. Create a PortLink instance, target the active ServerApp's sender
   port, set the opcode to B_KEY_UP, attach the buffer en masse, and send
   it to the BApplication.
4. Release the active application lock

B_UNMAPPED_KEY_DOWN
-------------------

Sent when the user presses a key that has not been mapped to a
character.

Attached Data:

+-----------------------------------+-----------------------------------+
| int64 when                        | event time in seconds since       |
|                                   | 1/1/70                            |
+-----------------------------------+-----------------------------------+
| int32 rawcode                     | code for the physical key pressed |
+-----------------------------------+-----------------------------------+
| int32 modifiers                   | flags signifying the states of    |
|                                   | the modifier keys                 |
+-----------------------------------+-----------------------------------+
| int8 state_count                  | number of bytes to follow         |
|                                   | containing the state of all keys  |
+-----------------------------------+-----------------------------------+
| int8 \*states                     | array of the state of all keys at |
|                                   | the time of the event             |
+-----------------------------------+-----------------------------------+

1. Acquire the active application lock
2. Create a PortLink instance, target the active ServerApp's sender
   port, set the opcode to B_UNMAPPED_KEY_DOWN, attach the buffer en masse,
   and send it to the BApplication.
3. Release the active application lock

B_UNMAPPED_KEY_UP
-----------------

Sent when the user presses a key that has not been mapped to a
character.

Attached Data:

+-----------------------------------+-----------------------------------+
| int64 when                        | event time in seconds since       |
|                                   | 1/1/70                            |
+-----------------------------------+-----------------------------------+
| int32 rawcode                     | code for the physical key pressed |
+-----------------------------------+-----------------------------------+
| int32 modifiers                   | flags signifying the states of    |
|                                   | the modifier keys                 |
+-----------------------------------+-----------------------------------+
| int8 state_count                  | number of bytes to follow         |
|                                   | containing the state of all keys  |
+-----------------------------------+-----------------------------------+
| int8 \*states                     | array of the state of all keys at |
|                                   | the time of the event             |
+-----------------------------------+-----------------------------------+

1. Acquire the active application lock
2. Create a PortLink instance, target the active ServerApp's sender
   port, set the opcode to B_UNMAPPED_KEY_UP, attach the buffer en masse,
   and send it to the BApplication.
3. Release the active application lock

B_MODIFIERS_CHANGED
-------------------

Sent when the user presses or releases one of the modifier keys

Attached Data:

+-----------------------------------+-----------------------------------+
| int64 when                        | event time in seconds since       |
|                                   | 1/1/70                            |
+-----------------------------------+-----------------------------------+
| int32 modifiers                   | flags signifying the states of    |
|                                   | the modifier keys                 |
+-----------------------------------+-----------------------------------+
| int32 old_modifiers               | former states of the modifier     |
|                                   | keys                              |
+-----------------------------------+-----------------------------------+
| int8 state_count                  | number of bytes to follow         |
|                                   | containing the state of all keys  |
+-----------------------------------+-----------------------------------+
| int8 \*states                     | array of the state of all keys at |
|                                   | the time of the event             |
+-----------------------------------+-----------------------------------+

1. Acquire the active application lock
2. Create a PortLink instance, target the active ServerApp's sender
   port, set the opcode to B_MODIFIERS_CHANGED, attach the buffer en masse,
   and send it to the BApplication.
3. Release the active application lock

