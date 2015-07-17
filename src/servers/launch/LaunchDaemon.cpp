/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "LaunchDaemon.h"

#include <map>
#include <set>

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Directory.h>
#include <driver_settings.h>
#include <Entry.h>
#include <File.h>
#include <ObjectList.h>
#include <Path.h>
#include <PathFinder.h>
#include <Server.h>

#include <AppMisc.h>
#include <LaunchDaemonDefs.h>
#include <LaunchRosterPrivate.h>
#include <RosterPrivate.h>
#include <syscalls.h>

#include "multiuser_utils.h"

#include "Conditions.h"
#include "Events.h"
#include "InitRealTimeClockJob.h"
#include "InitSharedMemoryDirectoryJob.h"
#include "InitTemporaryDirectoryJob.h"
#include "Job.h"
#include "SettingsParser.h"
#include "Target.h"
#include "Utility.h"
#include "Worker.h"


using namespace ::BPrivate;
using namespace BSupportKit;
using BSupportKit::BPrivate::JobQueue;


static const char* kLaunchDirectory = "launch";


class Session {
public:
								Session(uid_t user, const BMessenger& target);

			uid_t				User() const
									{ return fUser; }
			const BMessenger&	Daemon() const
									{ return fDaemon; }

private:
			uid_t				fUser;
			BMessenger			fDaemon;
};


class RegisteredEvent {
public:
								RegisteredEvent(BMessenger& source,
									const char* ownerName,
									const char* name);
								~RegisteredEvent();

			const char*			Name() const;

			int32				CountListeners() const;
			BaseJob*			ListenerAt(int32 index) const;

			status_t			AddListener(BaseJob* job);
			void				RemoveListener(BaseJob* job);

private:
			BString				fName;
			BObjectList<BaseJob> fListeners;
};


typedef std::map<BString, Job*> JobMap;
typedef std::map<uid_t, Session*> SessionMap;
typedef std::map<BString, Target*> TargetMap;
typedef std::map<BString, RegisteredEvent*> EventMap;


class LaunchDaemon : public BServer, public Finder, public ConditionContext {
public:
								LaunchDaemon(bool userMode,
									const EventMap& events, status_t& error);
	virtual						~LaunchDaemon();

	virtual	Job*				FindJob(const char* name) const;
	virtual	Target*				FindTarget(const char* name) const;
			Session*			FindSession(uid_t user) const;

	virtual	bool				IsSafeMode() const;
	virtual	bool				BootVolumeIsReadOnly() const;

	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_HandleGetLaunchData(BMessage* message);
			void				_HandleLaunchTarget(BMessage* message);
			void				_HandleLaunchSession(BMessage* message);
			void				_HandleRegisterSessionDaemon(BMessage* message);
			void				_HandleRegisterLaunchEvent(BMessage* message);
			void				_HandleUnregisterLaunchEvent(BMessage* message);
			void				_HandleNotifyLaunchEvent(BMessage* message);

			uid_t				_GetUserID(BMessage* message);

			void				_ReadPaths(const BStringList& paths);
			void				_ReadEntry(const char* context, BEntry& entry);
			void				_ReadDirectory(const char* context,
									BEntry& directory);
			status_t			_ReadFile(const char* context, BEntry& entry);

			void				_AddJobs(Target* target, BMessage& message);
			void				_AddTargets(BMessage& message);
			void				_AddRunTargets(BMessage& message);
			void				_AddRunTargets(BMessage& message,
									const char* name);
			void				_AddJob(Target* target, bool service,
									BMessage& message);
			void				_InitJobs(Target* target);
			void				_LaunchJobs(Target* target,
									bool forceNow = false);
			void				_LaunchJob(Job* job, bool forceNow = false);
			void				_AddTarget(Target* target);
			void				_SetCondition(BaseJob* job,
									const BMessage& message);
			void				_SetEvent(BaseJob* job,
									const BMessage& message);
			void				_SetEnvironment(BaseJob* job,
									const BMessage& message);

