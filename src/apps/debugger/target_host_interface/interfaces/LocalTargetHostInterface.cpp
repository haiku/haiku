/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "LocalTargetHostInterface.h"

#include <stdio.h>
#include <unistd.h>

#include <AutoLocker.h>
#include <system_info.h>
#include <util/KMessage.h>

#include "TargetHost.h"


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
LocalTargetHostInterface::Init()
{
	char hostname[HOST_NAME_MAX + 1];
	status_t error = gethostname(hostname, sizeof(hostname));
	if (error != B_OK)
		return error;

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

	fPortWorker = spawn_thread(PortLoop, "Local Target Host Loop",
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
LocalTargetHostInterface::Attach(team_id id, thread_id threadID,
	DebuggerInterface*& _interface)
{
	return B_NOT_SUPPORTED;
}


status_t
LocalTargetHostInterface::CreateTeam(int commandLineArgc,
	const char* const* arguments, DebuggerInterface*& _interface)
{
	return B_NOT_SUPPORTED;
}


status_t
LocalTargetHostInterface::PortLoop(void* arg)
{
	LocalTargetHostInterface* interface = (LocalTargetHostInterface*)arg;

	for (;;) {
		char buffer[2048];
		int32 messageCode;
		ssize_t size = read_port(interface->fDataPort, &messageCode, buffer,
			sizeof(buffer));
		if (size == B_INTERRUPTED)
			continue;
		else if (size < 0)
			return size;

		KMessage message;
		size = message.SetTo(buffer);
		if (size != B_OK)
			return size;

		if (message.What() != B_SYSTEM_OBJECT_UPDATE)
			continue;

		int32 opcode = 0;
		if (message.FindInt32("opcode", &opcode) != B_OK)
			continue;

		team_id team = -1;
		if (message.FindInt32("team", &team) != B_OK)
			continue;

		AutoLocker<TargetHost> locker(interface->fTargetHost);
		switch (opcode) {
			case B_TEAM_CREATED:
			case B_TEAM_EXEC:
			{
				team_info info;
				status_t error = get_team_info(team, &info);
				if (error != B_OK)
					continue;

				if (opcode == B_TEAM_CREATED)
					interface->fTargetHost->AddTeam(info);
				else
					interface->fTargetHost->UpdateTeam(info);
				break;
			}

			case B_TEAM_DELETED:
			{
				interface->fTargetHost->RemoveTeam(team);
				break;
			}

			default:
			{
				break;
			}
		}
	}

	return B_OK;
}
