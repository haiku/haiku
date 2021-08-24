Registrar Protocols
===================

Standard Replies
----------------

standard success reply message
..............................

reply:		B_REG_SUCCESS

- [ <additional fields> ]

fields:

- <additional fields>: Request-specific fields.

standard error reply message
............................

reply:		B_REG_ERROR

- "error": B_INT32_TYPE
- [ "error_description": B_STRING_TYPE ]
- [ <additional fields> ]

fields:

- "error": The error code (a status_t).
- "error_description": Optional human readable description.
- <additional fields>: Request-specific fields.


standard general result reply message
.....................................

reply:		B_REG_RESULT

- "result": B_INT32_TYPE
- [ "result_description": B_STRING_TYPE ]
- [ <additional fields> ]

fields:

- "result": The result value (a status_t).
- "result_description": Optional human readable description.
- <additional fields>: Request-specific fields.

General Requests
----------------

Getting the messengers
......................

for MIME, clipboard and disk device management respectively

+--------------+-------------------------------------------------------+
| target       | registrar app looper (preferred handler)              |
+--------------+-------------------------------------------------------+
| message      | - B_REG_GET_MIME_MESSENGER                            |
|              | - B_REG_GET_CLIPBOARD_MESSENGER                       |
|              | - B_REG_GET_DISK_DEVICE_MESSENGER                     |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - "messenger": B_MESSENGER_TYPE                       |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error (fatal)                              |
+--------------+-------------------------------------------------------+

reply fields:

- "messenger": The requested messenger.

Shut down
.........

+--------------+-------------------------------------------------------+
| target       | registrar app looper (preferred handler)              |
+--------------+-------------------------------------------------------+
| message      | B_REG_SHUT_DOWN                                       |
|              |                                                       |
|              | - "reboot":			B_BOOL_TYPE            |
|              | - "confirm":			B_BOOL_TYPE            |
|              | - "synchronously":	B_BOOL_TYPE                    |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error (fatal)                              |
+--------------+-------------------------------------------------------+

message fields:

- "reboot": If true, the system reboots instead of turning the power off.
- "confirm": If true, the user will be asked to confirm to shut down the system.
- "synchronously": If true, the request sender gets a reply only, if the
  shutdown process fails. Otherwise the reply will be sent
  right before asking the user for confirmation (if desired).

Roster Requests
---------------

App registration (BRoster::AddApplication())
............................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_ADD_APP                                         |
|              |                                                       |
|              | - "signature":			B_MIME_STRING_TYPE     |
|              | - "ref":			B_REF_TYPE             |
|              | - "flags":			B_UINT32_TYPE          |
|              | - "team":			B_INT32_TYPE           |
|              | - "thread":			B_INT32_TYPE           |
|              | - "port":			B_INT32_TYPE           |
|              | - "full_registration":	        B_BOOL_TYPE            |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - [ "token":				B_INT32_TYPE ] |
|              | - [ "other_team":			B_INT32_TYPE ] |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "signature": The application signature.
- "ref": An entry_ref to the application executable.
- "flags": The application flags.
- "team": The application team (team_id).
- "port": The app looper port (port_id).
- "full_registration": Whether full or pre-registration is requested.

reply fields:

- "token": If pre-registration was requested (uint32). Unique token to be
  passed to BRoster::SetThreadAndTeam().
