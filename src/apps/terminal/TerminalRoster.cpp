/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TerminalRoster.h"

#include <stdio.h>

#include <new>

#include <Looper.h>
#include <Roster.h>
#include <String.h>

#include <AutoLocker.h>

#include "TermConst.h"


static const bigtime_t kAppsRunningCheckInterval = 1000000;


// #pragma mark - Info


/*!	Creates an Info with the given \a id and \a team ID.
	\c workspaces is set to 0 and \c minimized to \c true.
*/
TerminalRoster::Info::Info(int32 id, team_id team)
	:
	id(id),
	team(team),
	workspaces(0),
	minimized(true)
{
}


/*!	Create an Info and initializes its data from \a archive.
*/
TerminalRoster::Info::Info(const BMessage& archive)
{
	if (archive.FindInt32("id", &id) != B_OK)
		id = -1;
	if (archive.FindInt32("team", &team) != B_OK)
		team = -1;
	if (archive.FindUInt32("workspaces", &workspaces) != B_OK)
		workspaces = 0;
	if (archive.FindBool("minimized", &minimized) != B_OK)
		minimized = true;
}


/*!	Writes the Info's data into fields of \a archive.
	The const BMessage& constructor can restore an identical Info from it.
*/
status_t
TerminalRoster::Info::Archive(BMessage& archive) const
{
	status_t error;
	if ((error = archive.AddInt32("id", id)) != B_OK
		|| (error = archive.AddInt32("team", team)) != B_OK
		|| (error = archive.AddUInt32("workspaces", workspaces)) != B_OK
		|| (error = archive.AddBool("minimized", minimized)) != B_OK) {
		return error;
	}

	return B_OK;
}


/*!	Compares two Infos.
	Infos are considered equal, iff all data members are.
*/
bool
TerminalRoster::Info::operator==(const Info& other) const
{
	return id == other.id && team == other.team
		&& workspaces == other.workspaces && minimized == other.minimized;
}


// #pragma mark - TerminalRoster


/*!	Creates a TerminalRoster.
	Most methods cannot be used until Register() has been invoked.
*/
TerminalRoster::TerminalRoster()
	:
	BHandler("terminal roster"),
	fLock("terminal roster"),
	fClipboard(TERM_SIGNATURE),
	fInfos(10, true),
	fOurInfo(NULL),
	fLastCheckedTime(0),
	fListener(NULL),
	fInfosUpdated(false)
{
}


/*!	Locks the object.
	Also makes sure the roster list is reasonably up-to-date.
*/
bool
TerminalRoster::Lock()
{
	// lock
	bool locked = fLock.Lock();
	if (!locked)
		return false;

	// make sure we're registered
	if (fOurInfo == NULL) {
		fLock.Unlock();
		return false;
	}

	// If the check interval has passed, make sure all infos still have running
	// teams.
	bigtime_t now = system_time();
	if (fLastCheckedTime + kAppsRunningCheckInterval) {
		bool needsUpdate = false;
		for (int32 i = 0; const Info* info = TerminalAt(i); i++) {
			if (!_TeamIsRunning(info->team)) {
				needsUpdate = true;
				break;
			}
		}

		if (needsUpdate) {
			AutoLocker<BClipboard> clipboardLocker(fClipboard);
			if (clipboardLocker.IsLocked()) {
				if (_UpdateInfos(true) == B_OK)
					_UpdateClipboard();
			}
		} else
			fLastCheckedTime = now;
	}

	return true;
}


/*!	Unlocks the object.
	As a side effect the listener will be notified, if the terminal list has
	changed in any way.
*/
void
TerminalRoster::Unlock()
{
	if (fOurInfo != NULL && fInfosUpdated) {
		// the infos have changed -- notify our listener
		_NotifyListener();
	}

	fLock.Unlock();
}


