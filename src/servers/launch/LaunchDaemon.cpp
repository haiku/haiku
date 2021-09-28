/*
 * Copyright 2015-2018, Axel Dörfler, axeld@pinc-software.de.
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
#include <locks.h>
#include <MessengerPrivate.h>
#include <RosterPrivate.h>
#include <syscalls.h>
#include <system_info.h>

#include "multiuser_utils.h"

#include "Conditions.h"
#include "Events.h"
#include "InitRealTimeClockJob.h"
#include "InitSharedMemoryDirectoryJob.h"
#include "InitTemporaryDirectoryJob.h"
#include "Job.h"
#include "Log.h"
#include "SettingsParser.h"
#include "Target.h"
#include "Utility.h"
#include "Worker.h"


#ifdef DEBUG
#	define TRACE(x, ...) debug_printf(x, __VA_ARGS__)
#else
#	define TRACE(x, ...) ;
#endif


using namespace ::BPrivate;
using namespace BSupportKit;
using BSupportKit::BPrivate::JobQueue;


#ifndef TEST_MODE
static const char* kLaunchDirectory = "launch";
static const char* kUserLaunchDirectory = "user_launch";
#endif


enum launch_options {
	FORCE_NOW		= 0x01,
	TRIGGER_DEMAND	= 0x02
};


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


/*!	This class is the connection between the external events that are part of
	a job, and the external event source.

	There is one object per registered event source, and it keeps all jobs that
	reference the event as listeners. If the event source triggers the event,
	the object will be used to trigger the jobs.
*/
class ExternalEventSource {
public:
								ExternalEventSource(BMessenger& source,
									const char* ownerName,
									const char* name, uint32 flags);
								~ExternalEventSource();

			const char*			Name() const;
			uint32				Flags() const
									{ return fFlags; }

			void				Trigger();
			void				ResetSticky();

			status_t			AddDestination(Event* event);
			void				RemoveDestination(Event* event);

private:
			BString				fName;
			uint32				fFlags;
			BObjectList<Event>	fDestinations;
			bool				fStickyTriggered;
};


typedef std::map<BString, Job*> JobMap;
typedef std::map<uid_t, Session*> SessionMap;
typedef std::map<BString, Target*> TargetMap;
typedef std::map<BString, ExternalEventSource*> EventMap;
typedef std::map<team_id, Job*> TeamMap;


class LaunchDaemon : public BServer, public Finder, public ConditionContext,
	public EventRegistrator, public TeamListener {
public:
								LaunchDaemon(bool userMode,
									const EventMap& events, status_t& error);
	virtual						~LaunchDaemon();

	virtual	Job*				FindJob(const char* name) const;
	virtual	Target*				FindTarget(const char* name) const;
			Session*			FindSession(uid_t user) const;

	// ConditionContext
	virtual	bool				IsSafeMode() const;
	virtual	bool				BootVolumeIsReadOnly() const;

	// EventRegistrator
	virtual	status_t			RegisterExternalEvent(Event* event,
									const char* name,
									const BStringList& arguments);
	virtual	void				UnregisterExternalEvent(Event* event,
									const char* name);

	// TeamListener
	virtual	void				TeamLaunched(Job* job, status_t status);

	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_HandleGetLaunchData(BMessage* message);
			void				_HandleLaunchTarget(BMessage* message);
			void				_HandleStopLaunchTarget(BMessage* message);
			void				_HandleLaunchJob(BMessage* message);
			void				_HandleEnableLaunchJob(BMessage* message);
			void				_HandleStopLaunchJob(BMessage* message);
			void				_HandleLaunchSession(BMessage* message);
			void				_HandleRegisterSessionDaemon(BMessage* message);
			void				_HandleRegisterLaunchEvent(BMessage* message);
			void				_HandleUnregisterLaunchEvent(BMessage* message);
			void				_HandleNotifyLaunchEvent(BMessage* message);
			void				_HandleResetStickyLaunchEvent(
									BMessage* message);
			void				_HandleGetLaunchTargets(BMessage* message);
			void				_HandleGetLaunchTargetInfo(BMessage* message);
			void				_HandleGetLaunchJobs(BMessage* message);
			void				_HandleGetLaunchJobInfo(BMessage* message);
			void				_HandleGetLaunchLog(BMessage* message);
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
			void				_StopJobs(Target* target, bool force);
			bool				_CanLaunchJob(Job* job, uint32 options,
									bool testOnly = false);
			bool				_CanLaunchJobRequirements(Job* job,
									uint32 options);
			bool				_LaunchJob(Job* job, uint32 options = 0);
			void				_StopJob(Job* job, bool force);
			void				_AddTarget(Target* target);
			void				_SetCondition(BaseJob* job,
									const BMessage& message);
			void				_SetEvent(BaseJob* job,
									const BMessage& message);
			void				_SetEnvironment(BaseJob* job,
									const BMessage& message);

			ExternalEventSource*
								_FindExternalEventSource(const char* owner,
									const char* name) const;
			void				_ResolveExternalEvents(
									ExternalEventSource* event,
									const BString& name);
			void				_GetBaseJobInfo(BaseJob* job, BMessage& info);
			void				_ForwardEventMessage(uid_t user,
									BMessage* message);

			status_t			_StartSession(const char* login);

			void				_RetrieveKernelOptions();
			void				_SetupEnvironment();
			void				_InitSystem();
			void				_AddInitJob(BJob* job);

private:
			Log					fLog;
			JobMap				fJobs;
			TargetMap			fTargets;
			BStringList			fRunTargets;
			EventMap			fEvents;
			JobQueue			fJobQueue;
			SessionMap			fSessions;
			MainWorker*			fMainWorker;
			Target*				fInitTarget;
			TeamMap				fTeams;
			mutex				fTeamsLock;
			bool				fSafeMode;
			bool				fReadOnlyBootVolume;
			bool				fUserMode;
};


