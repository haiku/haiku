/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "LocalTargetHostInterface.h"

#include <set>

#include <stdio.h>
#include <unistd.h>

#include <image.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <system_info.h>
#include <util/KMessage.h>

#include "debug_utils.h"

#include "CoreFile.h"
#include "CoreFileDebuggerInterface.h"
#include "LocalDebuggerInterface.h"
#include "TargetHost.h"

using std::set;

LocalTargetHostInterface::LocalTargetHostInterface()
	:
	TargetHostInterface(),
	fTargetHost(NULL),
	fDataPort(-1)
{
	SetName("Local");
}


LocalTargetHostInterface::~LocalTargetHostInterface()
{
	Close();

	if (fTargetHost != NULL)
		fTargetHost->ReleaseReference();
}


status_t
LocalTargetHostInterface::Init(Settings* settings)
{
	char hostname[HOST_NAME_MAX + 1];
	status_t error = gethostname(hostname, sizeof(hostname));
	if (error != B_OK) {
		fprintf(stderr, "gethostname() failed, defaults to localhost\n");
		strlcpy(hostname, "localhost", sizeof(hostname));
	}

	fTargetHost = new(std::nothrow) TargetHost(hostname);
	if (fTargetHost == NULL)
		return B_NO_MEMORY;

	team_info info;
	error = get_team_info(B_CURRENT_TEAM, &info);
	if (error != B_OK)
		return error;

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "LocalTargetHostInterface %" B_PRId32,
		info.team);

	fDataPort = create_port(100, buffer);
	if (fDataPort < 0)
		return fDataPort;

	fPortWorker = spawn_thread(_PortLoop, "Local Target Host Loop",
		B_NORMAL_PRIORITY, this);
	if (fPortWorker < 0)
		return fPortWorker;

	resume_thread(fPortWorker);

	AutoLocker<TargetHost> hostLocker(fTargetHost);

	error = __start_watching_system(-1,
		B_WATCH_SYSTEM_TEAM_CREATION | B_WATCH_SYSTEM_TEAM_DELETION,
		fDataPort, 0);
	if (error != B_OK)
		return error;

	int32 cookie = 0;
	while (get_next_team_info(&cookie, &info) == B_OK) {
		error = fTargetHost->AddTeam(info);
		if (error != B_OK)
			return error;
	}

	snprintf(buffer, sizeof(buffer), "Local (%s)", hostname);
	SetName(buffer);

	return B_OK;
}


void
LocalTargetHostInterface::Close()
{
	if (fDataPort > 0) {
		__stop_watching_system(-1,
			B_WATCH_SYSTEM_TEAM_CREATION | B_WATCH_SYSTEM_TEAM_DELETION,
			fDataPort, 0);

		delete_port(fDataPort);
		fDataPort = -1;
	}

	if (fPortWorker > 0) {
		wait_for_thread(fPortWorker, NULL);
		fPortWorker = -1;
	}
}


bool
LocalTargetHostInterface::IsLocal() const
{
	return true;
}


bool
LocalTargetHostInterface::Connected() const
{
	return true;
}


TargetHost*
LocalTargetHostInterface::GetTargetHost()
{
	return fTargetHost;
}


status_t
LocalTargetHostInterface::Attach(team_id teamID, thread_id threadID,
	DebuggerInterface*& _interface) const
{
	if (teamID < 0 && threadID < 0)
		return B_BAD_VALUE;

	status_t error;
	if (teamID < 0) {
		thread_info threadInfo;
		error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK)
			return error;

		teamID = threadInfo.team;
	}

	LocalDebuggerInterface* interface
		= new(std::nothrow) LocalDebuggerInterface(teamID);
	if (interface == NULL)
		return B_NO_MEMORY;

	BReference<DebuggerInterface> interfaceReference(interface, true);
	error = interface->Init();
	if (error != B_OK)
		return error;

	_interface = interface;
	interfaceReference.Detach();
	return B_OK;
}


status_t
LocalTargetHostInterface::CreateTeam(int commandLineArgc,
	const char* const* arguments, team_id& _teamID) const
{
	thread_id thread = load_program(arguments, commandLineArgc, false);
	if (thread < 0)
		return thread;

	// main thread ID == team ID.
	_teamID = thread;
	return B_OK;
}


