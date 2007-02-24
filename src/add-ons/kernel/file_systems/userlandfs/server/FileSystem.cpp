// FileSystem.cpp

#include <Application.h>
#include <Autolock.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>

#include "Debug.h"
#include "DispatcherDefs.h"
#include "FileSystem.h"
#include "ServerDefs.h"

// constructor
FileSystem::FileSystem(const char* name, status_t* _error)
	: LazyInitializable(),
	  Referencable(),
	  fName(),
	  fInfo(NULL),
	  fTeam(-1),
	  fFinishInitSemaphore(-1),
	  fTeamLock()
{
	status_t error = B_OK;
	if (!fName.SetTo(name))
		error = B_NO_MEMORY;
	if (_error)
		*_error = error;
}

// constructor
FileSystem::FileSystem(team_id team, FSInfo* info, status_t* _error)
	: LazyInitializable(false),
	  Referencable(),
	  fName(),
	  fInfo(info),
	  fTeam(team),
	  fFinishInitSemaphore(-1),
	  fTeamLock()
{
	status_t error = B_OK;
	if (!fName.SetTo(info->GetName()))
		error = B_NO_MEMORY;
	if (_error)
		*_error = error;
}

// destructor
FileSystem::~FileSystem()
{
	if (fFinishInitSemaphore >= 0)
		delete_sem(fFinishInitSemaphore);
	delete fInfo;
}

// GetName
const char*
FileSystem::GetName() const
{
	return fName.GetString();
}

// GetInfo
const FSInfo*
FileSystem::GetInfo() const
{
	return fInfo;
}

// GetTeam
team_id
FileSystem::GetTeam() const
{
	BAutolock _(fTeamLock);
	return fTeam;
}

// CompleteInit
void
FileSystem::CompleteInit(FSInfo* info)
{
	fInfo = info;
	if (fFinishInitSemaphore >= 0)
		release_sem(fFinishInitSemaphore);
}

// AbortInit
void
FileSystem::AbortInit()
{
	if (fInitStatus == B_OK)
		fInitStatus = B_NO_INIT;
	if (fFinishInitSemaphore >= 0)
		release_sem(fFinishInitSemaphore);
}

// FirstTimeInit
status_t
FileSystem::FirstTimeInit()
{
	if (fName.GetLength() == 0)
		RETURN_ERROR(B_BAD_VALUE);
	// create the init finish semaphore
	fFinishInitSemaphore = create_sem(0, "FS init finish sem");
	if (fFinishInitSemaphore < 0)
		return fFinishInitSemaphore;
	// get a server entry ref
	app_info appInfo;
	status_t error = be_app->GetAppInfo(&appInfo);
	if (error != B_OK)
		RETURN_ERROR(error);
	// launch a server instance
	team_id team = -1;
	fTeamLock.Lock();
	if (gServerSettings.ShallEnterDebugger()) {
		int argc = 2;
		const char *argv[] = { "--debug", fName.GetString(), NULL };
		error = be_roster->Launch(&appInfo.ref, argc, argv, &team);
	} else {
		int argc = 1;
		const char *argv[] = { fName.GetString(), NULL };
		error = be_roster->Launch(&appInfo.ref, argc, argv, &team);
	}
	fTeam = team;
	fTeamLock.Unlock();
	if (error != B_OK)
		RETURN_ERROR(error);
	// wait for the initialization to complete/fail
	error = _WaitForInitToFinish();
	RETURN_ERROR(error);
}

// _WaitForInitToFinish
status_t
FileSystem::_WaitForInitToFinish()
{
	status_t error = acquire_sem(fFinishInitSemaphore);
	delete_sem(fFinishInitSemaphore);
	fFinishInitSemaphore = -1;
	if (error != B_OK)
		return error;
	return (fInfo ? B_OK : B_ERROR);
}

