Graphics
=========

Design overview
---------------

The app_server drawing system was designed in BeOS with the goal to provide low latency response
(it should look fast), making use of the quite powerful CPU, but somewhat limited RAM available
at the time.

As a result of these constraints, in BeOS the app_server operated with a single buffer framebuffer
(there was not enough RAM and especially not enough video RAM to enable double buffer). It is also
designed to use 2D acceleration whenever possible, to free up the CPU for other, more interesting
tasks. This is achived by the use of "accelerants", add-ons loaded into app-server that communicate
with the kernel part of graphics drivers. Usually the kernel part will be as minimal as possible,
providing low-level access to the video card (command ring buffers, memory mapping of registers,
DMA setup, that kind of things), and the accelerant will be doing the higher level work on top of
it. Note, however, that on modern hardware, the graphics card acceleration is often not that fast
for 2D work, compared to the power offered by multi-GigaHerz CPUs. So, Haiku does not currently use
most of these acceleration features, doing its drawing using the CPU instead.

The single buffer approach creates a problem: applications that are too slow to redraw things can
result in "glitches" on screen. These are of mainly two types: flickering and stamping. The
app_server in Haiku takes some care to avoid these.

Desktop Initialization
-----------------------

The graphics hardware is abstracted from the rest of the app_server.
When started, the server creates the desktop, which is little more than
a collection of workspaces. The desktop actually creates a DisplayDriver
and then calls the driver's method Inititialize() before calling a few
high-level routines for setup. Below is the process by which the
HWDriver class, which is used to access the primary graphics card in the
system, followed by the steps taken to set up the desktop.

Load Accelerant
...............

First of all, the available video cards are scanned by enumerating the contents of /dev/graphics.
For each device, the B_GET_ACCELERANT_SIGNATURE ioctl is used to find the corresponding accelerant
name.

The app_server looks for an accelerant matching that name in the "accelerants" subdirectory of each
add-on directory (enumerated using BPathFinder). The first matching accelerant is loaded
using load_add_on(). Then the get_accelerant_hook() function is obtained through get_image_symbol.
This is the only needed entry point for the accelerant, and can be used to call all the other
needed code, starting with B_INIT_ACCELERANT.

For more information about the accelerant hooks, see `Writing video card drivers <https://www.haiku-os.org/legacy-docs/writing-video-card-drivers/04-accelerant>`_.

Set up workspaces
.................

Workspace preferences are read in from disk. If they exist, they are
used; otherwise the default of 3 workspace, each with the settings
640x480x256@59.9Hz, is used. Each workspace is initialized to the proper
information (preferences or default). Additionally, all settings are
checked and possibly "clipped" by information gained through the driver
class. With the desktop having been given the proper settings, the
default workspace, 0, is activated.

Display
.......

Provided that everything has gone well so far, the screen is filled to
the user-set workspace color or RGB(51,102,160) Also, the global
clipboard is created, which is nothing more than a BClipboard object.

The Input Server will notify the app_server of its own existence, at which
point the cursor will be set to B_HAND_CURSOR and shown on the screen.

Window management
-----------------

Window management is a complicated issue, requiring the cooperation of a
number of different types of elements. Each BApplication, BWindow, and
BView has a counterpart in the app_server which has a role to play.
These objects are Decorators, ServerApps, ServerWindows, Layers, and
WindowBorders.

ServerApps
..........

ServerApp objects are created when a BApplication notifies the
app_server of its presence. In acknowledging the BApplication's
existence, the server creates a ServerApp which will handle future
server-app communications and notifies the BApplication of the port to
which it must send future messages.

ServerApps are each an independent thread which has a function similar
to that of a BLooper, but with additional tasks. When a BWindow is
created, it spawns a ServerWindow object to handle the new window. The
same applies to when a window is destroyed. Cursor commands and all
other BApplication functions which require server interaction are also
handled. B_QUIT_REQUESTED messages are received and passed along to the
main thread in order for the ServerApp object to be destroyed. The
server's Picasso thread also utilizes ServerApp::PingTarget in order to
determine whether the counterpart BApplication is still alive and
running.

ServerWindows
.............

ServerWindow objects' purpose is to take care of the needs of BWindows.
This includes all calls which require a trip to the server, such as
BView graphics calls and sending messages to invoke hook functions
within a window.

Layers
......

Layers are shadowed BViews and are used to handle much BView
functionality and also determine invalid screen regions. Hierarchal
functions, such as AddChild, are mirrored. Invalid regions are tracked
and generate Draw requests which are sent to the application for a
specific BView to update its part of the screen.

WindowBorders
.............

WindowBorders are a special kind of Layer with no BView counterpart,
designed to handle window management issues, such as click tests, resize
and move events, and ensuring that its decorator updates the screen
appropriately.

Decorators
..........

Decorators are addons which are intended to do one thing: draw the
window frame. The Decorator API and development information is described
in the Decorator Development Reference. They are essentially the means
by which WindowBorders draw to the screen.

How It All Works
................

The app_server is one large, complex beast because of all the tasks it
performs. It also utilizes the various objects to accomplish them. Input
messages are received from the Input Server and all messages not
specific to the server (such as Ctrl-Alt-Shift-Backspace) are passed to
the active application, if any. Mouse clicks are passed to the
ServerWindow class for hit testing. These hit tests can result in window
tabs and buttons being clicked, or mouse click messages being passed to
a specific view in a window.

These input messages which are passed to a running application will
sometimes cause things to happen inside it, such as button presses,
window closings/openings, etc. which will cause messages to be sent to
the server. These messages are sent either from a BWindow to a
ServerWindow or a BApplication to a ServerApp. When such messages are
sent, then the corresponding app_server object performs an appropriate
action.

