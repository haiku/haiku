Input Processing
================

Input Server messages
---------------------

The Input Server collects information about keyboard and mouse events
and forwards them to the app_server via messages. They are sent to port
specifically for such messages, and the port is monitored by a thread
whose task is to monitor, process, and dispatch them to the appropriate
recipients. The Input Server is a regular BApplication, and unlike other
applications, it requests a port to which it can send input messages.

Mouse
-----

Mouse events consist of button changes, mouse movements, and the mouse
wheel. The message will consist of the time of the event and attachments
appropriate for each message listed below:

B_MOUSE_DOWN
............

- when
- buttons' status
- location of the cursor
- modifiers
- clicks

B_MOUSE_UP
..........

- time
- buttons' status
- location of the cursor
- modifiers

B_MOUSE_MOVED
.............

- time
- location of the cursor
- buttons' status

B_MOUSE_WHEEL_CHANGED
.....................

- time
- location of the cursor
- transit - in or out
- x delta
- y delta

Keyboard
--------

Keyboard events consist of notification when a key is pressed or
released. Any keypress or release will evoke a message, regardless of
whether or not the key is mapped. The message will consist of the
appropriate code and attachments listed below:

B_KEY_DOWN
..........

- time
- key code
- repeat count
- modifiers
- states
- UTF-8 code
- string generated
- modifier-independent ASCII code

B_KEY_UP
........

- time
- key code
- modifiers
- states
- UTF-8 code
- string generated
- modifier-independent ASCII code

B_UNMAPPED_KEY_DOWN
...................

- time
- key code
- modifiers
- states

B_UNMAPPED_KEY_UP
.................

- time
- key code
- modifiers
- states

B_MODIFIERS_CHANGED
...................

sent when a modifier key changes

- time
- modifier states
- previous modifier states
- states

Nearly all keypresses received by the app_server are passed onto the
appropriate application. Control-Tab, when held, is sent to the Deskbar
for app switching. Command+F?? is intercepted and a workspace is
switched. Left Control + Alt + Delete is not even intercepted by the
app_server. The Input Server receives it and shows the Team Monitor
window.