/*!	Registers a terminal with the roster and establishes a link.

	The object attaches itself to the supplied \a looper and will receive
	updates via messaging (obviously the looper must run (not necessarily
	right now) for this to work).

	\param teamID The team ID of this team.
	\param looper A looper the object can attach itself to.
	\return \c B_OK, if successful, another error code otherwise.
*/
status_t
TerminalRoster::Register(team_id teamID, BLooper* looper)
{
	AutoLocker<BLocker> locker(fLock);

	if (fOurInfo != NULL) {
		// already registered
		return B_BAD_VALUE;
	}

	// lock the clipboard
	AutoLocker<BClipboard> clipboardLocker(fClipboard);
	if (!clipboardLocker.IsLocked())
		return B_BAD_VALUE;

	// get the current infos from the clipboard
	status_t error = _UpdateInfos(true);
	if (error != B_OK)
		return error;

	// find an unused ID
	int32 id = 0;
	for (int32 i = 0; const Info* info = TerminalAt(i); i++) {
		if (info->id > id)
			break;
		id++;
	}

	// create our own info
	fOurInfo = new(std::nothrow) Info(id, teamID);
	if (fOurInfo == NULL)
		return B_NO_MEMORY;

	// insert it
	if (!fInfos.BinaryInsert(fOurInfo, &_CompareInfos)) {
		delete fOurInfo;
		fOurInfo = NULL;
		return B_NO_MEMORY;
	}

	// update the clipboard
	error = _UpdateClipboard();
	if (error != B_OK) {
		fInfos.MakeEmpty(true);
		fOurInfo = NULL;
		return error;
	}

	// add ourselves to the looper and start watching
	looper->AddHandler(this);

	be_roster->StartWatching(this, B_REQUEST_QUIT);
	fClipboard.StartWatching(this);

	// Update again in case we've missed a update message sent before we were
	// listening.
	_UpdateInfos(false);

	return B_OK;
}


/*!	Unregisters the terminal from the roster and closes the link.

	Basically undoes all effects of Register().
*/
void
TerminalRoster::Unregister()
{
	AutoLocker<BLocker> locker(fLock);
	if (!locker.IsLocked())
		return;

	// stop watching and remove ourselves from the looper
	be_roster->StartWatching(this);
	fClipboard.StartWatching(this);

	Looper()->RemoveHandler(this);

	// lock the clipboard and get the current infos
	AutoLocker<BClipboard> clipboardLocker(fClipboard);
	if (!clipboardLocker.IsLocked() || _UpdateInfos(false) != B_OK)
		return;

	// remove our info and update the clipboard
	fInfos.RemoveItem(fOurInfo);
	fOurInfo = NULL;

	_UpdateClipboard();
}


/*!	Returns the ID assigned to this terminal when it was registered.
*/
int32
TerminalRoster::ID() const
{
	return fOurInfo != NULL ? fOurInfo->id : -1;
}


/*!	Updates this terminal's window status.
	All other running terminals will be notified, if the status changed.

	\param minimized \c true, if the window is minimized.
	\param workspaces The window's workspaces mask.
*/
void
TerminalRoster::SetWindowInfo(bool minimized, uint32 workspaces)
{
	AutoLocker<TerminalRoster> locker(this);
	if (!locker.IsLocked())
		return;

	if (minimized == fOurInfo->minimized && workspaces == fOurInfo->workspaces)
		return;

	fOurInfo->minimized = minimized;
	fOurInfo->workspaces = workspaces;
	fInfosUpdated = true;

	// lock the clipboard and get the current infos
	AutoLocker<BClipboard> clipboardLocker(fClipboard);
	if (!clipboardLocker.IsLocked() || _UpdateInfos(false) != B_OK)
		return;

	// update the clipboard to make our change known to the others
	_UpdateClipboard();
}


