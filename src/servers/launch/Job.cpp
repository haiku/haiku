/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Job.h"

#include <Entry.h>
#include <Looper.h>
#include <Message.h>
#include <Roster.h>

#include "Target.h"


Job::Job(const char* name)
	:
	BJob(name),
	fEnabled(true),
	fService(false),
	fCreateDefaultPort(false),
	fLaunchInSafeMode(true),
	fInitStatus(B_NO_INIT),
	fTeam(-1),
	fTarget(NULL)
{
}


Job::Job(const Job& other)
	:
	BJob(other.Name()),
	fEnabled(other.IsEnabled()),
	fService(other.IsService()),
	fCreateDefaultPort(other.CreateDefaultPort()),
	fLaunchInSafeMode(other.LaunchInSafeMode()),
	fInitStatus(B_NO_INIT),
	fTeam(-1),
	fTarget(other.Target())
{
	for (int32 i = 0; i < other.Arguments().CountStrings(); i++)
		AddArgument(other.Arguments().StringAt(i));

	for (int32 i = 0; i < other.Requirements().CountStrings(); i++)
		AddRequirement(other.Requirements().StringAt(i));

	PortMap::const_iterator constIterator = other.Ports().begin();
	for (; constIterator != other.Ports().end(); constIterator++) {
		fPortMap.insert(
			std::make_pair(constIterator->first, constIterator->second));
	}

	PortMap::iterator iterator = fPortMap.begin();
	for (; iterator != fPortMap.end(); iterator++)
		iterator->second.RemoveData("port");
}


Job::~Job()
{
	_DeletePorts();
}


const char*
Job::Name() const
{
	return Title().String();
}


bool
Job::IsEnabled() const
{
	return fEnabled;
}


void
Job::SetEnabled(bool enable)
{
	fEnabled = enable;
}


bool
Job::IsService() const
{
	return fService;
}


void
Job::SetService(bool service)
{
	fService = service;
}


bool
Job::CreateDefaultPort() const
{
	return fCreateDefaultPort;
}


void
Job::SetCreateDefaultPort(bool createPort)
{
	fCreateDefaultPort = createPort;
}


void
Job::AddPort(BMessage& data)
{
	const char* name = data.GetString("name");
	fPortMap.insert(std::pair<BString, BMessage>(BString(name), data));
}


bool
Job::LaunchInSafeMode() const
{
	return fLaunchInSafeMode;
}


void
Job::SetLaunchInSafeMode(bool launch)
{
	fLaunchInSafeMode = launch;
}


const BStringList&
Job::Arguments() const
{
	return fArguments;
}


BStringList&
Job::Arguments()
{
	return fArguments;
}


void
Job::AddArgument(const char* argument)
{
	fArguments.Add(argument);
}


::Target*
Job::Target() const
{
	return fTarget;
}


void
Job::SetTarget(::Target* target)
{
	fTarget = target;
}


const BStringList&
Job::Requirements() const
{
	return fRequirements;
}


BStringList&
Job::Requirements()
{
	return fRequirements;
}


void
Job::AddRequirement(const char* requirement)
{
	fRequirements.Add(requirement);
}