static const char*
get_leaf(const char* signature)
{
	if (signature == NULL)
		return NULL;

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


ExternalEventSource::ExternalEventSource(BMessenger& source,
	const char* ownerName, const char* name, uint32 flags)
	:
	fName(name),
	fFlags(flags),
	fDestinations(5),
	fStickyTriggered(false)
{
}


ExternalEventSource::~ExternalEventSource()
{
}


const char*
ExternalEventSource::Name() const
{
	return fName.String();
}


void
ExternalEventSource::Trigger()
{
	for (int32 index = 0; index < fDestinations.CountItems(); index++)
		Events::TriggerExternalEvent(fDestinations.ItemAt(index));

	if ((fFlags & B_STICKY_EVENT) != 0)
		fStickyTriggered = true;
}


void
ExternalEventSource::ResetSticky()
{
	if ((fFlags & B_STICKY_EVENT) != 0)
		fStickyTriggered = false;

	for (int32 index = 0; index < fDestinations.CountItems(); index++)
		Events::ResetStickyExternalEvent(fDestinations.ItemAt(index));
}


status_t
ExternalEventSource::AddDestination(Event* event)
{
	if (fDestinations.AddItem(event))
		return B_OK;

	return B_NO_MEMORY;
}


void
ExternalEventSource::RemoveDestination(Event* event)
{
	fDestinations.RemoveItem(event);
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
#ifdef TEST_MODE
	fUserMode(true)
#else
	fUserMode(userMode)
#endif
{
	mutex_init(&fTeamsLock, "teams lock");

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


status_t
LaunchDaemon::RegisterExternalEvent(Event* event, const char* name,
	const BStringList& arguments)
{
	status_t status = B_NAME_NOT_FOUND;
	for (EventMap::iterator iterator = fEvents.begin();
			iterator != fEvents.end(); iterator++) {
		ExternalEventSource* eventSource = iterator->second;
		Event* externalEvent = Events::ResolveExternalEvent(event,
			eventSource->Name(), eventSource->Flags());
		if (externalEvent != NULL) {
			status = eventSource->AddDestination(event);
			break;
		}
	}
	return status;
}


void
LaunchDaemon::UnregisterExternalEvent(Event* event, const char* name)
{
	for (EventMap::iterator iterator = fEvents.begin();
			iterator != fEvents.end(); iterator++) {
		ExternalEventSource* eventSource = iterator->second;
		Event* externalEvent = Events::ResolveExternalEvent(event,
			eventSource->Name(), eventSource->Flags());
		if (externalEvent != NULL) {
			eventSource->RemoveDestination(event);
			break;
		}
	}
}


void
LaunchDaemon::TeamLaunched(Job* job, status_t status)
{
	fLog.JobLaunched(job, status);

	MutexLocker locker(fTeamsLock);
	fTeams.insert(std::make_pair(job->Team(), job));
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
#ifndef TEST_MODE
		BLaunchRoster roster;
		BLaunchRoster::Private(roster).RegisterSessionDaemon(this);
#endif	// TEST_MODE
	} else
		_InitSystem();

	BStringList paths;
#ifdef TEST_MODE
	paths.Add("/boot/home/test_launch");
#else
	if (fUserMode) {
		// System-wide user specific jobs
		BPathFinder::FindPaths(B_FIND_PATH_DATA_DIRECTORY, kUserLaunchDirectory,
			B_FIND_PATHS_SYSTEM_ONLY, paths);
		_ReadPaths(paths);
	}

	BPathFinder::FindPaths(B_FIND_PATH_DATA_DIRECTORY, kLaunchDirectory,
		fUserMode ? B_FIND_PATHS_USER_ONLY : B_FIND_PATHS_SYSTEM_ONLY, paths);
	_ReadPaths(paths);

	if (fUserMode) {
		BPathFinder::FindPaths(B_FIND_PATH_SETTINGS_DIRECTORY,
			kUserLaunchDirectory, B_FIND_PATHS_SYSTEM_ONLY, paths);
		_ReadPaths(paths);
	}

	BPathFinder::FindPaths(B_FIND_PATH_SETTINGS_DIRECTORY, kLaunchDirectory,
		fUserMode ? B_FIND_PATHS_USER_ONLY : B_FIND_PATHS_SYSTEM_ONLY, paths);
#endif	// TEST_MODE
	_ReadPaths(paths);

	BMessenger target(this);
	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();
	__start_watching_system(-1, B_WATCH_SYSTEM_TEAM_DELETION, port, token);

	_InitJobs(NULL);
	_LaunchJobs(NULL);

	// Launch run targets (ignores events)
	for (int32 index = 0; index < fRunTargets.CountStrings(); index++) {
		Target* target = FindTarget(fRunTargets.StringAt(index));
		if (target != NULL)
			_LaunchJobs(target);
	}

	if (fUserMode)
		be_roster->StartWatching(this, B_REQUEST_LAUNCHED);
}


void
LaunchDaemon::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SYSTEM_OBJECT_UPDATE:
		{
			int32 opcode = message->GetInt32("opcode", 0);
			team_id team = (team_id)message->GetInt32("team", -1);
			if (opcode != B_TEAM_DELETED || team < 0)
				break;

			MutexLocker locker(fTeamsLock);

			TeamMap::iterator found = fTeams.find(team);
			if (found != fTeams.end()) {
				Job* job = found->second;
				TRACE("Job %s ended!\n", job->Name());

				// Get the exit status, and pass it on to the log
				status_t exitStatus = B_OK;
				wait_for_thread(team, &exitStatus);
				fLog.JobTerminated(job, exitStatus);
				job->TeamDeleted();

				if (job->IsService()) {
					bool inProgress = false;
					BRoster roster;
					BRoster::Private rosterPrivate(roster);
					status_t status = rosterPrivate.IsShutDownInProgress(
						&inProgress);
					if (status != B_OK || !inProgress) {
						// TODO: take restart throttle into account
						_LaunchJob(job);
					}
				}
			}
			break;
		}
		case B_SOME_APP_LAUNCHED:
		{
			team_id team = (team_id)message->GetInt32("be:team", -1);
			Job* job = NULL;

			MutexLocker locker(fTeamsLock);

			TeamMap::iterator found = fTeams.find(team);
			if (found != fTeams.end()) {
				job = found->second;
				locker.Unlock();
			} else {
				locker.Unlock();

				// Find job by name instead
				const char* signature = message->GetString("be:signature");
				job = FindJob(get_leaf(signature));
				if (job != NULL) {
					TRACE("Updated default port of untracked team %d, %s\n",
						(int)team, signature);
				}
			}

			if (job != NULL) {
				// Update port info
				app_info info;
				status_t status = be_roster->GetRunningAppInfo(team, &info);
				if (status == B_OK && info.port != job->DefaultPort()) {
					TRACE("Update default port for %s to %d\n", job->Name(),
						(int)info.port);
					job->SetDefaultPort(info.port);
				}
			}
			break;
		}

		case B_GET_LAUNCH_DATA:
			_HandleGetLaunchData(message);
			break;

		case B_LAUNCH_TARGET:
			_HandleLaunchTarget(message);
			break;
		case B_STOP_LAUNCH_TARGET:
			_HandleStopLaunchTarget(message);
			break;
		case B_LAUNCH_JOB:
			_HandleLaunchJob(message);
			break;
		case B_ENABLE_LAUNCH_JOB:
			_HandleEnableLaunchJob(message);
			break;
		case B_STOP_LAUNCH_JOB:
			_HandleStopLaunchJob(message);
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
		case B_RESET_STICKY_LAUNCH_EVENT:
			_HandleResetStickyLaunchEvent(message);
			break;

		case B_GET_LAUNCH_TARGETS:
			_HandleGetLaunchTargets(message);
			break;
		case B_GET_LAUNCH_TARGET_INFO:
			_HandleGetLaunchTargetInfo(message);
			break;
		case B_GET_LAUNCH_JOBS:
			_HandleGetLaunchJobs(message);
			break;
		case B_GET_LAUNCH_JOB_INFO:
			_HandleGetLaunchJobInfo(message);
			break;

		case B_GET_LAUNCH_LOG:
			_HandleGetLaunchLog(message);
			break;

		case kMsgEventTriggered:
		{
			// An internal event has been triggered.
			// Check if its job(s) can be launched now.
			const char* name = message->GetString("owner");
			if (name == NULL)
				break;

			Event* event = (Event*)message->GetPointer("event");

			Job* job = FindJob(name);
			if (job != NULL) {
				fLog.EventTriggered(job, event);
				_LaunchJob(job);
				break;
			}

			Target* target = FindTarget(name);
			if (target != NULL) {
				fLog.EventTriggered(target, event);
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
	} else if (job->IsService() && !job->IsLaunched()) {
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
	} else
		launchJob = false;

	bool ownsMessage = false;
	if (reply.what == B_OK) {
		// Launch the job if it hasn't been launched already
		if (launchJob)
			_LaunchJob(job, TRIGGER_DEMAND);

		DetachCurrentMessage();
		status_t result = job->HandleGetLaunchData(message);
		if (result == B_OK) {
			// Replying is delegated to the job.
			return;
		}

		ownsMessage = true;
		reply.what = result;
	}

	message->SendReply(&reply);
	if (ownsMessage)
		delete message;
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

	BMessage reply((uint32)B_OK);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleStopLaunchTarget(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("target");

	Target* target = FindTarget(name);
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

	bool force = message->GetBool("force");
	fLog.JobStopped(target, force);
	_StopJobs(target, force);

	BMessage reply((uint32)B_OK);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleLaunchJob(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("name");

	Job* job = FindJob(name);
	if (job == NULL) {
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

	job->SetEnabled(true);
	_LaunchJob(job, FORCE_NOW);

	BMessage reply((uint32)B_OK);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleEnableLaunchJob(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("name");
	bool enable = message->GetBool("enable");

	Job* job = FindJob(name);
	if (job == NULL) {
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

	job->SetEnabled(enable);
	fLog.JobEnabled(job, enable);

	BMessage reply((uint32)B_OK);
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleStopLaunchJob(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("name");

	Job* job = FindJob(name);
	if (job == NULL) {
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

	bool force = message->GetBool("force");
	fLog.JobStopped(job, force);
	_StopJob(job, force);

	BMessage reply((uint32)B_OK);
	message->SendReply(&reply);
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
		uint32 flags = message->GetUInt32("flags", 0);
		BMessenger source;
		if (name != NULL && ownerName != NULL
			&& message->FindMessenger("source", &source) == B_OK) {
			// Register event
			ownerName = get_leaf(ownerName);

			ExternalEventSource* event = new (std::nothrow)
				ExternalEventSource(source, ownerName, name, flags);
			if (event != NULL) {
				// Use short name, and fully qualified name
				BString eventName = name;
				fEvents.insert(std::make_pair(eventName, event));
				_ResolveExternalEvents(event, eventName);

				eventName.Prepend("/");
				eventName.Prepend(ownerName);
				fEvents.insert(std::make_pair(eventName, event));
				_ResolveExternalEvents(event, eventName);

				fLog.ExternalEventRegistered(name);
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

			fLog.ExternalEventRegistered(name);
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

		ExternalEventSource* event = _FindExternalEventSource(ownerName, name);
		if (event != NULL) {
			fLog.ExternalEventTriggered(name);
			event->Trigger();
		}
	}

	_ForwardEventMessage(user, message);
}


void
LaunchDaemon::_HandleResetStickyLaunchEvent(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	if (user == 0 || fUserMode) {
		// Reset sticky events
		const char* name = message->GetString("name");
		const char* ownerName = message->GetString("owner");
		// TODO: support arguments (as selectors)

		ExternalEventSource* eventSource = _FindExternalEventSource(ownerName, name);
		if (eventSource != NULL)
			eventSource->ResetSticky();
	}

	_ForwardEventMessage(user, message);
}


void
LaunchDaemon::_HandleGetLaunchTargets(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	BMessage reply;
	status_t status = B_OK;

	if (!fUserMode) {
		// Request the data from the user's daemon, too
		Session* session = FindSession(user);
		if (session != NULL) {
			BMessage request(B_GET_LAUNCH_TARGETS);
			status = request.AddInt32("user", 0);
			if (status == B_OK) {
				status = session->Daemon().SendMessage(&request,
					&reply);
			}
			if (status == B_OK)
				status = reply.what;
		} else
			status = B_NAME_NOT_FOUND;
	}

	if (status == B_OK) {
		TargetMap::const_iterator iterator = fTargets.begin();
		for (; iterator != fTargets.end(); iterator++)
			reply.AddString("target", iterator->first);
	}

	reply.what = status;
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleGetLaunchTargetInfo(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("name");
	Target* target = FindTarget(name);
	if (target == NULL && !fUserMode) {
		_ForwardEventMessage(user, message);
		return;
	}

	BMessage info(uint32(target != NULL ? B_OK : B_NAME_NOT_FOUND));
	if (target != NULL) {
		_GetBaseJobInfo(target, info);

		for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
				iterator++) {
			Job* job = iterator->second;
			if (job->Target() == target)
				info.AddString("job", job->Name());
		}
	}
	message->SendReply(&info);
}


void
LaunchDaemon::_HandleGetLaunchJobs(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* targetName = message->GetString("target");

	BMessage reply;
	status_t status = B_OK;

	if (!fUserMode) {
		// Request the data from the user's daemon, too
		Session* session = FindSession(user);
		if (session != NULL) {
			BMessage request(B_GET_LAUNCH_JOBS);
			status = request.AddInt32("user", 0);
			if (status == B_OK && targetName != NULL)
				status = request.AddString("target", targetName);
			if (status == B_OK) {
				status = session->Daemon().SendMessage(&request,
					&reply);
			}
			if (status == B_OK)
				status = reply.what;
		} else
			status = B_NAME_NOT_FOUND;
	}

	if (status == B_OK) {
		JobMap::const_iterator iterator = fJobs.begin();
		for (; iterator != fJobs.end(); iterator++) {
			Job* job = iterator->second;
			if (targetName != NULL && (job->Target() == NULL
					|| job->Target()->Title() != targetName)) {
				continue;
			}
			reply.AddString("job", iterator->first);
		}
	}

	reply.what = status;
	message->SendReply(&reply);
}


void
LaunchDaemon::_HandleGetLaunchJobInfo(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	const char* name = message->GetString("name");
	Job* job = FindJob(name);
	if (job == NULL && !fUserMode) {
		_ForwardEventMessage(user, message);
		return;
	}

	BMessage info(uint32(job != NULL ? B_OK : B_NAME_NOT_FOUND));
	if (job != NULL) {
		_GetBaseJobInfo(job, info);

		info.SetInt32("team", job->Team());
		info.SetBool("enabled", job->IsEnabled());
		info.SetBool("running", job->IsRunning());
		info.SetBool("launched", job->IsLaunched());
		info.SetBool("service", job->IsService());

		if (job->Target() != NULL)
			info.SetString("target", job->Target()->Name());

		for (int32 i = 0; i < job->Arguments().CountStrings(); i++)
			info.AddString("launch", job->Arguments().StringAt(i));

		for (int32 i = 0; i < job->Requirements().CountStrings(); i++)
			info.AddString("requires", job->Requirements().StringAt(i));

		PortMap::const_iterator iterator = job->Ports().begin();
		for (; iterator != job->Ports().end(); iterator++)
			info.AddMessage("port", &iterator->second);
	}
	message->SendReply(&info);
}


void
LaunchDaemon::_HandleGetLaunchLog(BMessage* message)
{
	uid_t user = _GetUserID(message);
	if (user < 0)
		return;

	BMessage filter;
	BString jobName;
	const char* event = NULL;
	int32 limit = 0;
	bool systemOnly = false;
	bool userOnly = false;
	if (message->FindMessage("filter", &filter) == B_OK) {
		limit = filter.GetInt32("limit", 0);
		jobName = filter.GetString("job");
		jobName.ToLower();
		event = filter.GetString("event");
		systemOnly = filter.GetBool("systemOnly");
		userOnly = filter.GetBool("userOnly");
	}

	BMessage info((uint32)B_OK);
	int32 count = 0;

	if (user == 0 || !userOnly) {
		LogItemList::Iterator iterator = fLog.Iterator();
		while (iterator.HasNext()) {
			LogItem* item = iterator.Next();
			if (!item->Matches(jobName.IsEmpty() ? NULL : jobName.String(),
					event)) {
				continue;
			}

			BMessage itemMessage;
			itemMessage.AddUInt64("when", item->When());
			itemMessage.AddInt32("type", (int32)item->Type());
			itemMessage.AddString("message", item->Message());

			BMessage parameter;
			item->GetParameter(parameter);
			itemMessage.AddMessage("parameter", &parameter);

			info.AddMessage("item", &itemMessage);

			// limit == 0 means no limit
			if (++count == limit)
				break;
		}
	}

	// Get the list from the user daemon, and merge it into our reply
	Session* session = FindSession(user);
	if (session != NULL && !systemOnly) {
		if (limit != 0) {
			// Update limit for user daemon
			limit -= count;
			if (limit <= 0) {
				message->SendReply(&info);
				return;
			}
		}

		BMessage reply;

		BMessage request(B_GET_LAUNCH_LOG);
		status_t status = request.AddInt32("user", 0);
		if (status == B_OK && (limit != 0 || !jobName.IsEmpty()
				|| event != NULL)) {
			// Forward filter specification when needed
			status = filter.SetInt32("limit", limit);
			if (status == B_OK)
				status = request.AddMessage("filter", &filter);
		}
		if (status == B_OK)
			status = session->Daemon().SendMessage(&request, &reply);
		if (status == B_OK)
			info.AddMessage("user", &reply);
	}

	message->SendReply(&info);
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
		TRACE("launch_daemon: read file %s\n", path.Path());
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

		if (target->Event() != NULL)
			target->Event()->Register(*this);
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
		TRACE("  add job \"%s\"\n", name.String());

		job = new (std::nothrow) Job(name);
		if (job == NULL)
			return;

		job->SetTeamListener(this);
		job->SetService(service);
		job->SetCreateDefaultPort(service);
		job->SetTarget(target);
	} else
		TRACE("  amend job \"%s\"\n", name.String());

	if (message.HasBool("disabled")) {
		job->SetEnabled(!message.GetBool("disabled", !job->IsEnabled()));
		fLog.JobEnabled(job, job->IsEnabled());
	}

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

	if (message.HasString("launch"))
		message.FindStrings("launch", &job->Arguments());

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
				if (status == B_OK && job->Event() != NULL)
					status = job->Event()->Register(*this);
			}
		}

		if (status == B_OK) {
			fLog.JobInitialized(job);
		} else {
			if (status != B_NO_INIT) {
				// TODO: log error
				debug_printf("Init \"%s\" failed: %s\n", job->Name(),
					strerror(status));
			}
			fLog.JobIgnored(job, status);

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


/*!	Stops all running jobs of the specified target (may be \c NULL).
*/
void
LaunchDaemon::_StopJobs(Target* target, bool force)
{
	if (target != NULL && !target->HasLaunched())
		return;

	for (JobMap::reverse_iterator iterator = fJobs.rbegin();
			iterator != fJobs.rend(); iterator++) {
		Job* job = iterator->second;
		if (job->Target() == target)
			_StopJob(job, force);
	}
}


/*!	Checks whether or not the specified \a job can be launched.
	If \a testOnly is \c false, calling this method will trigger a demand
	to the \a job.
*/
bool
LaunchDaemon::_CanLaunchJob(Job* job, uint32 options, bool testOnly)
{
	if (job == NULL || !job->CanBeLaunched())
		return false;

	return (options & FORCE_NOW) != 0
		|| (job->EventHasTriggered() && job->CheckCondition(*this)
			&& ((options & TRIGGER_DEMAND) == 0
				|| Events::TriggerDemand(job->Event(), testOnly)));
}


/*!	Checks recursively if the requirements of the specified job can be launched,
	if they are not running already.
	Calling this method will not trigger a demand for the requirements.
*/
bool
LaunchDaemon::_CanLaunchJobRequirements(Job* job, uint32 options)
{
	int32 count = job->Requirements().CountStrings();
	for (int32 index = 0; index < count; index++) {
		Job* requirement = FindJob(job->Requirements().StringAt(index));
		if (requirement != NULL
			&& !requirement->IsRunning() && !requirement->IsLaunching()
			&& (!_CanLaunchJob(requirement, options, true)
				|| _CanLaunchJobRequirements(requirement, options))) {
			requirement->AddPending(job->Name());
			return false;
		}
	}

	return true;
}


/*!	Adds the specified \a job to the launch queue
	queue, except those that are triggered by events.

	Unless \c FORCE_NOW is set, the target is only launched if its events,
	if any, have been triggered already.

	Calling this method will trigger a demand event if \c TRIGGER_DEMAND has
	been set.
*/
bool
LaunchDaemon::_LaunchJob(Job* job, uint32 options)
{
	if (job != NULL && (job->IsLaunching() || job->IsRunning()))
		return true;

	if (!_CanLaunchJob(job, options))
		return false;

	// Test if we can launch all requirements
	if (!_CanLaunchJobRequirements(job, options | TRIGGER_DEMAND))
		return false;

	// Actually launch the requirements
	int32 count = job->Requirements().CountStrings();
	for (int32 index = 0; index < count; index++) {
		Job* requirement = FindJob(job->Requirements().StringAt(index));
		if (requirement != NULL) {
			// TODO: For jobs that have their communication channels set up,
			// we would not need to trigger demand at this point
			if (!_LaunchJob(requirement, options | TRIGGER_DEMAND)) {
				// Failed to put a requirement into the launch queue
				return false;
			}
		}
	}

	if (job->Target() != NULL)
		job->Target()->ResolveSourceFiles();
	if (job->Event() != NULL)
		job->Event()->ResetTrigger();

	job->SetLaunching(true);

	status_t status = fJobQueue.AddJob(job);
	if (status != B_OK) {
		debug_printf("Adding job %s to queue failed: %s\n", job->Name(),
			strerror(status));
		return false;
	}

	// Try to launch pending jobs as well
	count = job->Pending().CountStrings();
	for (int32 index = 0; index < count; index++) {
		Job* pending = FindJob(job->Pending().StringAt(index));
		if (pending != NULL && _LaunchJob(pending, 0)) {
			// Remove the job from the pending list once its in the launch
			// queue, so that is not being launched again next time.
			index--;
			count--;
		}
	}

	return true;
}


void
LaunchDaemon::_StopJob(Job* job, bool force)
{
	// TODO: find out which jobs require this job, and don't stop if any,
	// unless force, and then stop them all.
	job->SetEnabled(false);

	if (!job->IsRunning())
		return;

	// Be nice first, and send a simple quit message
	BMessenger messenger;
	if (job->GetMessenger(messenger) == B_OK) {
		BMessage request(B_QUIT_REQUESTED);
		messenger.SendMessage(&request);

		// TODO: wait a bit before going further
		return;
	}
	// TODO: allow custom shutdown

	send_signal(-job->Team(), SIGINT);
	// TODO: this would be the next step, again, after a delay
	//send_signal(job->Team(), SIGKILL);
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
		event = Events::AddOnDemand(this, event);
		updated = true;
	}

	if (updated) {
		TRACE("    event: %s\n", event->ToString().String());
		job->SetEvent(event);
	}
}


void
LaunchDaemon::_SetEnvironment(BaseJob* job, const BMessage& message)
{
	BMessage environmentMessage;
	if (message.FindMessage("env", &environmentMessage) == B_OK)
		job->SetEnvironment(environmentMessage);
}


ExternalEventSource*
LaunchDaemon::_FindExternalEventSource(const char* owner, const char* name) const
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
LaunchDaemon::_ResolveExternalEvents(ExternalEventSource* eventSource,
	const BString& name)
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Event* externalEvent = Events::ResolveExternalEvent(iterator->second->Event(),
			name, eventSource->Flags());
		if (externalEvent != NULL)
			eventSource->AddDestination(externalEvent);
	}
}


void
LaunchDaemon::_GetBaseJobInfo(BaseJob* job, BMessage& info)
{
	info.SetString("name", job->Name());

	if (job->Event() != NULL)
		info.SetString("event", job->Event()->ToString());

	if (job->Condition() != NULL)
		info.SetString("condition", job->Condition()->ToString());
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

		if (passwd->pw_dir != NULL && passwd->pw_dir[0] != '\0') {
			setenv("HOME", passwd->pw_dir, true);

			if (chdir(passwd->pw_dir) != 0) {
				debug_printf("Could not switch to home dir %s: %s\n",
					passwd->pw_dir, strerror(errno));
			}
		}

		// TODO: This leaks the parent application
		be_app = NULL;

		// Reinitialize be_roster
		BRoster::Private().DeleteBeRoster();
		BRoster::Private().InitBeRoster();

		// TODO: take over system jobs, and reserve their names (or ask parent)
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
	setenv("SAFEMODE", IsSafeMode() ? "yes" : "no", true);

	// Default locale settings
	setenv("LC_TYPE", "en_US.UTF-8", true);
}


/*!	Basic system initialization that must happen before any jobs are launched.
*/
void
LaunchDaemon::_InitSystem()
{
#ifndef TEST_MODE
	_AddInitJob(new InitRealTimeClockJob());
	_AddInitJob(new InitSharedMemoryDirectoryJob());
	_AddInitJob(new InitTemporaryDirectoryJob());
#endif

	fJobQueue.AddJob(fInitTarget);
}


void
LaunchDaemon::_AddInitJob(BJob* job)
{
	fInitTarget->AddDependency(job);
	fJobQueue.AddJob(job);
}


// #pragma mark -


#ifndef TEST_MODE


static void
open_stdio(int targetFD, int openMode)
{
#ifdef DEBUG
	int fd = open("/dev/dprintf", openMode);
#else
	int fd = open("/dev/null", openMode);
#endif
	if (fd != targetFD) {
		dup2(fd, targetFD);
		close(fd);
	}
}


#endif	// TEST_MODE


int
main()
{
	if (find_port(B_LAUNCH_DAEMON_PORT_NAME) >= 0) {
		fprintf(stderr, "The launch_daemon is already running!\n");
		return EXIT_FAILURE;
	}

#ifndef TEST_MODE
	// Make stdin/out/err available
	open_stdio(STDIN_FILENO, O_RDONLY);
	open_stdio(STDOUT_FILENO, O_WRONLY);
	dup2(STDOUT_FILENO, STDERR_FILENO);
#endif

	EventMap events;
	status_t status;
	LaunchDaemon* daemon = new LaunchDaemon(false, events, status);
	if (status == B_OK)
		daemon->Run();

	delete daemon;
	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