			RegisteredEvent*	_FindEvent(const char* owner,
									const char* name) const;
			void				_ResolveRegisteredEvents(RegisteredEvent* event,
									const BString& name);
			void				_ResolveRegisteredEvents(BaseJob* job);
			void				_ForwardEventMessage(uid_t user,
									BMessage* message);

			status_t			_StartSession(const char* login);

			void				_RetrieveKernelOptions();
			void				_SetupEnvironment();
			void				_InitSystem();
			void				_AddInitJob(BJob* job);

private:
			JobMap				fJobs;
			TargetMap			fTargets;
			BStringList			fRunTargets;
			EventMap			fEvents;
			JobQueue			fJobQueue;
			SessionMap			fSessions;
			MainWorker*			fMainWorker;
			Target*				fInitTarget;
			bool				fSafeMode;
			bool				fReadOnlyBootVolume;
			bool				fUserMode;
};


static const char*
get_leaf(const char* signature)
{
	const char* separator = strrchr(signature, '/');
	if (separator != NULL)
		return separator + 1;

	return signature;
}


// #pragma mark -


Session::Session(uid_t user, const BMessenger& daemon)
	:
	fUser(user),
	fDaemon(daemon)
{
}


// #pragma mark -


RegisteredEvent::RegisteredEvent(BMessenger& source, const char* ownerName,
	const char* name)
	:
	fName(name),
	fListeners(5, true)
{
}


RegisteredEvent::~RegisteredEvent()
{
}


const char*
RegisteredEvent::Name() const
{
	return fName.String();
}


int32
RegisteredEvent::CountListeners() const
{
	return fListeners.CountItems();
}


BaseJob*
RegisteredEvent::ListenerAt(int32 index) const
{
	return fListeners.ItemAt(index);
}


status_t
RegisteredEvent::AddListener(BaseJob* job)
{
	if (fListeners.AddItem(job))
		return B_OK;

	return B_NO_MEMORY;
}


void
RegisteredEvent::RemoveListener(BaseJob* job)
{
	fListeners.RemoveItem(job);
}


// #pragma mark -


LaunchDaemon::LaunchDaemon(bool userMode, const EventMap& events,
	status_t& error)
	:
	BServer(kLaunchDaemonSignature, NULL,
		create_port(B_LOOPER_PORT_DEFAULT_CAPACITY,
			userMode ? "AppPort" : B_LAUNCH_DAEMON_PORT_NAME), false, &error),
	fEvents(events),
	fInitTarget(userMode ? NULL : new Target("init")),
	fUserMode(userMode)
{
	fMainWorker = new MainWorker(fJobQueue);
	fMainWorker->Init();

	if (fInitTarget != NULL)
		_AddTarget(fInitTarget);

	// We may not be able to talk to the registrar
	if (!fUserMode)
		BRoster::Private().SetWithoutRegistrar(true);
}


LaunchDaemon::~LaunchDaemon()
{
}


Job*
LaunchDaemon::FindJob(const char* name) const
{
	if (name == NULL)
		return NULL;

	JobMap::const_iterator found = fJobs.find(BString(name).ToLower());
	if (found != fJobs.end())
		return found->second;

	return NULL;
}


Target*
LaunchDaemon::FindTarget(const char* name) const
{
	if (name == NULL)
		return NULL;

	TargetMap::const_iterator found = fTargets.find(BString(name).ToLower());
	if (found != fTargets.end())
		return found->second;

	return NULL;
}


Session*
LaunchDaemon::FindSession(uid_t user) const
{
	SessionMap::const_iterator found = fSessions.find(user);
	if (found != fSessions.end())
		return found->second;

	return NULL;
}


bool
LaunchDaemon::IsSafeMode() const
{
	return fSafeMode;
}


bool
LaunchDaemon::BootVolumeIsReadOnly() const
{
	return fReadOnlyBootVolume;
}


