/*
 * Copyright 2001-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@@users.sf.net
 */


#include <AppMisc.h>

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <Entry.h>
#include <image.h>
#include <OS.h>

#include <ServerLink.h>
#include <ServerProtocol.h>


namespace BPrivate {


/*!	\brief Returns the path to an application's executable.
	\param team The application's team ID.
	\param buffer A pointer to a pre-allocated character array of at least
		   size B_PATH_NAME_LENGTH to be filled in by this function.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a buffer.
	- another error code
*/
status_t
get_app_path(team_id team, char *buffer)
{
	// The only way to get the path to the application's executable seems to
	// be to get an image_info of its image, which also contains a path.
	// Several images may belong to the team (libraries, add-ons), but only
	// the one in question should be typed B_APP_IMAGE.
	if (!buffer)
		return B_BAD_VALUE;

	image_info info;
	int32 cookie = 0;

	while (get_next_image_info(team, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE) {
			strlcpy(buffer, info.name, B_PATH_NAME_LENGTH - 1);
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


/*!	\brief Returns the path to the application's executable.
	\param buffer A pointer to a pre-allocated character array of at least
		   size B_PATH_NAME_LENGTH to be filled in by this function.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a buffer.
	- another error code
*/
status_t
get_app_path(char *buffer)
{
	return get_app_path(B_CURRENT_TEAM, buffer);
}


/*!	\brief Returns an entry_ref referring to an application's executable.
	\param team The application's team ID.
	\param ref A pointer to a pre-allocated entry_ref to be initialized
		   to an entry_ref referring to the application's executable.
	\param traverse If \c true, the function traverses symbolic links.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- another error code
*/
status_t
get_app_ref(team_id team, entry_ref *ref, bool traverse)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	char appFilePath[B_PATH_NAME_LENGTH];

	if (error == B_OK)
		error = get_app_path(team, appFilePath);

	if (error == B_OK) {
		BEntry entry(appFilePath, traverse);
		error = entry.GetRef(ref);
	}

	return error;
}


/*!	\brief Returns an entry_ref referring to the application's executable.
	\param ref A pointer to a pre-allocated entry_ref to be initialized
		   to an entry_ref referring to the application's executable.
	\param traverse If \c true, the function traverses symbolic links.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- another error code
*/
status_t
get_app_ref(entry_ref *ref, bool traverse)
{
	return get_app_ref(B_CURRENT_TEAM, ref, traverse);
}


/*!	\brief Returns the ID of the current team.
	\return The ID of the current team.
*/
team_id
current_team()
{
	static team_id team = -1;
	if (team < 0) {
		thread_info info;
		if (get_thread_info(find_thread(NULL), &info) == B_OK)
			team = info.team;
	}
	return team;
}


/*!	Returns the ID of the supplied team's main thread.
	\param team The team.
	\return
	- The thread ID of the supplied team's main thread
	- \c B_BAD_TEAM_ID: The supplied team ID does not identify a running team.
	- another error code
*/
thread_id
main_thread_for(team_id team)
{
	// Under Haiku the team ID is equal to it's main thread ID. We just get
	// a team info to verify the existence of the team.
	team_info info;
	status_t error = get_team_info(team, &info);
	return (error == B_OK ? team : error);
}


/*!	\brief Returns whether the application identified by the supplied
		   \c team_id is currently showing a modal window.
	\param team the ID of the application in question.
	\return \c true, if the application is showing a modal window, \c false
			otherwise.
*/
bool
is_app_showing_modal_window(team_id team)
{
	// TODO: Implement!
	return true;
}


port_id
get_app_server_port()
{
	static port_id sServerPort = -1;

	if (sServerPort < 0) {
		// No need for synchronization - in the worst case, we'll call
		// find_port() twice.
		sServerPort = find_port(SERVER_PORT_NAME);
	}

	return sServerPort;
}


/*!	Creates a connection with the desktop.
*/
status_t
create_desktop_connection(ServerLink* link, const char* name, int32 capacity)
{
	port_id serverPort = get_app_server_port();
	if (serverPort < 0)
		return serverPort;

	// Create the port so that the app_server knows where to send messages
	port_id clientPort = create_port(capacity, name);
	if (clientPort < 0)
		return clientPort;

	link->SetTo(serverPort, clientPort);

	link->StartMessage(AS_GET_DESKTOP);
	link->Attach<port_id>(clientPort);
	link->Attach<int32>(getuid());
	link->AttachString(getenv("TARGET_SCREEN"));
	link->Attach<int32>(AS_PROTOCOL_VERSION);

	int32 code;
	if (link->FlushWithReply(code) != B_OK || code != B_OK) {
		link->SetSenderPort(-1);
		return B_ERROR;
	}

	link->Read<port_id>(&serverPort);
	link->SetSenderPort(serverPort);

	return B_OK;
}


} // namespace BPrivate