status_t
Job::Init(const Finder& finder, std::set<BString>& dependencies)
{
	// Only initialize the jobs once
	if (fInitStatus != B_NO_INIT)
		return fInitStatus;

	fInitStatus = B_OK;

	if (fTarget != NULL)
		fTarget->AddDependency(this);

	// Check dependencies

	for (int32 index = 0; index < Requirements().CountStrings(); index++) {
		const BString& requires = Requirements().StringAt(index);
		if (dependencies.find(requires) != dependencies.end()) {
			// Found a cyclic dependency
			// TODO: log error
			return fInitStatus = B_ERROR;
		}
		dependencies.insert(requires);

		Job* dependency = finder.FindJob(requires);
		if (dependency != NULL) {
			std::set<BString> subDependencies = dependencies;

			fInitStatus = dependency->Init(finder, subDependencies);
			if (fInitStatus != B_OK) {
				// TODO: log error
				return fInitStatus;
			}

			fInitStatus = _AddRequirement(dependency);
		} else {
			::Target* target = finder.FindTarget(requires);
			if (target != NULL)
				fInitStatus = _AddRequirement(dependency);
			else {
				// Could not find dependency
				fInitStatus = B_NAME_NOT_FOUND;
			}
		}
		if (fInitStatus != B_OK) {
			// TODO: log error
			return fInitStatus;
		}
	}

	// Create ports
	// TODO: prefix system ports with "system:"

	bool defaultPort = false;

	for (PortMap::iterator iterator = fPortMap.begin();
			iterator != fPortMap.end(); iterator++) {
		BString name(Name());
		const char* suffix = iterator->second.GetString("name");
		if (suffix != NULL)
			name << ':' << suffix;
		else
			defaultPort = true;

		const int32 capacity = iterator->second.GetInt32("capacity",
			B_LOOPER_PORT_DEFAULT_CAPACITY);

		port_id port = create_port(capacity, name.String());
		if (port < 0) {
			fInitStatus = port;
			break;
		}
		iterator->second.SetInt32("port", port);
	}

	if (fInitStatus == B_OK && fCreateDefaultPort && !defaultPort) {
		BMessage data;
		data.AddInt32("capacity", B_LOOPER_PORT_DEFAULT_CAPACITY);

		port_id port = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY, Name());
		if (port < 0) {
			// TODO: log error
			fInitStatus = port;
		} else {
			data.SetInt32("port", port);
			AddPort(data);
		}
	}

	return fInitStatus;
}


status_t
Job::InitCheck() const
{
	return fInitStatus;
}


team_id
Job::Team() const
{
	return fTeam;
}


const PortMap&
Job::Ports() const
{
	return fPortMap;
}


port_id
Job::Port(const char* name) const
{
	PortMap::const_iterator found = fPortMap.find(name);
	if (found != fPortMap.end())
		return found->second.GetInt32("port", -1);

	return B_NAME_NOT_FOUND;
}


status_t
Job::Launch()
{
	if (fArguments.IsEmpty()) {
		// Launch by signature
		BString signature("application/");
		signature << Name();
		return be_roster->Launch(signature.String(), (BMessage*)NULL, &fTeam);
	}

	entry_ref ref;
	status_t status = get_ref_for_path(fArguments.StringAt(0).String(), &ref);
	if (status != B_OK)
		return status;

	size_t count = fArguments.CountStrings() - 1;
	const char* args[count + 1];
	for (int32 i = 1; i < fArguments.CountStrings(); i++) {
		args[i - 1] = fArguments.StringAt(i);
	}
	args[count] = NULL;

	return be_roster->Launch(&ref, count, args, &fTeam);
}


bool
Job::IsLaunched() const
{
	return fTeam >= 0;
}


status_t
Job::Execute()
{
	if (!IsLaunched())
		return Launch();

	return B_OK;
}


void
Job::_DeletePorts()
{
	PortMap::const_iterator iterator = Ports().begin();
	for (; iterator != Ports().end(); iterator++) {
		port_id port = iterator->second.GetInt32("port", -1);
		if (port >= 0)
			delete_port(port);
	}
}


status_t
Job::_AddRequirement(BJob* dependency)
{
	if (dependency == NULL)
		return B_OK;

	switch (dependency->State()) {
		case B_JOB_STATE_WAITING_TO_RUN:
		case B_JOB_STATE_STARTED:
		case B_JOB_STATE_IN_PROGRESS:
			AddDependency(dependency);
			break;

		case B_JOB_STATE_SUCCEEDED:
			// Just queue it without any dependencies
			break;

		case B_JOB_STATE_FAILED:
		case B_JOB_STATE_ABORTED:
			// TODO: return appropriate error
			return B_BAD_VALUE;
	}

	return B_OK;
}
