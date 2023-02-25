Application Server
============================================

Purpose
~~~~~~~

The app_server provides services to the Haiku by managing processes,
filtering and dispatching input from the Input Server to the appropriate
applications, and managing all graphics-related tasks.

Tasks performed by app_server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The tasks performed by the app_server are grouped in relation to its purpose.

- Receives and redirects (dispatches) messages from the input server
- Responds to messages from apps
- Receives and consolidates requests from BView, BWindow, BBitmap, and others to draw stuff (draw bitmap, etc)
- Utilizes ports to communicate with child processes
- Handles drag & drop messaging
- Manages the system clipboard
- Loads and Kills processes
- Detects absence of Input Server and restarts when not running
- Aids in system shutdown
- Dynamically loads accelerant portion of graphics driver
- Creates a connection with BBitmaps requiring a child view
- Draws the blue desktop screen
- Provides workspace support
- Provides functionality to the BeAPI for drawing primitives, such as rectangles, ellipses, and beziers
- Provides a means for BViews to draw on BBitmaps
- Manages window behavior with respect to redraw (move to front, minimize, etc)
- Returns a frame buffer to direct-access classes
- Caches fonts for screen and printer use
- Draws text and provides other font API support for the BeAPI classes

App server components
~~~~~~~~~~~~~~~~~~~~~

.. toctree::

   /servers/app_server/graphics
   /servers/app_server/process_management
   /servers/app_server/input
   /servers/app_server/messaging

-  `Multiple Monitor Support Spec <MultiMonitor.htm>`__

Class Descriptions
~~~~~~~~~~~~~~~~~~

Application Management

.. toctree::

   /servers/app_server/AppServer
   /servers/app_server/ServerApp
   /servers/app_server/SharedObject
   /servers/app_server/TokenHandler
   /servers/app_server/DebugTools

Graphics Management

.. toctree::

   /servers/app_server/BitmapManager
   /servers/app_server/ColorUtils
   /servers/app_server/CursorManager
   /servers/app_server/Decorator
   /servers/app_server/Desktop
   /servers/app_server/DesktopClasses
   /servers/app_server/DisplayDriver
   /servers/app_server/Layer
   /servers/app_server/PatternHandler
   /servers/app_server/RGBColor
   /servers/app_server/ServerBitmap
   /servers/app_server/SystemPalette
   /servers/app_server/WinBorder

Font Infrastructure

.. toctree::

   /servers/app_server/FontServer
   /servers/app_server/FontFamily