void
LaunchDaemon::ReadyToRun()
{
	_RetrieveKernelOptions();
	_SetupEnvironment();

	fReadOnlyBootVolume = Utility::IsReadOnlyVolume("/boot");
	if (fReadOnlyBootVolume)
		Utility::BlockMedia("/boot", true);

	if (fUserMode) {
		BLaunchRoster roster;
		BLaunchRoster::Private(roster).RegisterSessionDaemon(this);
	} else
		_InitSystem();

	BStringList paths;
	BPathFinder::FindPaths(B_FIND_PATH_DATA_DIRECTORY, kLaunchDirectory,
		fUserMode ? B_FIND_PATHS_USER_ONLY : B_FIND_PATHS_SYSTEM_ONLY, paths);
	_ReadPaths(paths);

	BPathFinder::FindPaths(B_FIND_PATH_SETTINGS_DIRECTORY, kLaunchDirectory,
		fUserMode ? B_FIND_PATHS_USER_ONLY : B_FIND_PATHS_SYSTEM_ONLY, paths);
	_ReadPaths(paths);

	_InitJobs(NULL);
	_LaunchJobs(NULL);

	// Launch run targets (ignores events)
	for (int32 index = 0; index < fRunTargets.CountStrings(); index++) {
		Target* target = FindTarget(fRunTargets.StringAt(index));
		if (target != NULL)
			_LaunchJobs(target);
	}
}


void
LaunchDaemon::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_GET_LAUNCH_DATA:
			_HandleGetLaunchData(message);
			break;

		case B_LAUNCH_TARGET:
			_HandleLaunchTarget(message);
			break;

		case B_LAUNCH_SESSION:
			_HandleLaunchSession(message);
			break;

		case B_REGISTER_SESSION_DAEMON:
			_HandleRegisterSessionDaemon(message);
			break;

		case B_REGISTER_LAUNCH_EVENT:
			_HandleRegisterLaunchEvent(message);
			break;

		case B_UNREGISTER_LAUNCH_EVENT:
			_HandleUnregisterLaunchEvent(message);
			break;

		case B_NOTIFY_LAUNCH_EVENT:
			_HandleNotifyLaunchEvent(message);
			break;

		case kMsgEventTriggered:
		{
			// An internal event has been triggered.
			// Check if its job can be launched now.
			const char* name = message->GetString("owner");
			if (name == NULL)
				break;

			Job* job = FindJob(name);
			if (job != NULL) {
				_LaunchJob(job);
				break;
			}

			Target* target = FindTarget(name);
			if (target != NULL) {
				_LaunchJobs(target);
				break;
			}
			break;
		}

		default:
			BServer::MessageReceived(message);
			break;
	}
}


