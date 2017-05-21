/*
 * Copyright 2016-2017, Rene Gollent, rene@gollent.com.
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "NetworkTargetHostInterface.h"

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <system_info.h>
#include <util/KMessage.h>

#include "debug_utils.h"

#include "TargetHost.h"


NetworkTargetHostInterface::NetworkTargetHostInterface()
	:
	TargetHostInterface(),
	fTargetHost(NULL)
{
	SetName("Network");
}


NetworkTargetHostInterface::~NetworkTargetHostInterface()
{
	Close();

	if (fTargetHost != NULL)
		fTargetHost->ReleaseReference();
}


status_t
NetworkTargetHostInterface::Init(Settings* settings)
{
	return B_NOT_SUPPORTED;
}


void
NetworkTargetHostInterface::Close()
{
}


bool
NetworkTargetHostInterface::IsLocal() const
{
	return false;
}


bool
NetworkTargetHostInterface::Connected() const
{
	return false;
}


TargetHost*
NetworkTargetHostInterface::GetTargetHost()
{
	return fTargetHost;
}


status_t
NetworkTargetHostInterface::Attach(team_id teamID, thread_id threadID,
	DebuggerInterface*& _interface) const
{
	return B_NOT_SUPPORTED;
}


status_t
NetworkTargetHostInterface::CreateTeam(int commandLineArgc,
	const char* const* arguments, team_id& _teamID) const
{
	return B_NOT_SUPPORTED;
}


status_t
NetworkTargetHostInterface::LoadCore(const char* coreFilePath,
	DebuggerInterface*& _interface, thread_id& _thread) const
{
	return B_NOT_SUPPORTED;
}


status_t
NetworkTargetHostInterface::FindTeamByThread(thread_id thread,
	team_id& _teamID) const
{
	return B_NOT_SUPPORTED;
}
