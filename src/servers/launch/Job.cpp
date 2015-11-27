/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Job.h"

#include <stdlib.h>

#include <Entry.h>
#include <Looper.h>
#include <Message.h>
#include <Roster.h>

#include <MessagePrivate.h>
#include <RosterPrivate.h>
#include <user_group.h>

#include "Target.h"
#include "Utility.h"


Job::Job(const char* name)
	:
	BaseJob(name),
	fEnabled(true),
	fService(false),
	fCreateDefaultPort(false),
	fLaunching(false),
	fInitStatus(B_NO_INIT),
	fTeam(-1),
	fDefaultPort(-1),
	fToken((uint32)B_PREFERRED_TOKEN),
	fLaunchStatus(B_NO_INIT),
	fTarget(NULL),
	fPendingLaunchDataReplies(0, false)
{
	mutex_init(&fLaunchStatusLock, "launch status lock");
}


Job::Job(const Job& other)
	:
	BaseJob(other.Name()),
	fEnabled(other.IsEnabled()),
	fService(other.IsService()),
	fCreateDefaultPort(other.CreateDefaultPort()),
	fLaunching(other.IsLaunching()),
	fInitStatus(B_NO_INIT),
	fTeam(-1),
	fDefaultPort(-1),
	fToken((uint32)B_PREFERRED_TOKEN),
	fLaunchStatus(B_NO_INIT),
	fTarget(other.Target()),
	fPendingLaunchDataReplies(0, false)
{
	mutex_init(&fLaunchStatusLock, "launch status lock");

	fCondition = other.fCondition;
	// TODO: copy events
	//fEvent = other.fEvent;
	fEnvironment = other.fEnvironment;
	fSourceFiles = other.fSourceFiles;

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


::TeamRegistrator*
Job::TeamRegistrator() const
{
	return fTeamRegistrator;
}


void
Job::SetTeamRegistrator(::TeamRegistrator* registrator)
{
	fTeamRegistrator = registrator;
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


const BStringList&
Job::Pending() const
{
	return fPendingJobs;
}


BStringList&
Job::Pending()
{
	return fPendingJobs;
}


void
Job::AddPending(const char* pending)
{
	fPendingJobs.Add(pending);
}


bool
Job::CheckCondition(ConditionContext& context) const
{
	if (Target() != NULL && !Target()->HasLaunched())
		return false;

	return BaseJob::CheckCondition(context);
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


port_id
Job::DefaultPort() const
{
	return fDefaultPort;
}


void
Job::SetDefaultPort(port_id port)
{
	fDefaultPort = port;

	PortMap::iterator iterator = fPortMap.begin();
	for (; iterator != fPortMap.end(); iterator++) {
		BString name;
		if (iterator->second.HasString("name"))
			continue;

		iterator->second.SetInt32("port", (int32)port);
		break;
	}
}


status_t
Job::Launch()
{
	// Build environment

	std::vector<const char*> environment;
	for (const char** variable = (const char**)environ; variable[0] != NULL;
			variable++) {
		environment.push_back(variable[0]);
	}

	if (Target() != NULL)
		_AddStringList(environment, Target()->Environment());
	_AddStringList(environment, Environment());

	// Resolve source files
	BStringList sourceFilesEnvironment;
	GetSourceFilesEnvironment(sourceFilesEnvironment);
	_AddStringList(environment, sourceFilesEnvironment);

	environment.push_back(NULL);

	if (fArguments.IsEmpty()) {
		// Launch by signature
		BString signature("application/");
		signature << Name();

		return _Launch(signature.String(), NULL, 0, NULL, &environment[0]);
	}

	// Build argument vector

	entry_ref ref;
	status_t status = get_ref_for_path(
		Utility::TranslatePath(fArguments.StringAt(0).String()), &ref);
	if (status != B_OK) {
		_SetLaunchStatus(status);
		return status;
	}

	std::vector<BString> strings;
	std::vector<const char*> args;

	size_t count = fArguments.CountStrings() - 1;
	if (count > 0) {
		for (int32 i = 1; i < fArguments.CountStrings(); i++) {
			strings.push_back(Utility::TranslatePath(fArguments.StringAt(i)));
			args.push_back(strings.back());
		}
		args.push_back(NULL);
	}

	// Launch via entry_ref
	return _Launch(NULL, &ref, count, &args[0], &environment[0]);
}


bool
Job::IsLaunched() const
{
	return fLaunchStatus != B_NO_INIT;
}


bool
Job::IsRunning() const
{
	return fTeam >= 0;
}


void
Job::TeamDeleted()
{
	fTeam = -1;
	fDefaultPort = -1;

	if (IsService())
		SetState(B_JOB_STATE_WAITING_TO_RUN);

	MutexLocker locker(fLaunchStatusLock);
	fLaunchStatus = B_NO_INIT;
}


bool
Job::CanBeLaunched() const
{
	// Services cannot be launched while they are running
	return IsEnabled() && !IsLaunching() && (!IsService() || !IsRunning());
}


bool
Job::IsLaunching() const
{
	return fLaunching;
}


void
Job::SetLaunching(bool launching)
{
	fLaunching = launching;
}


status_t
Job::HandleGetLaunchData(BMessage* message)
{
	MutexLocker launchLocker(fLaunchStatusLock);
	if (IsLaunched())
		return _SendLaunchDataReply(message);

	return fPendingLaunchDataReplies.AddItem(message) ? B_OK : B_NO_MEMORY;
}


status_t
Job::GetMessenger(BMessenger& messenger)
{
	if (fDefaultPort < 0)
		return B_NAME_NOT_FOUND;

	BMessenger::Private(messenger).SetTo(fTeam, fDefaultPort, fToken);
	return B_OK;
}


status_t
Job::Run()
{
	status_t status = BJob::Run();

	// Jobs can be relaunched at any time
	if (!IsService())
		SetState(B_JOB_STATE_WAITING_TO_RUN);

	return status;
}


status_t
Job::Execute()
{
	status_t status = B_OK;
	if (!IsRunning() || !IsService())
		status = Launch();
	else
		debug_printf("Ignore launching %s\n", Name());

	fLaunching = false;
	return status;
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


void
Job::_AddStringList(std::vector<const char*>& array, const BStringList& list)
{
	int32 count = list.CountStrings();
	for (int32 index = 0; index < count; index++) {
		array.push_back(list.StringAt(index).String());
	}
}


void
Job::_SetLaunchStatus(status_t launchStatus)
{
	MutexLocker launchLocker(fLaunchStatusLock);
	fLaunchStatus = launchStatus != B_NO_INIT ? launchStatus : B_ERROR;
	launchLocker.Unlock();

	_SendPendingLaunchDataReplies();
}


status_t
Job::_SendLaunchDataReply(BMessage* message)
{
	BMessage reply(fTeam < 0 ? fTeam : (uint32)B_OK);
	if (reply.what == B_OK) {
		reply.AddInt32("team", fTeam);

		PortMap::const_iterator iterator = fPortMap.begin();
		for (; iterator != fPortMap.end(); iterator++) {
			BString name;
			if (iterator->second.HasString("name"))
				name << iterator->second.GetString("name") << "_";
			name << "port";

			reply.AddInt32(name.String(),
				iterator->second.GetInt32("port", -1));
		}
	}

	message->SendReply(&reply);
	delete message;
	return B_OK;
}


void
Job::_SendPendingLaunchDataReplies()
{
	for (int32 i = 0; i < fPendingLaunchDataReplies.CountItems(); i++)
		_SendLaunchDataReply(fPendingLaunchDataReplies.ItemAt(i));

	fPendingLaunchDataReplies.MakeEmpty();
}


/*!	Creates the ports for a newly launched job. If the registrar already
	pre-registered the application, \c fDefaultPort will already be set, and
	honored when filling the ports message.
*/
status_t
Job::_CreateAndTransferPorts()
{
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

		port_id port = -1;
		if (suffix != NULL || fDefaultPort < 0) {
			port = _CreateAndTransferPort(name.String(), capacity);
			if (port < 0)
				return port;

			if (suffix == NULL)
				fDefaultPort = port;
		} else if (suffix == NULL)
			port = fDefaultPort;

		iterator->second.SetInt32("port", port);

		if (name == "x-vnd.haiku-registrar:auth") {
			// Allow the launch_daemon to access the registrar authentication
			BPrivate::set_registrar_authentication_port(port);
		}
	}

	if (fCreateDefaultPort && !defaultPort) {
		BMessage data;
		data.AddInt32("capacity", B_LOOPER_PORT_DEFAULT_CAPACITY);

		port_id port = -1;
		if (fDefaultPort < 0) {
			port = _CreateAndTransferPort(Name(),
				B_LOOPER_PORT_DEFAULT_CAPACITY);
			if (port < 0)
				return port;

			fDefaultPort = port;
		} else
			port = fDefaultPort;

		data.SetInt32("port", port);
		AddPort(data);
	}

	return B_OK;
}


port_id
Job::_CreateAndTransferPort(const char* name, int32 capacity)
{
	port_id port = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY, Name());
	if (port < 0)
		return port;

	status_t status = set_port_owner(port, fTeam);
	if (status != B_OK) {
		delete_port(port);
		return status;
	}

	return port;
}


status_t
Job::_Launch(const char* signature, entry_ref* ref, int argCount,
	const char* const* args, const char** environment)
{
	thread_id mainThread = -1;
	status_t result = BRoster::Private().Launch(signature, ref, NULL, argCount,
		args, environment, &fTeam, &mainThread, &fDefaultPort, &fToken, true);
	if (result == B_OK) {
		result = _CreateAndTransferPorts();

		if (result == B_OK) {
			resume_thread(mainThread);

			if (fTeamRegistrator != NULL)
				fTeamRegistrator->RegisterTeam(this);
		} else
			kill_thread(mainThread);
	}

	_SetLaunchStatus(result);
	return result;
}