Screen Updates
--------------

Managing invalidation
.....................

The drawing is architectured around a single framebuffer, where all windows can draw.
In general, redrawing should be avoided when not necessary, and if possible, multiple drawing
requests should be combined together to avoid redrawing the same area over and over.

To achieve this, a protocol to decide which parts of the screen need to be redrawn is implemented.

When something needs to change, that region is marked as "invalidated" and the app_server will
ask the corresponding view to redraw itself. Invalidation can happen in two ways:

- Window management events (a window was resized or hidden, for example)
- The application asked to redraw something by calling Invalidate()

These two are handled slightly differently. When the event comes from window management, one of
the views involved will have parts of it newly exposed (previously they were hidden by another
window that is now hidden, or they were outside the window bounds, for example). In this case,
app_server will immediately fill the newly exposed area with the view color. This avoids one of
the two drawing problems when applications are too slow to redraw: stamping. For example, if one
windows is not redrawing fast enough, and another is moved above it, that movement will quickly
hide and re-expose parts of the bottom window. If the window does not redraw fast enough, and
nothing is done, we would be left with parts of the top window being partially drawn where they
shouldn't be anymore.

In the case of invalidation coming from the view itself, however, things are a bit different. We
can assume that the view had already drawn something at that place. If we cleared the area to the
view color, and the view takes a little time to redraw, this would result in flickering: the view
would be briefly visible with only its view color, and then redrawn with its content again. So,
in the case of invalidation, the app_server does nothing, and waits for the view to redraw itself.

Getting things drawn on screen
..............................

Screen updates are done entirely through the BView class or some
subclass thereof, hereafter referred to as a view. A view's drawing
commands will cause its window to store draw command messages in a
message packet. At some point Flush() will be called and the command
packet will be sent to the window's ServerWindow object inside the
server.

The ServerWindow will receive the packet, check to ensure that its size
is correct, and begin retrieving each command from the packet and
dispatching it, taking the appropriate actions. Actual drawing commands,
such as StrokeRect, will involve the ServerWindow object calling the
appropriate command in the graphics module for the Layer corresponding
to the view which sent the command.

The commands are grouped together in a drawing session, that corresponds to a call to the
BView::Draw() method. In Haiku, the app_server uses double buffering, and all the drawing from
one session will be done on the backbuffer, and moved to the front buffer only when the session is
complete. The normal workflow is to trigger this by a request to draw something (either an
"expose" event because a part of the window has become visible, or a call to the Invalidate function
from the application side). However, it is also possible for views to send "unsollicited" drawing
commands outside of an update session. While this will work, the lack of a session means each
command will be handled separately, and immediately copied to the front buffer. As a result, there
will be more fickering and the drawing will be a lot slower.

When interpreting the drawing commands, app_server will prevent any drawing from happening outside
the area designated for a given view, including parts of it that could be hidden by other windows.
There is an exception to this, however: when using BDirectWindow, it is possible to access the
whole frame buffer. In this case, app_server provides the application with a BRegion it should
redraw, and it is up to the application to not draw ouside those bounds.

Offscreen views
...............

When a view does very complex drawing, that will take more than a frame to complete, the single
framebuffer design is not desirable, and will result in a lot of flickering as partially drawn
states of the view are shown on screen. To avoid this, the app_server provides the option for a
view to draw off-screen, into a BBitmap. When the bitmap is complete, it can then be put on-screen
using another view.

This can be done in two ways: either using DrawBitmap() or SetViewBitmap(). The latter is better,
since it simply lets app_server know that the view should show that bitmap, and then there is no
need to do anything to handle expose and invalidate events, the app_server can automatically draw
the bitmap instead of using the view color to fill the newly exposed or invalidated area.

Overlays
........

When view bitmaps are not enough, it is possible to go one step further: have the hardware insert
the picture inside a view, instead of app_server having to copy it in the framebuffer. This is
achieved using overlays. The API is similar to SetViewBitmap, but the bitmap is allocated directly
in video memory and managed by the video card. Unfortunately, not all video drivers currently
support this feature.

It is possible to mix overlays with normal drawing. The overlay is normally made visible only when
the framebuffer is a certain specific color(usually pure green or pure magenta, the specific
color is determined by the graphics driver and multiple colors may be used for multiple overlays
from different views, if the hardware can do that). The application can then simply let the view be
filled with that 'color key' (setting it as the view color), or it can draw other things that will
be displayed over the 'overlay' picture.

Depending on the graphics hardware, overlays can also be resized in hardware, and use a different
colorspace from other parts of the framebuffer (for example, a video overlay can be in YUV format
while the framebuffer is in RGB or even in a 256 color palette mode).

Cursor Management
-----------------

The app_server handles all messiness to do with the cursor. The cursor
commands which are members of the BApplication class will send a message
to its ServerApp, which will then call the DisplayDriver's appropriate
function. The DisplayDriver used will actually handle the drawing of the
cursor and whether or not to do so at any given time.

In addition to the 1 bit per pixel cursors used in BeOS, Haiku also allows to create a BCursor
object from a BBitmap in any colorspace. This allows color cursors and also larger cursor sizes.
The default cursors also use greyscale and alpha channel for antialiasing.

Display Drivers
---------------

Unlike the BeOS R5 app_server, Haiku' server has an extra abstraction layer between the graphic
driver and the main drawing functions. This allows to generalize the interface and redirect the
drawing commands in various ways. For example, drawing commands can be redirected to another
machine for the remote_app_server, or drawing for a specific window can be granted direct access
to the framebuffer on a specific display and video card, while other applications go through the
normal process of drawing only to their currently exposed region only.