void
LaunchDaemon::_HandleGetLaunchData(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	BMessage reply((uint32)B_OK);
	bool launchJob = true;

	Job* job = FindJob(get_leaf(message->GetString("name")));
	if (job == NULL) {
		Session* session = FindSession(user);
		if (session != NULL) {
			// Forward request to user launch_daemon
			if (session->Daemon().SendMessage(message) == B_OK)
				return;
		}
		reply.what = B_NAME_NOT_FOUND;
	} else if (!job->IsLaunched()) {
		if (job->InitCheck() == B_NO_INIT || !job->CheckCondition(*this)) {
			// The job exists, but cannot be started yet, as its
			// conditions are not met; don't make it available yet
			// TODO: we may not want to initialize jobs with conditions
			// that aren't met yet
			reply.what = B_NO_INIT;
		} else if (job->Event() != NULL) {
			if (!Events::TriggerDemand(job->Event())) {
				// The job is not triggered by demand; we cannot start it now
				reply.what = B_NO_INIT;
			} else {
				// The job has already been triggered, don't launch it again
				launchJob = false;
			}
		}
	}

	if (reply.what == B_OK) {
		// If the job has not been launched yet, we'll pass on our
		// team here. The rationale behind this is that this team
		// will temporarily own the synchronous reply ports.
		reply.AddInt32("team", job->Team() < 0
			? current_team() : job->Team());

		PortMap::const_iterator iterator = job->Ports().begin();
		for (; iterator != job->Ports().end(); iterator++) {
			BString name;
			if (iterator->second.HasString("name"))
				name << iterator->second.GetString("name") << "_";
			name << "port";

			reply.AddInt32(name.String(),
				iterator->second.GetInt32("port", -1));
		}

		// Launch the job if it hasn't been launched already
		if (launchJob)
			_LaunchJob(job);
	}
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleLaunchTarget(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("target");
	const char* baseName = message->GetString("base target");

	Target* target = FindTarget(name);
	if (target == NULL && baseName != NULL) {
		Target* baseTarget = FindTarget(baseName);
		if (baseTarget != NULL) {
			target = new Target(name);

			// Copy all jobs with the base target into the new target
			for (JobMap::iterator iterator = fJobs.begin();
					iterator != fJobs.end();) {
				Job* job = iterator->second;
				iterator++;

				if (job->Target() == baseTarget) {
					Job* copy = new Job(*job);
					copy->SetTarget(target);

					fJobs.insert(std::make_pair(copy->Name(), copy));
				}
			}
		}
	}
	if (target == NULL) {
		Session* session = FindSession(user);
		if (session != NULL) {
			// Forward request to user launch_daemon
			if (session->Daemon().SendMessage(message) == B_OK)
				return;
		}

		BMessage reply(B_NAME_NOT_FOUND);
		message->SendReply(&reply);
		return;
	}

	BMessage data;
	if (message->FindMessage("data", &data) == B_OK)
		target->AddData(data.GetString("name"), data);

	_LaunchJobs(target);
}


void
LaunchDaemon::_HandleLaunchSession(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	status_t status = B_OK;
	const char* login = message->GetString("login");
	if (login == NULL)
		status = B_BAD_VALUE;
	if (status == B_OK && user != 0) {
		// Only the root user can start sessions
		// TODO: we'd actually need to know the uid of the sender
		status = B_PERMISSION_DENIED;
	}
	if (status == B_OK)
		status = _StartSession(login);

	BMessage reply((uint32)status);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleRegisterSessionDaemon(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	status_t status = B_OK;

	BMessenger target;
	if (message->FindMessenger("daemon", &target) != B_OK)
		status = B_BAD_VALUE;

	if (status == B_OK) {
		Session* session = new (std::nothrow) Session(user, target);
		if (session != NULL)
			fSessions.insert(std::make_pair(user, session));
		else
			status = B_NO_MEMORY;
	}

	BMessage reply((uint32)status);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleRegisterLaunchEvent(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	if (user == 0 || fUserMode) {
		status_t status = B_OK;

		const char* name = message->GetString("name");
		const char* ownerName = message->GetString("owner");
		BMessenger source;
		if (name != NULL && ownerName != NULL
			&& message->FindMessenger("source", &source) == B_OK) {
			// Register event
			ownerName = get_leaf(ownerName);

			RegisteredEvent* event = new (std::nothrow) RegisteredEvent(
				source, ownerName, name);
			if (event != NULL) {
				// Use short name, and fully qualified name
				BString eventName = name;
				fEvents.insert(std::make_pair(eventName, event));
				_ResolveRegisteredEvents(event, eventName);

				eventName.Prepend("/");
				eventName.Prepend(ownerName);
				fEvents.insert(std::make_pair(eventName, event));
				_ResolveRegisteredEvents(event, eventName);
			} else
				status = B_NO_MEMORY;
		} else
			status = B_BAD_VALUE;

		BMessage reply((uint32)status);
		message->SendReply(&reply);
	}

	_ForwardEventMessage(user, message);
}


void
LaunchDaemon::_HandleUnregisterLaunchEvent(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	if (user == 0 || fUserMode) {
		status_t status = B_OK;

		const char* name = message->GetString("name");
		const char* ownerName = message->GetString("owner");
		BMessenger source;
		if (name != NULL && ownerName != NULL
			&& message->FindMessenger("source", &source) == B_OK) {
			// Unregister short and fully qualified event name
			ownerName = get_leaf(ownerName);

			BString eventName = name;
			fEvents.erase(eventName);

			eventName.Prepend("/");
			eventName.Prepend(ownerName);
			fEvents.erase(eventName);
		} else
			status = B_BAD_VALUE;

		BMessage reply((uint32)status);
		message->SendReply(&reply);
	}

	_ForwardEventMessage(user, message);
}


void
LaunchDaemon::_HandleNotifyLaunchEvent(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	if (user == 0 || fUserMode) {
		// Trigger events
		const char* name = message->GetString("name");
		const char* ownerName = message->GetString("owner");
		// TODO: support arguments (as selectors)

		RegisteredEvent* event = _FindEvent(ownerName, name);
		if (event != NULL) {
			// Evaluate all of its jobs
			int32 count = event->CountListeners();
			for (int32 index = 0; index < count; index++) {
				BaseJob* listener = event->ListenerAt(index);
				Events::TriggerRegisteredEvent(listener->Event(), name);
			}
		}
	}

	_ForwardEventMessage(user, message);
}


uid_t
LaunchDaemon::_GetUserID(BMessage* message)
{
	uid_t user = (uid_t)message->GetInt32("user", -1);
	if (user < 0) {
		BMessage reply((uint32)B_BAD_VALUE);
		message->SendReply(&reply);
	}
	return user;
}


void
LaunchDaemon::_ReadPaths(const BStringList& paths)
{
	for (int32 i = 0; i < paths.CountStrings(); i++) {
		BEntry entry(paths.StringAt(i));
		if (entry.InitCheck() != B_OK || !entry.Exists())
			continue;

		_ReadDirectory(NULL, entry);
	}
}


void
LaunchDaemon::_ReadEntry(const char* context, BEntry& entry)
{
	if (entry.IsDirectory())
		_ReadDirectory(context, entry);
	else
		_ReadFile(context, entry);
}


void
LaunchDaemon::_ReadDirectory(const char* context, BEntry& directoryEntry)
{
	BDirectory directory(&directoryEntry);

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		_ReadEntry(context, entry);
	}
}


status_t
LaunchDaemon::_ReadFile(const char* context, BEntry& entry)
{
	BPath path;
	status_t status = path.SetTo(&entry);
	if (status != B_OK)
		return status;

	SettingsParser parser;
	BMessage message;
	status = parser.ParseFile(path.Path(), message);
	if (status == B_OK) {
		_AddJobs(NULL, message);
		_AddTargets(message);
		_AddRunTargets(message);
	}

	return status;
}


void
LaunchDaemon::_AddJobs(Target* target, BMessage& message)
{
	BMessage job;
	for (int32 index = 0; message.FindMessage("service", index,
			&job) == B_OK; index++) {
		_AddJob(target, true, job);
	}

	for (int32 index = 0; message.FindMessage("job", index, &job) == B_OK;
			index++) {
		_AddJob(target, false, job);
	}
}


void
LaunchDaemon::_AddTargets(BMessage& message)
{
	BMessage targetMessage;
	for (int32 index = 0; message.FindMessage("target", index,
			&targetMessage) == B_OK; index++) {
		const char* name = targetMessage.GetString("name");
		if (name == NULL) {
			// TODO: log error
			debug_printf("Target has no name, ignoring it!\n");
			continue;
		}

		Target* target = FindTarget(name);
		if (target == NULL) {
			target = new Target(name);
			_AddTarget(target);
		} else if (targetMessage.GetBool("reset")) {
			// Remove all jobs from this target
			for (JobMap::iterator iterator = fJobs.begin();
					iterator != fJobs.end();) {
				Job* job = iterator->second;
				JobMap::iterator remove = iterator++;

				if (job->Target() == target) {
					fJobs.erase(remove);
					delete job;
				}
			}
		}

		_SetCondition(target, targetMessage);
		_SetEvent(target, targetMessage);
		_SetEnvironment(target, targetMessage);
		_AddJobs(target, targetMessage);
	}
}


void
LaunchDaemon::_AddRunTargets(BMessage& message)
{
	BMessage runMessage;
	for (int32 index = 0; message.FindMessage("run", index,
			&runMessage) == B_OK; index++) {
		BMessage conditions;
		bool pass = true;
		if (runMessage.FindMessage("if", &conditions) == B_OK) {
			Condition* condition = Conditions::FromMessage(conditions);
			if (condition != NULL) {
				pass = condition->Test(*this);
				debug_printf("Test: %s -> %d\n", condition->ToString().String(),
					pass);
				delete condition;
			} else
				debug_printf("Could not parse condition!\n");
		}

		if (pass) {
			_AddRunTargets(runMessage, NULL);
			_AddRunTargets(runMessage, "then");
		} else {
			_AddRunTargets(runMessage, "else");
		}
	}
}


void
LaunchDaemon::_AddRunTargets(BMessage& message, const char* name)
{
	BMessage targets;
	if (name != NULL && message.FindMessage(name, &targets) != B_OK)
		return;

	const char* target;
	for (int32 index = 0; targets.FindString("target", index, &target) == B_OK;
			index++) {
		fRunTargets.Add(target);
	}
}


void
LaunchDaemon::_AddJob(Target* target, bool service, BMessage& message)
{
	BString name = message.GetString("name");
	if (name.IsEmpty()) {
		// Invalid job description
		return;
	}
	name.ToLower();

	Job* job = FindJob(name);
	if (job == NULL) {
		job = new (std::nothrow) Job(name);
		if (job == NULL)
			return;

		job->SetService(service);
		job->SetCreateDefaultPort(service);
		job->SetTarget(target);
	}

	if (message.HasBool("disabled"))
		job->SetEnabled(!message.GetBool("disabled", !job->IsEnabled()));

	if (message.HasBool("legacy"))
		job->SetCreateDefaultPort(!message.GetBool("legacy", !service));

	_SetCondition(job, message);
	_SetEvent(job, message);
	_SetEnvironment(job, message);

	BMessage portMessage;
	for (int32 index = 0;
			message.FindMessage("port", index, &portMessage) == B_OK; index++) {
		job->AddPort(portMessage);
	}

	if (message.HasString("launch")) {
		job->Arguments().MakeEmpty();

		const char* argument;
		for (int32 index = 0; message.FindString("launch", index, &argument)
				== B_OK; index++) {
			job->AddArgument(argument);
		}
	}

	const char* requirement;
	for (int32 index = 0;
			message.FindString("requires", index, &requirement) == B_OK;
			index++) {
		job->AddRequirement(requirement);
	}
	if (fInitTarget != NULL)
		job->AddRequirement(fInitTarget->Name());

	fJobs.insert(std::make_pair(job->Title(), job));
}


/*!	Initializes all jobs for the specified target (may be \c NULL).
	Jobs that cannot be initialized, and those that never will be due to
	conditions, will be removed from the list.
*/
void
LaunchDaemon::_InitJobs(Target* target)
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();) {
		Job* job = iterator->second;
		JobMap::iterator remove = iterator++;

		if (job->Target() != target)
			continue;

		status_t status = B_NO_INIT;
		if (job->IsEnabled()) {
			// Filter out jobs that have a constant and failing condition
			if (job->Condition() == NULL || !job->Condition()->IsConstant(*this)
				|| job->Condition()->Test(*this)) {
				std::set<BString> dependencies;
				status = job->Init(*this, dependencies);
			}
		}

		if (status != B_OK) {
			if (status != B_NO_INIT) {
				// TODO: log error
				debug_printf("Init \"%s\" failed: %s\n", job->Name(),
					strerror(status));
			}

			// Remove jobs that won't be used later on
			fJobs.erase(remove);
			delete job;
		}
	}
}