- "other_team": For single/exclusive launch applications that are launched
  the second time. The team ID of the already running instance (team_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an invalid value.
  - B_ENTRY_NOT_FOUND: The entry_ref doesn't refer to a file.
  - B_ALREADY_RUNNING: For single/exclusive launch applications that are launched the second time.
  - B_REG_ALREADY_REGISTERED: The team is already registered.
  - ...

app registration (BRoster::CompleteRegistration())
..................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_COMPLETE_REGISTRATION                           |
|              |                                                       |
|              | - "team":		B_INT32_TYPE                   |
|              | - "thread":	B_INT32_TYPE                           |
|              | - "port":		B_INT32_TYPE                   |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The application team (team_id).
- "thread": The application looper thread (thread_id).
- "port": The app looper port (port_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_REG_APP_NOT_PRE_REGISTERED: The team is unknown to the roster or the
    application is already fully registered.
  - ...

app registration check (BRoster::IsAppRegistered())
...................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_IS_APP_REGISTERED                               |
|              |                                                       |
|              | - "ref":		B_REF_TYPE                     |
|              | - ( "team":	B_INT32_TYPE | "token":	B_INT32_TYPE ) |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - "registered":		B_BOOL_TYPE            |
|              | - "pre-registered":	B_BOOL_TYPE                    |
|              | - [ "app_info":		flattened app_info ]   |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "ref": An entry_ref to the application executable.
- "team": The application team (team_id).
- "token": The application's preregistration token.

reply fields:

- "registered": true, if the app is (pre-)registered, false otherwise.
- "pre-registered": true, if the app is pre-registered, false otherwise.
- "app_info": Flattened app info, if the app is known (i.e. (pre-)registered).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_ENTRY_NOT_FOUND: The entry_ref doesn't refer to a file.
  - ...

pre-registrated app unregistration (BRoster::RemovePreRegApp())
...............................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_REMOVE_PRE_REGISTERED_APP                       |
|              |                                                       |
|              | - "token":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "token": The token BRoster::AddApplication() returned (uint32).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_REG_APP_NOT_PRE_REGISTERED: The token does not identify a pre-registered
    application.
  - ...

app unregistration (BRoster::RemoveApp())
.........................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_REMOVE_APP                                      |
|              |                                                       |
|              | - "team":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The application team (team_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_REG_APP_NOT_REGISTERED: The team is unknown to the roster.
  - ...

app pre-registration, set thread/team (BRoster::SetThreadAndTeam())
...................................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_SET_THREAD_AND_TEAM                             |
|              |                                                       |
| 	       | - "token":	B_INT32_TYPE                           |
|	       | - "team":		B_INT32_TYPE                   |
| 	       | - "thread":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "token": The token BRoster::AddApplication() returned (uint32).
- "team": The application team (team_id).
- "thread": The application looper thread (thread_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_REG_APP_NOT_PRE_REGISTERED: The token does not identify a pre-registered
    application.
  - ...

app registration, change app signature (BRoster::SetSignature())
................................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_SET_SIGNATURE                                   |
|              |                                                       |
|	       | - "team":			B_INT32_TYPE           |
|	       | - "signature":	B_STRING_TYPE                          |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The application team (team_id).
- "signature": The application's new signature.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_REG_APP_NOT_REGISTERED: The team does not identify a registered
    application.
  - ...

get an app info (BRoster::Get{Running,Active,}AppInfo())
........................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_GET_APP_INFO                                    |
|              |                                                       |
| 	       | [ "team":			B_INT32_TYPE           |
| 	       |  | "ref":			B_REF_TYPE             |
| 	       |  | "signature":	B_STRING_TYPE ]                |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
| 	       | - "app_info":	B_REG_APP_INFO_TYPE                    |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The application team (team_id).
- "ref": An entry_ref to the application executable.
- "signature": The application signature.
- If both are omitted the active application is referred to.

reply fields:

- "app_info": The requested app_info (flat_app_info).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_BAD_TEAM_ID: The supplied team ID does not identify a registered
    application.
  - B_ERROR:

    - An entry_ref or a signature has been supplied and no such application
      is currently running.
    - No team ID has been supplied and currently there is no active
      application.

  - ...

get an app list (BRoster::GetAppList())
.......................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_GET_APP_LIST                                    |
|              |                                                       |
|              | - [ "signature":	B_STRING_TYPE ]                |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - "teams":	B_INT32_TYPE[]                         |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "signature": The application signature.

reply fields:

- "teams": The requested list of team IDs (team_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

activate an app (BRoster::ActivateApp())
........................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_ACTIVATE_APP                                    |
|              |                                                       |
|              | "team":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The application team (team_id).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_BAD_TEAM_ID: The supplied team ID does not identify a registered
    application.
  - ...

broadcast a message (BRoster::Broadcast())
..........................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_BROADCAST                                       |
|              |                                                       |
| 	       | - "team":			B_INT32_TYPE           |
| 	       | - "message":		B_MESSAGE_TYPE                 |
| 	       | - "reply_target":	B_MESSENGER_TYPE               |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": The requesting team (team_id).
- "message": The message to be broadcast.
- "reply_target": The reply target for the message.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

start roster watching (BRoster::StartWatching())
................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_START_WATCHING                                  |
|              |                                                       |
| 	       | - "target":	B_MESSENGER_TYPE                       |
| 	       | - "events":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "target": The target the event messages shall be sent to.
- "events": Specifies the events the caller is interested in (uint32).

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

stop roster watching (BRoster::StopWatching())
..............................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_STOP_WATCHING                                   |
|              |                                                       |
|              | - "target":	B_MESSENGER_TYPE                       |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "target": The target that shall not longer receive any event messages.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

MIME Database Requests
----------------------

install a mime type (BMimeType::Install())
..........................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_INSTALL                                    |
|              |                                                       |
|              | - "type": B_STRING_TYPE                               |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "type": The mime type

reply fields:

- "result":

  - B_OK: success
  - B_FILE_EXISTS: the type is already installed
  - ...			

remove a mime type (BMimeType::Delete())
........................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_DELETE                                     |
|              |                                                       |
|              | - "type": B_STRING_TYPE                               |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "type": The mime type

reply fields:

- "result":

  - B_OK: success
  - B_ENTRY_NOT_FOUND: the type was not found
  - (other error code): failure			

set a specific attribute of a mime type (BMimeType::Set*()), installing if necessary
....................................................................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_SET_PARAM                                  |
|              |                                                       |
| 	       | - "type": B_STRING_TYPE                               |
| 	       | - "which": B_INT32_TYPE                               |
| 	       | - [ additional fields depending upon the "which"      |
|              |   field (see below) ]                                 |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "type": The mime type
- "which": Which attribute to set. May be one of the following:

+---------------------------+-------------------------------+-----------------------------------+
| "which"                   | additional message fields     | field comments                    |
+===========================+===============================+===================================+
| B_REG_MIME_ICON           | "icon data": B_RAW_TYPE,      |  B_CMAP8 bitmap data only         |
|                           +-------------------------------+-----------------------------------+
|                           | "icon size": B_INT32_TYPE     | B_{MINI,LARGE}_ICON or -1 for flat|
|                           |                               | vector icon data                  |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_PREFERRED_APP  | "signature": B_STRING_TYPE    |                                   |
|                           +-------------------------------+-----------------------------------+
|                           | "app verb": B_INT32_TYPE      |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_ATTR_INFO      | "attr info": B_MESSAGE_TYPE   |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_FILE_EXTENSIONS| "extensions": B_MESSAGE_TYPE  |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_DESCRIPTION    | "long": B_BOOL_TYPE,          |                                   |
|                           +-------------------------------+-----------------------------------+
|                           | "description": B_STRING_TYPE  |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_SNIFFER_RULE   | "sniffer rule": B_STRING_TYPE |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_APP_HINT       | "app hint": B_REF_TYPE        |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_ICON_FOR_TYPE  | "file type": B_STRING_TYPE,   |                                   |
|                           +-------------------------------+-----------------------------------+
|                           | "icon data": B_RAW_TYPE,      |                                   |
|                           +-------------------------------+-----------------------------------+
|                           | "icon size": B_INT32_TYPE     |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_SUPPORTED_TYPES| "types": B_MESSAGE_TYPE       |                                   |
+---------------------------+-------------------------------+-----------------------------------+

reply fields:

- "result":

  - B_OK: success
  - (error code): failure

delete a specific attribute of a mime type (BMimeType::Delete*()),
..................................................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_DELETE_PARAM                               |
|              |                                                       |
| 	       | - "type": B_STRING_TYPE                               |
| 	       | - "which": B_INT32_TYPE                               |
| 	       | - [ additional fields depending upon the "which"      |
|              |   field (see below) ]                                 |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "type": The mime type
- "which": Which attribute to delete. May be one of the following:

+---------------------------+-------------------------------+-----------------------------------+
| "which"                   | additional message fields     | field comments                    |
+===========================+===============================+===================================+
| B_REG_MIME_ICON:	    | "icon size": B_INT32_TYPE     | B_{MINI,LARGE}_ICON or -1 for     |
|                           |                               | vector icon data                  |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_PREFERRED_APP  | "app verb": B_INT32_TYPE      |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_ATTR_INFO      |                               |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_FILE_EXTENSIONS|                               |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_DESCRIPTION    | "long": B_BOOL_TYPE           |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_SNIFFER_RULE   |                               |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_APP_HINT       |                               |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_ICON_FOR_TYPE  | "file type": B_STRING_TYPE,   |                                   |
|                           +-------------------------------+-----------------------------------+
|                           | "icon size": B_INT32_TYPE     |                                   |
+---------------------------+-------------------------------+-----------------------------------+
| B_REG_MIME_SUPPORTED_TYPES|                               |                                   |
+---------------------------+-------------------------------+-----------------------------------+

reply fields:

- "result":

  - B_OK: success
  - B_ENTRY_NOT_FOUND: no such attribute exists, or the type is not installed
  - (other error code): failure

subscribe a BMessenger to the MIME monitor service (BMimeType::StartWatching())
...............................................................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_START_WATCHING                             |
|              |                                                       |
| 	       | "target": B_MESSENGER_TYPE                            |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "target": The BMessenger subscribing to the monitor service

reply fields:

- "result":

  - B_OK: success
  - (error code): failure

unsubscribe a BMessenger from the MIME monitor service (BMimeType::StopWatching())
..................................................................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_STOP_WATCHING                              |
|              |                                                       |
|              | "target": B_MESSENGER_TYPE                            |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "target": The BMessenger unsubscribing from the monitor service

reply fields:

- "result":

  - B_OK: success
  - B_ENTRY_NOT_FOUND: the given BMessenger was not subscribed to the service
  - (other error code): failure

perform an update_mime_info() call
..................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_UPDATE_MIME_INFO                           |
|              |                                                       |
|	       | - "entry": B_REF_TYPE                                 |
|	       | - "recursive": B_BOOLEAN_TYPE                         |
|	       | - "synchronous": B_BOOLEAN_TYPE                       |
|	       | - "force": B_INT32_TYPE                               |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "entry": The base entry to update.
- "recursive": If true and "entry" is a directory, update all entries
  below "entry" in the hierarchy.
- "synchronous": If true, the call will block until the operation is
  completed. If false, the call will return immediately and the operation
  will run asynchronously in another thread.                 
- "force": Specifies how to handle entries for which a BEOS:TYPE attribute
  already exists. Valid values are 
  B_UPDATE_MIME_INFO_{NO_FORCE, FORCE_KEEP_TYPE, FORCE_UPDATE_ALL}.

reply fields:

- "result":

  - B_OK: The asynchronous update_mime_info() call has been successfully
          started (and may still be running).
  - (error code): failure

perform a create_app_meta_mime() call
.....................................

+--------------+-------------------------------------------------------+
| target       | mime manager (BRoster::fMimeMess)                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_MIME_CREATE_APP_META_MIME                       |
|              |                                                       |
|	       | - "entry": B_REF_TYPE                                 |
|	       | - "recursive": B_BOOLEAN_TYPE                         |
|	       | - "synchronous": B_BOOLEAN_TYPE                       |
|	       | - "force": B_INT32_TYPE                               |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "entry": The base entry to update.
- "recursive": If true and "entry" is a directory, update all entries
  below "entry" in the hierarchy.
- "synchronous": If true, the call will block until the operation is
  completed. If false, the call will return immediately and the operation
  will run asynchronously in another thread.                 
- "force": If != 0, also update entries for which meta app information
  already exists.

reply fields:

- "result":

  - B_OK: The asynchronous update_mime_info() call has been successfully
          started (and may still be running).
  - (error code): failure

notify the thread manager to perform a clean up run
...................................................

+--------------+-----------------------------------------------------------------------------+
| target       | thread manager/mime manager (MIMEManager::fThreadManager/BRoster::fMimeMess)|
+--------------+-----------------------------------------------------------------------------+
| message      | B_REG_MIME_UPDATE_THREAD_FINISHED                                           |
+--------------+-----------------------------------------------------------------------------+
| reply        | none (message should be sent asynchronously)                                |
+--------------+-----------------------------------------------------------------------------+

Message Runner Requests
-----------------------

message runner registration (BMessageRunner::InitData())
........................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_REGISTER_MESSAGE_RUNNER                         |
|              |                                                       |
|              | - "team":			   B_INT32_TYPE        |
|              | - "target":			B_MESSENGER_TYPE       |
|              | - "message":				B_MESSAGE_TYPE |
|              | - "interval":				B_INT64_TYPE   |
|              | - "count":				B_INT32_TYPE   |
|              | - "reply_target":		   B_MESSENGER_TYPE    |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - "token":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+
| on error:    | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "team": ID of the team owning the BMessageRunner (team_id).
- "target": The message target.
- "message": The message to be sent to the target.
- "interval": Period of time before the first message is sent and between
  messages (if more than one shall be sent) in microseconds.
- "count": Specifies how many times the message shall be sent.
  A value less than 0 for an unlimited number of repetitions.
- "reply_target": Target replies to the delivered message(s) shall be sent to.

reply fields:

- "token": Unique token identifying the message runner.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

message runner unregistration (BMessageRunner::~BMessageRunner())
.................................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message:     | B_REG_UNREGISTER_MESSAGE_RUNNER                       |
|              |                                                       |
|              | - "token":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error:    | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "token": Unique token identifying the message runner. Returned by the
  B_REG_REGISTER_MESSAGE_RUNNER request.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

set message runner parameters (BMessageRunner::SetParams())
...........................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message      | B_REG_SET_MESSAGE_RUNNER_PARAMS                       |
|              |                                                       |
|              | - "token":				B_INT32_TYPE   |
|              | - [ "interval":			B_INT64_TYPE ] |
|              | - [ "count":				B_INT32_TYPE ] |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
+--------------+-------------------------------------------------------+
| on error:    | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "token": Unique token identifying the message runner. Returned by the
  B_REG_REGISTER_MESSAGE_RUNNER request.
- "interval": Period of time before the first message is sent and between
  messages (if more than one shall be sent) in microseconds.
- "count": Specifies how many times the message shall be sent.
  A value less than 0 for an unlimited number of repetitions.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

get message runner info (BMessageRunner::InitData())
....................................................

+--------------+-------------------------------------------------------+
| target       | roster                                                |
+--------------+-------------------------------------------------------+
| message:     | B_REG_GET_MESSAGE_RUNNER_INFO                         |
|              |                                                       |
|              | - "token":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+
| reply        | standard success                                      |
|              |                                                       |
|              | - "interval":				B_INT64_TYPE   |
|              | - "count":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+
| on error:    | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "token": Unique token identifying the message runner. Returned by the
  B_REG_REGISTER_MESSAGE_RUNNER request.

reply fields:

- "interval": Period of time before the first message is sent and between
  messages (if more than one shall be sent) in microseconds.
- "count": Specifies how many times the message still has to be sent.
  A value less than 0 for an unlimited number of repetitions.

error reply fields:

- "error":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

Clipboard Handler Requests
--------------------------

add new clipboard to system (BClipboard::BClipboard())
......................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_ADD_CLIPBOARD                                   |
|              |                                                       |
|              | - "name":			      B_STRING_TYPE    |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard to add

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: name field was not specified in message

get clipboard write count (BClipboard::GetSystemCount())
........................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_GET_CLIPBOARD_COUNT                             |
|              |                                                       |
|              | - "name":				 B_STRING_TYPE |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "count":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: no name / no valid name specified in message

- "count":

  - number of times this clipboard has been written to

start watching clipboard (BClipboard::StartWatching())
......................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_CLIPBOARD_START_WATCHING                        |
|              |                                                       |
|              | - "name":				 B_STRING_TYPE |
|              | - "target":			      B_MESSENGER_TYPE |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard
- "target": Messenger pointing to the target to notify

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: no name / no valid name specified in message
  		 no target specified

stop watching clipboard (BClipboard::StopWatching())
....................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_CLIPBOARD_STOP_WATCHING                         |
|              |                                                       |
|              | - "name":				 B_STRING_TYPE |
|              | - "target":			      B_MESSENGER_TYPE |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard
- "target": Messenger pointing to the target to remove from the notify list

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: no name / no valid name specified in message / no target specified

download clipboard data (BClipboard::DownloadFromSystem())
..........................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_DOWNLOAD_CLIPBOARD                              |
|              |                                                       |
|              | "name":				B_STRING_TYPE  |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "data":			      B_MESSAGE_TYPE   |
|              | - "data source":		      B_MESSENGER_TYPE |
|              | - "count":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: no name / no valid name specified in message
    no target specified

- "data": message with Data fields containing the contents of the clipboard
- "data source": messenger to the be_app_messenger of the application which last wrote data
- "count": number of times this clipboard has been written to

upload clipboard data (BClipboard::UploadToSystem())
....................................................

+--------------+-------------------------------------------------------+
| target       | clipboard handler                                     |
+--------------+-------------------------------------------------------+
| message      | B_REG_UPLOAD_CLIPBOARD                                |
|              |                                                       |
|	       | "name":			   B_STRING_TYPE       |
|	       | "data":			   B_MESSAGE_TYPE      |
|	       | "data source":			   B_MESSENGER_TYPE    |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "count":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+

message fields:

- "name": Name used to identify the particular clipboard
- "data": message with Data fields containing the contents of the clipboard
- "data source": messenger to the be_app_messenger of the application which last wrote data

reply fields:

- "result":

  - B_OK: success
  - B_BAD_VALUE: no name / no valid name specified in message / no target specified

- "count":

  - number of times this clipboard has been written to

Disk Device Requests
--------------------

get next disk device
....................

+--------------+-------------------------------------------------------+
| target       | disk device manager                                   |
+--------------+-------------------------------------------------------+
| message      | B_REG_NEXT_DISK_DEVICE                                |
|              |                                                       |
|              | - "cookie":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "device":				B_MESSAGE_TYPE |
|              | - "cookie":				B_INT32_TYPE   |
+--------------+-------------------------------------------------------+

message fields:

- "cookie": An iteration cookie. Initially 0.

reply fields:

- "device": Archived BDiskDevice info.
- "cookie": Next value for the iteration cookie.
- "result":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_ENTRY_NOT_FOUND: Iteration finished.
  - ...

get disk device
...............

+--------------+-------------------------------------------------------+
| target       | disk device manager                                   |
+--------------+-------------------------------------------------------+
| message      | B_REG_GET_DISK_DEVICE                                 |
|              |                                                       |
|              | - "device_id":			B_INT32_TYPE           |
|              | - | "session_id":			B_INT32_TYPE   |
|              | - | "partition_id":		B_INT32_TYPE           |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "device":				B_MESSAGE_TYPE |
+--------------+-------------------------------------------------------+

message fields:

- "device_id": ID of the device to be retrieved.
- "session_id": ID of session whose device shall be retrieved.
- "partition_id": ID of partition whose device shall be retrieved.

reply fields:

- "device": Archived BDiskDevice info.
- "result":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_ENTRY_NOT_FOUND: A device/session/partition with that ID could not
  	be found.
  - ...

update disk device
..................

+--------------+-------------------------------------------------------+
| target       | disk device manager                                   |
+--------------+-------------------------------------------------------+
| message      | B_REG_UPDATE_DISK_DEVICE                              |
|              |                                                       |
|              | - "device_id":			B_INT32_TYPE           |
|              | - | "session_id":			B_INT32_TYPE   |
|              | - | "partition_id":		B_INT32_TYPE           |
|              | - "change_counter":		B_INT32_TYPE           |
|              | - "update_policy":		B_INT32_TYPE           |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
|              |                                                       |
|              | - "up_to_date":			B_BOOLEAN_TYPE |
|              | - [ "device":			B_MESSAGE_TYPE ]       |
+--------------+-------------------------------------------------------+

message fields:

- "device_id": ID of the device to be retrieved.
- "session_id": ID of session whose device shall be retrieved.
- "partition_id": ID of partition whose device shall be retrieved.
- "change_counter": Change counter of the object (or device in case of
  B_REG_DEVICE_UPDATE_DEVICE_CHANGED update policy) in question.
- "update_policy": (uint32)

  - B_REG_DEVICE_UPDATE_CHECK: Check only, if the object is up to date.
  - B_REG_DEVICE_UPDATE_CHANGED: Update only, if the object has changed.
  - B_REG_DEVICE_UPDATE_DEVICE_CHANGED: Update, if the device has changed, even
    if the partition/session has not.

  The latter two have the same semantics, if the object is a device.

reply fields:

- "up_to_date": true, if the object (and for
  B_REG_DEVICE_UPDATE_DEVICE_CHANGED also the device) is already up to date,
  false otherwise.
- "device": Archived BDiskDevice info.
- "result":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - B_ENTRY_NOT_FOUND: A device/session/partition with that ID could not
    be found.
  - ...

start disk device watching
..........................

+--------------+-------------------------------------------------------+
| target       | disk device manager                                   |
+--------------+-------------------------------------------------------+
| message      | B_REG_DEVICE_START_WATCHING                           |
|              |                                                       |
|              | - "target":	B_MESSENGER_TYPE                       |
|              | - "events":	B_INT32_TYPE                           |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "target": The target the event messages shall be sent to.
- "events": Specifies the events the caller is interested in (uint32).
- "result":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...

stop disk device watching
.........................

+--------------+-------------------------------------------------------+
| target       | disk device manager                                   |
+--------------+-------------------------------------------------------+
| message      | B_REG_DEVICE_STOP_WATCHING                            |
|              |                                                       |
|              | - "target":	B_MESSENGER_TYPE                       |
+--------------+-------------------------------------------------------+
| reply        | standard general result                               |
+--------------+-------------------------------------------------------+
| on error     | - B_NO_REPLY (fatal)                                  |
|              | - standard error                                      |
+--------------+-------------------------------------------------------+

message fields:

- "target": The target that shall not longer receive any event messages.
- "result":

  - B_BAD_VALUE: A request message field is missing or contains an
    invalid value.
  - ...
