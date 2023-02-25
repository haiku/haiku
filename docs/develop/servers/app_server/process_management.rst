Process Management
==================

BApplication execution
-----------------------

Applications will come in two types: those which communicate with the
app_server and take advantage of its services, and those which do not.
To access the app_server, an application must be derived from
BApplication.

When a BApplication (referred to hereafter as a BApp) is executed, the
app constructor creates its BLooper message port with the name
AppLooperPort. This port's id, by means of BLooper, registers its
port_id with the app_server so that the two can communicate with each
other most easily.

When the app_server receives notification that an app has been created,
the server creates an AppMonitor (with accompanying thread) in its own
team to handle messages sent to it and sends a reply with the port_id of
the AppMonitor, to which all future messages are sent. These AppMonitor
objects are stored in a global BList created for the storage of such
things.

This setup normally requires that there is a single app_server instance running, and all
BApplications establish contact with it. There are however several exceptions to this:

- Multi-user and multi-sessions setups are possible, for example with the use of remote_app_server.
  In this case, each user session (corresponding to UNIX 'login' sessions) can have its own
  app_server.
- The test_app_server is a test tool that allows to run most of app_server code in its own BWindow,
  displayed inside the main app_server (mainly for debugging app_server code). In this case, the
  applications are compiled with specific code to connect specifically with the test_app_server.

non-BApplication execution
--------------------------

Other applications do not communicate with the app_server. These
applications have no access to app services and do not generally pass
BMessages. This includes, but is not limited to, UNIX apps. The
app_server ignores such applications except when asked to kill them.

While, technically, these are not limited to being non-GUI applications,
in practice these applications are command-line-only, for the
application would be required to (1) render the app_server unable to
access video hardware and (2) reinvent existing graphics code to load
and use accelerants and draw onto the video buffer. This is extremely
bad style and programming practice, not to mention more work than it is
worth.

Killing/Exiting Applications
----------------------------

While the input server handles the Team Monitor window, the app_server
actually takes care of shutting down teams, peacefully or not. Exiting
an app is done simply by sending a B_QUIT_REQUESTED message to
particular app. Killing an app is done via kill_team, but all the messy
details are handled by the kernel itself through this call. When the
user requests a team die via the Team Monitor, the Input Server sends a
message to the app_server to kill the team, attaching the team_id. The
app_server responds by happily nuking the respective team and notifies
the registrar of its forcible removal from the roster.

System Shutdown
---------------

Although the server maintains an internal list of running GUI
applications, when a request to shut down the system is received by the
app_server, it will pass the request on to the registrar, which will, in
turn, increment its way through the app roster and request each app
quit. When each quit request is sent, a timer is started and after
timeout, the registrar will ask the server to kill the particular team
and continue iterating through the application list.