/*!	Adds all jobs for the specified target (may be \c NULL) to the launch
	queue, except those that are triggered by events that haven't been
	triggered yet.

	Unless \a forceNow is true, the target is only launched if its events,
	if any, have been triggered already, and its conditions are met.
*/
void
LaunchDaemon::_LaunchJobs(Target* target, bool forceNow)
{
	if (!forceNow && target != NULL && (!target->EventHasTriggered()
		|| !target->CheckCondition(*this))) {
		return;
	}

	if (target != NULL && !target->HasLaunched()) {
		target->SetLaunched(true);
		_InitJobs(target);
	}

	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Job* job = iterator->second;
		if (job->Target() == target)
			_LaunchJob(job);
	}
}


/*!	Adds the specified \a job to the launch queue
	queue, except those that are triggered by events.

	Unless \a forceNow is true, the target is only launched if its events,
	if any, have been triggered already.

	Calling this method will trigger a demand event.
*/
void
LaunchDaemon::_LaunchJob(Job* job, bool forceNow)
{
	if (job == NULL || job->IsLaunched() || !forceNow
		&& (!job->EventHasTriggered() || !job->CheckCondition(*this)
			|| Events::TriggerDemand(job->Event()))) {
		return;
	}

	int32 count = job->Requirements().CountStrings();
	for (int32 index = 0; index < count; index++) {
		Job* requirement = FindJob(job->Requirements().StringAt(index));
		if (requirement != NULL)
			_LaunchJob(requirement);
	}

	if (job->Target() != NULL)
		job->Target()->ResolveSourceFiles();
	if (job->Event() != NULL)
		job->Event()->ResetTrigger();

	fJobQueue.AddJob(job);
}