status_t
LocalTargetHostInterface::LoadCore(const char* coreFilePath,
	DebuggerInterface*& _interface, thread_id& _thread) const
{
	// load the core file
	CoreFile* coreFile = new(std::nothrow) CoreFile;
	if (coreFile == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreFile> coreFileDeleter(coreFile);

	status_t error = coreFile->Init(coreFilePath);
	if (error != B_OK)
		return error;

	// create the debugger interface
	CoreFileDebuggerInterface* interface
		= new(std::nothrow) CoreFileDebuggerInterface(coreFile);
	if (interface == NULL)
		return B_NO_MEMORY;
	coreFileDeleter.Detach();

	BReference<DebuggerInterface> interfaceReference(interface, true);
	error = interface->Init();
	if (error != B_OK)
		return error;

	const CoreFileTeamInfo& teamInfo = coreFile->GetTeamInfo();
	_thread = teamInfo.Id();
	_interface = interface;
	interfaceReference.Detach();

	return B_OK;
}


status_t
LocalTargetHostInterface::FindTeamByThread(thread_id thread,
	team_id& _teamID) const
{
	thread_info info;
	status_t error = get_thread_info(thread, &info);
	if (error != B_OK)
		return error;

	_teamID = info.team;
	return B_OK;
}


status_t
LocalTargetHostInterface::_PortLoop(void* arg)
{
	LocalTargetHostInterface* interface = (LocalTargetHostInterface*)arg;
	set<team_id> waitingTeams;

	for (;;) {
		status_t error;
		bool addToWaiters;
		char buffer[2048];
		int32 messageCode;
		team_id team;

		ssize_t size = read_port_etc(interface->fDataPort, &messageCode,
			buffer, sizeof(buffer), B_TIMEOUT, waitingTeams.empty()
				? B_INFINITE_TIMEOUT : 20000);
		if (size == B_INTERRUPTED)
			continue;
		else if (size == B_TIMED_OUT && !waitingTeams.empty()) {
			for (set<team_id>::iterator it = waitingTeams.begin();
				it != waitingTeams.end(); ++it) {
				team = *it;
				error = interface->_HandleTeamEvent(team,
					B_TEAM_CREATED, addToWaiters);
				if (error != B_OK)
					continue;
				else if (!addToWaiters) {
					waitingTeams.erase(it);
					if (waitingTeams.empty())
						break;
					it = waitingTeams.begin();
				}
			}
			continue;
		} else if (size < 0)
			return size;

		KMessage message;
		size = message.SetTo(buffer);
		if (size != B_OK)
			continue;

		if (message.What() != B_SYSTEM_OBJECT_UPDATE)
			continue;

		int32 opcode = 0;
		if (message.FindInt32("opcode", &opcode) != B_OK)
			continue;

		team = -1;
		if (message.FindInt32("team", &team) != B_OK)
			continue;

		error = interface->_HandleTeamEvent(team, opcode,
			addToWaiters);
		if (error != B_OK)
			continue;
		if (opcode == B_TEAM_CREATED && addToWaiters) {
			try {
				waitingTeams.insert(team);
			} catch (...) {
				continue;
			}
		}
	}

	return B_OK;
}


status_t
LocalTargetHostInterface::_HandleTeamEvent(team_id team, int32 opcode,
	bool& addToWaiters)
{
	addToWaiters = false;
	AutoLocker<TargetHost> locker(fTargetHost);
	switch (opcode) {
		case B_TEAM_CREATED:
		case B_TEAM_EXEC:
		{
			team_info info;
			status_t error = get_team_info(team, &info);
			// this team is already gone, no point in sending a notification
			if (error == B_BAD_TEAM_ID)
				return B_OK;
			else if (error != B_OK)
				return error;
			else {
				int32 cookie = 0;
				image_info imageInfo;
				addToWaiters = true;
				while (get_next_image_info(team, &cookie, &imageInfo)
					== B_OK) {
					if (imageInfo.type == B_APP_IMAGE) {
						addToWaiters = false;
						break;
					}
				}
				if (addToWaiters)
					return B_OK;
			}

			if (opcode == B_TEAM_CREATED)
				fTargetHost->AddTeam(info);
			else
				fTargetHost->UpdateTeam(info);
			break;
		}

		case B_TEAM_DELETED:
		{
			fTargetHost->RemoveTeam(team);
			break;
		}

		default:
		{
			break;
		}
	}

	return B_OK;
}
