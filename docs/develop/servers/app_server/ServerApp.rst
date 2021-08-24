ServerApp class
###############

ServerApps are the server-side counterpart to BApplications. They
monitor for messages for the BApplication, create BWindows and BBitmaps,
and provide a channel for the app_server to send messages to a user
application without having a window.

Member Functions
================

- ServerApp(port_id sendport, port_id rcvport, const char\*signature, thread_id  thread_bapp)
- ~ServerApp(void)
- bool Run(void)                    
- static int32 MonitorApp(void \*data)
- void Lock(void)       
- void Unlock(void)        
- bool IsLocked(void)            
- void WindowBroadcast(int32 code)  
- bool IsActive(void)            
- bool PingTarget(void)          
- void DispatchMessage(int32 code, int8 \*buffer)              

ServerApp(port_id sendport, port_id rcvport, const char \*sig, thread_id thread_bapp)
-------------------------------------------------------------------------------------

1. Create the window list as empty
2. Save sendport, rcvport, sig, and thread_bapp to the respective
   ServerApp members
3. Set quit_app flag to false
4. Create the window list lock

~ServerApp(void)
----------------

1. Empty and delete window list and accompanying windows
2. Wait for the monitoring thread to exit
3. Call CursorManager::RemoveAppCursors(this)
4. Delete the window list lock
5. If monitoring thread still active, kill it (in case app is deleted
   without a quit message)

bool Run(void)
--------------

Run() simply makes a ServerApp start monitoring for messages from its
BApplication, telling it to quit if there is a problem.

1. Spawn the monitoring thread (which utilizes MonitorApp()) 2) If any
   error, tell the BApplication to quit, spit an error to stderr, and
   return false
2. Resume the monitoring thread
3. Return true

static int32 MonitorApp(void \*data)
------------------------------------

Thread function for monitoring for messages from the ServerApp's
BApplication.

1. Call port_buffer_size - which will block if the port is empty
2. Allocate a buffer on the heap if the port buffer size is greater than 0
3. Read the port
4. Pass specified messages to DispatchMessage() for processing, spitting
   out an error message to stderr if the message's code is unrecognized
5. Return from DispatchMessage() and free the message buffer if one was
   allocated
6. If the message code matches the B_QUIT_REQUESTED definition and the
   quit_app flag is true, fall out of the infinite message-monitoring loop.
   Otherwise continue to next iteration
7. Send a DELETE_APP message to the server's main message to force
   deleting of the ServerApp instance and exit

bool IsActive(void)
-------------------

Used for determining whether the application is the active one. Simply
returns the isactive flag.

void PingTarget(void)
---------------------

PingTarget() is called only from the Picasso thread of the app_server
in order to determine whether its respective BApplication still
exists. BApplications have been known to crash from time to time
without the common courtesy of notifying the server of its intentions.
;D

1. Call get_thread_info() with the app's thread_id
2. if it returns anything but B_OK, return false. Otherwise, return
   true.

void DispatchMessage(int32 code, int8 \*buffer)
-----------------------------------------------

DispatchMessage implements all the code necessary to respond to a
given message sent to the ServerApp on its receiving message port.
This allows for clearer and more manageable code.

CREATE_WINDOW
.............

Sent by a new BWindow object via synchronous PortLink messaging. Set
up the corresponding ServerWindow and reply to the BWindow with the
new port to which it will send future communications with the App
Server.

Attached Data:

+-----------------------------------+-----------------------------------+
| port_id reply_port                | port to which the server is to    |
|                                   | reply in response to the current  |
|                                   | message                           |
+-----------------------------------+-----------------------------------+
| BRect wframe                      | frame of the requesting BWindow   |
+-----------------------------------+-----------------------------------+
| uint32 wflags                     | flag data of the requesting       |
|                                   | BWindow                           |
+-----------------------------------+-----------------------------------+
| port_id win_port                  | receiver port of the requesting   |
|                                   | BWindow                           |
+-----------------------------------+-----------------------------------+
| uint32 workspaces                 | workspaces on which the BWindow   |
|                                   | is to appear                      |
+-----------------------------------+-----------------------------------+
| const char \*title                | title of the requesting BWindow   |
+-----------------------------------+-----------------------------------+

1. Get all attached data
2. Acquire the window list lock
3. Allocate a ServerWindow object and add it to the list
4. Release window list lock
5. Send the message SET_SERVER_PORT (with the ServerWindow's receiver
   port attached to the reply port

DELETE_APP
..........

Sent by a ServerWindow when told to quit. It is identified by the
unique ID assigned to its thread.

Attached Data:

+-----------------------------------+-----------------------------------+
| thread_id win_thread              | Thread id of the ServerWindow     |
|                                   | sending this message              |
+-----------------------------------+-----------------------------------+

1. Get window's thread_id
2. Acquire window list lock
3. Iterate through the window list, searching for the ServerWindow
   object with the sent thread_id
4. Remove the object from the list and delete it
5. Release window list lock

SET_CURSOR_DATA
...............

Received from the ServerApp's BApplication when SetCursor(const void\*) is called.

Attached Data:

+-----------------------------------+-----------------------------------+
| int8 cursor[68]                   | Cursor data in the format as      |
|                                   | defined in the BeBook             |
+-----------------------------------+-----------------------------------+

1. Create a ServerCursor from the attached cursor data
2. Add the new ServerCursor to the CursorManager and then call
   CursorManager::SetCursor

SET_CURSOR_BCURSOR
..................

Received from the ServerApp's BApplication when SetCursor(BCursor \*,
bool) is called.

Attached Data:

+-----------------------------------+-----------------------------------+
| int32 token                       | Token identifier of cursor in the |
|                                   | BCursor class                     |
+-----------------------------------+-----------------------------------+

1) Get the attached token and call CursorManager::SetCursor(token)

B_QUIT_REQUESTED
................

Received from the BApplication when quits, so set the quit flag and
ask the server to delete the object

Attached Data: None


1) Set quit_app flag to true

UPDATE_DECORATOR
................

Received from the poller thread when the window decorator for the
system has changed.

Attached Data: None

1) Call WindowBroadcast(UPDATE_DECORATOR)

void WindowBroadcast(int32 code)
--------------------------------

Similar to AppServer::Broadcast(), this sends a message to all
ServerWindows which belong to the ServerApp.

1) Acquire window list lock
2) Create a PortLink instance and set its message code to the passed
   parameter.
3) Iterate through the window list, targeting the PortLink instance to
   each ServerWindow's message port and calling Flush().
4) Release window list lock

void Lock(void), void Unlock(void), bool IsLocked(void)
-------------------------------------------------------

These functions are used to regulate access to the ServerApp's data
members. Lock() acquires the internal semaphore, Unlock() releases it,
and IsLocked returns true only if the semaphore's value is positive.