void
LaunchDaemon::_AddTarget(Target* target)
{
	fTargets.insert(std::make_pair(target->Title(), target));
}


void
LaunchDaemon::_SetCondition(BaseJob* job, const BMessage& message)
{
	Condition* condition = job->Condition();
	bool updated = false;

	BMessage conditions;
	if (message.FindMessage("if", &conditions) == B_OK) {
		condition = Conditions::FromMessage(conditions);
		updated = true;
	}

	if (message.GetBool("no_safemode")) {
		condition = Conditions::AddNotSafeMode(condition);
		updated = true;
	}

	if (updated)
		job->SetCondition(condition);
}


void
LaunchDaemon::_SetEvent(BaseJob* job, const BMessage& message)
{
	Event* event = job->Event();
	bool updated = false;

	BMessage events;
	if (message.FindMessage("on", &events) == B_OK) {
		event = Events::FromMessage(this, events);
		updated = true;
	}

	if (message.GetBool("on_demand")) {
		event = Events::AddOnDemand(event);
		updated = true;
	}

	if (updated) {
		job->SetEvent(event);
		_ResolveRegisteredEvents(job);
	}
}


void
LaunchDaemon::_SetEnvironment(BaseJob* job, const BMessage& message)
{
	BMessage environmentMessage;
	if (message.FindMessage("env", &environmentMessage) == B_OK)
		job->SetEnvironment(environmentMessage);
}