/*!	Overriden to handle update messages.
*/
void
TerminalRoster::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SOME_APP_QUIT:
		{
			BString signature;
			if (message->FindString("be:signature", &signature) != B_OK
				|| signature != TERM_SIGNATURE) {
				break;
			}
			// fall through
		}

		case B_CLIPBOARD_CHANGED:
		{
			// lock ourselves and the clipboard and update the infos
			AutoLocker<TerminalRoster> locker(this);
			AutoLocker<BClipboard> clipboardLocker(fClipboard);
			if (clipboardLocker.IsLocked()) {
				_UpdateInfos(false);

				if (fInfosUpdated)
					_NotifyListener();
			}

			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


/*!	Updates the terminal info list from the clipboard.

	\param checkApps If \c true, it is checked for each found info whether the
		respective team is still running. If not, the info is removed from the
		list (though not from the clipboard).
	\return \c B_OK, if the update went fine, another error code otherwise. When
		an error occurs the object state will still be consistent, but might no
		longer be up-to-date.
*/
status_t
TerminalRoster::_UpdateInfos(bool checkApps)
{
	BMessage* data = fClipboard.Data();

	// find out how many infos we can expect
	type_code type;
	int32 count;
	status_t error = data->GetInfo("teams", &type, &count);
	if (error != B_OK)
		count = 0;

	// create an info list from the message
	InfoList infos(10, true);
	for (int32 i = 0; i < count; i++) {
		// get the team's message
		BMessage teamData;
		error = data->FindMessage("teams", i, &teamData);
		if (error != B_OK)
			return error;

		// create the info
		Info* info = new(std::nothrow) Info(teamData);
		if (info == NULL)
			return B_NO_MEMORY;
		if (info->id < 0 || info->team < 0
			|| infos.BinarySearchByKey(info->id, &_CompareIDInfo) != NULL
			|| (checkApps && !_TeamIsRunning(info->team))) {
			// invalid/duplicate info -- skip
			delete info;
			fInfosUpdated = true;
			continue;
		}

		// add it to the list
		if (!infos.BinaryInsert(info, &_CompareInfos)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	// update the current info list from the infos we just read
	int32 oldIndex = 0;
	int32 newIndex = 0;
	while (oldIndex < fInfos.CountItems() || newIndex < infos.CountItems()) {
		Info* oldInfo = fInfos.ItemAt(oldIndex);
		Info* newInfo = infos.ItemAt(newIndex);

		if (oldInfo == NULL || (newInfo != NULL && oldInfo->id > newInfo->id)) {
			// new info is not in old list -- transfer it
			if (!fInfos.AddItem(newInfo, oldIndex++))
				return B_NO_MEMORY;
			infos.RemoveItemAt(newIndex);
			fInfosUpdated = true;
		} else if (newInfo == NULL || oldInfo->id < newInfo->id) {
			// old info is not in new list -- delete it, unless it's our own
			if (oldInfo == fOurInfo) {
				oldIndex++;
			} else {
				delete fInfos.RemoveItemAt(oldIndex);
				fInfosUpdated = true;
			}
		} else {
			// info is in both lists -- update the old info, unless it's our own
			if (oldInfo != fOurInfo) {
				if (*oldInfo != *newInfo) {
					*oldInfo = *newInfo;
					fInfosUpdated = true;
				}
			}

			oldIndex++;
			newIndex++;
		}
	}

	if (checkApps)
		fLastCheckedTime = system_time();

	return B_OK;
}


/*!	Updates the clipboard with the object's terminal info list.

	\return \c B_OK, if the update went fine, another error code otherwise. When
		an error occurs the object state will still be consistent, but might no
		longer be in sync with the clipboard.
*/
status_t
TerminalRoster::_UpdateClipboard()
{
	// get the clipboard data message
	BMessage* data = fClipboard.Data();
	if (data == NULL)
		return B_BAD_VALUE;

	// clear the message and add all infos
	data->MakeEmpty();

	for (int32 i = 0; const Info* info = TerminalAt(i); i++) {
		BMessage teamData;
		status_t error = info->Archive(teamData);
		if (error != B_OK
			|| (error = data->AddMessage("teams", &teamData)) != B_OK) {
			fClipboard.Revert();
			return error;
		}
	}

	// commit the changes
	status_t error = fClipboard.Commit();
	if (error != B_OK) {
		fClipboard.Revert();
		return error;
	}

	return B_OK;
}


/*!	Notifies the listener, if something has changed.
*/
void
TerminalRoster::_NotifyListener()
{
	if (!fInfosUpdated)
		return;

	if (fListener != NULL)
		fListener->TerminalInfosUpdated(this);

	fInfosUpdated = false;
}


/*static*/ int
TerminalRoster::_CompareInfos(const Info* a, const Info* b)
{
	return a->id - b->id;
}


/*static*/ int
TerminalRoster::_CompareIDInfo(const int32* id, const Info* info)
{
	return *id - info->id;
}


bool
TerminalRoster::_TeamIsRunning(team_id teamID)
{
	// we are running for sure
	if (fOurInfo != NULL && fOurInfo->team == teamID)
		return true;

	team_info info;
	return get_team_info(teamID, &info) == B_OK;
}


// #pragma mark - Listener


TerminalRoster::Listener::~Listener()
{
}