RegisteredEvent*
LaunchDaemon::_FindEvent(const char* owner, const char* name) const
{
	if (name == NULL)
		return NULL;

	BString eventName = name;
	eventName.ToLower();

	EventMap::const_iterator found = fEvents.find(eventName);
	if (found != fEvents.end())
		return found->second;

	if (owner == NULL)
		return NULL;

	eventName.Prepend("/");
	eventName.Prepend(get_leaf(owner));

	found = fEvents.find(eventName);
	if (found != fEvents.end())
		return found->second;

	return NULL;
}


void
LaunchDaemon::_ResolveRegisteredEvents(RegisteredEvent* event,
	const BString& name)
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Job* job = iterator->second;
		if (Events::ResolveRegisteredEvent(job->Event(), name))
			event->AddListener(job);
	}
}


void
LaunchDaemon::_ResolveRegisteredEvents(BaseJob* job)
{
	if (job->Event() == NULL)
		return;

	for (EventMap::iterator iterator = fEvents.begin();
			iterator != fEvents.end(); iterator++) {
		RegisteredEvent* event = iterator->second;
		if (Events::ResolveRegisteredEvent(job->Event(), event->Name()))
			event->AddListener(job);
	}
}


void
LaunchDaemon::_ForwardEventMessage(uid_t user, BMessage* message)
{
	if (fUserMode)
		return;

	// Forward event to user launch_daemon(s)
	if (user == 0) {
		for (SessionMap::iterator iterator = fSessions.begin();
				iterator != fSessions.end(); iterator++) {
			Session* session = iterator->second;
			session->Daemon().SendMessage(message);
				// ignore reply
		}
	} else {
		Session* session = FindSession(user);
		if (session != NULL)
			session->Daemon().SendMessage(message);
	}
}


status_t
LaunchDaemon::_StartSession(const char* login)
{
	// TODO: enable user/group code
	// The launch_daemon currently cannot talk to the registrar, though

	struct passwd* passwd = getpwnam(login);
	if (passwd == NULL)
		return B_NAME_NOT_FOUND;
	if (strcmp(passwd->pw_name, login) != 0)
		return B_NAME_NOT_FOUND;

	// Check if there is a user session running already
	uid_t user = passwd->pw_uid;
	gid_t group = passwd->pw_gid;

	Unlock();

	if (fork() == 0) {
		if (setsid() < 0)
			exit(EXIT_FAILURE);

		if (initgroups(login, group) == -1)
			exit(EXIT_FAILURE);
		if (setgid(group) != 0)
			exit(EXIT_FAILURE);
		if (setuid(user) != 0)
			exit(EXIT_FAILURE);

		BString home="HOME=\"";
		home << passwd->pw_dir << "\"";
		putenv(home.String());

		// TODO: This leaks the parent application
		be_app = NULL;

		// TODO: take over system jobs, and reserve their names
		status_t status;
		LaunchDaemon* daemon = new LaunchDaemon(true, fEvents, status);
		if (status == B_OK)
			daemon->Run();

		delete daemon;
		exit(EXIT_SUCCESS);
	}
	Lock();
	return B_OK;
}


void
LaunchDaemon::_RetrieveKernelOptions()
{
	char buffer[32];
	size_t size = sizeof(buffer);
	status_t status = _kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, buffer,
		&size);
	if (status == B_OK) {
		fSafeMode = !strncasecmp(buffer, "true", size)
			|| !strncasecmp(buffer, "yes", size)
			|| !strncasecmp(buffer, "on", size)
			|| !strncasecmp(buffer, "enabled", size);
	} else
		fSafeMode = false;
}


void
LaunchDaemon::_SetupEnvironment()
{
	// Determine safemode kernel option
	BString safemode = "SAFEMODE=";
	safemode << (IsSafeMode() ? "yes" : "no");

	putenv(safemode.String());

	// Default locale settings
	putenv("LC_TYPE=en_US.UTF-8");
}


/*!	Basic system initialization that must happen before any jobs are launched.
*/
void
LaunchDaemon::_InitSystem()
{
	_AddInitJob(new InitRealTimeClockJob());
	_AddInitJob(new InitSharedMemoryDirectoryJob());
	_AddInitJob(new InitTemporaryDirectoryJob());

	fJobQueue.AddJob(fInitTarget);
}


void
LaunchDaemon::_AddInitJob(BJob* job)
{
	fInitTarget->AddDependency(job);
	fJobQueue.AddJob(job);
}


// #pragma mark -


int
main()
{
	EventMap events;
	status_t status;
	LaunchDaemon* daemon = new LaunchDaemon(false, events, status);
	if (status == B_OK)
		daemon->Run();

	delete daemon;
	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
