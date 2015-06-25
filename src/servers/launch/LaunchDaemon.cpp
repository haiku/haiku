/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


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
#include "InitRealTimeClockJob.h"
#include "InitSharedMemoryDirectoryJob.h"
#include "InitTemporaryDirectoryJob.h"
#include "Job.h"
#include "SettingsParser.h"
#include "Target.h"
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


typedef std::map<BString, Job*> JobMap;
typedef std::map<uid_t, Session*> SessionMap;
typedef std::map<BString, Target*> TargetMap;


class LaunchDaemon : public BServer, public Finder, public ConditionContext {
public:
								LaunchDaemon(bool userMode, status_t& error);
	virtual						~LaunchDaemon();

	virtual	Job*				FindJob(const char* name) const;
	virtual	Target*				FindTarget(const char* name) const;
			Session*			FindSession(uid_t user) const;

	virtual	bool				IsSafeMode() const;

	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
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
			void				_InitJobs();
			void				_LaunchJobs(Target* target);
			void				_AddLaunchJob(Job* job);
			void				_AddTarget(Target* target);
			void				_SetCondition(BaseJob* job,
									const BMessage& message);

			status_t			_StartSession(const char* login,
									const char* password);

			void				_RetrieveKernelOptions();
			void				_SetupEnvironment();
			void				_InitSystem();
			void				_AddInitJob(BJob* job);

private:
			JobMap				fJobs;
			TargetMap			fTargets;
			BStringList			fRunTargets;
			JobQueue			fJobQueue;
			SessionMap			fSessions;
			MainWorker*			fMainWorker;
			Target*				fInitTarget;
			bool				fSafeMode;
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


LaunchDaemon::LaunchDaemon(bool userMode, status_t& error)
	:
	BServer(kLaunchDaemonSignature, NULL,
		create_port(B_LOOPER_PORT_DEFAULT_CAPACITY,
			userMode ? "AppPort" : B_LAUNCH_DAEMON_PORT_NAME), false, &error),
	fInitTarget(userMode ? NULL : new Target("init")),
	fUserMode(userMode)
{
	fMainWorker = new MainWorker(fJobQueue);
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


void
LaunchDaemon::ReadyToRun()
{
	_RetrieveKernelOptions();
	_SetupEnvironment();
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

	_InitJobs();
	_LaunchJobs(NULL);

	// Launch run targets
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
		{
			uid_t user = _GetUserID(message);
			if (user < 0)
				return;

			BMessage reply((uint32)B_OK);
			Job* job = FindJob(get_leaf(message->GetString("name")));
			if (job == NULL) {
				Session* session = FindSession(user);
				if (session != NULL) {
					// Forward request to user launch_daemon
					if (session->Daemon().SendMessage(message) == B_OK)
						break;
				}
				reply.what = B_NAME_NOT_FOUND;
			} else if (!job->IsLaunched() && !job->CheckCondition(*this)) {
				// The job exists, but cannot be started yet, as its
				// conditions are not met; don't make it available yet
				// TODO: we may not want to initialize jobs with conditions
				// that aren't met yet
				reply.what = B_NO_INIT;
			} else {
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

				_AddLaunchJob(job);
			}
			message->SendReply(&reply);
			break;
		}

		case B_LAUNCH_TARGET:
		{
			uid_t user = _GetUserID(message);
			if (user < 0)
				break;

			const char* name = message->GetString("target");
			const char* baseName = message->GetString("base target");

			Target* target = FindTarget(name);
			if (target == NULL) {
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
						break;
				}

				BMessage reply(B_NAME_NOT_FOUND);
				message->SendReply(&reply);
				break;
			}

			BMessage data;
			if (message->FindMessage("data", &data) == B_OK)
				target->AddData(data.GetString("name"), data);

			_LaunchJobs(target);
			break;
		}

		case B_LAUNCH_SESSION:
		{
			uid_t user = _GetUserID(message);
			if (user < 0)
				break;

			status_t status = B_OK;
			const char* login = message->GetString("login");
			const char* password = message->GetString("password");
			if (login == NULL || password == NULL)
				status = B_BAD_VALUE;
			if (status == B_OK && user != 0) {
				// Only the root user can start sessions
				status = B_PERMISSION_DENIED;
			}
			if (status == B_OK)
				status = _StartSession(login, password);

			BMessage reply((uint32)status);
			message->SendReply(&reply);
			break;
		}

		case B_REGISTER_SESSION_DAEMON:
		{
			uid_t user = _GetUserID(message);
			if (user < 0)
				break;

			status_t status = B_OK;

			BMessenger target;
			if (message->FindMessenger("target", &target) != B_OK)
				status = B_BAD_VALUE;

			if (status == B_OK) {
				Session* session = new (std::nothrow) Session(user, target);
				if (session != NULL)
					fSessions.insert(std::pair<uid_t, Session*>(user, session));
				else
					status = B_NO_MEMORY;
			}

			BMessage reply((uint32)status);
			message->SendReply(&reply);
			break;
		}

		default:
			BServer::MessageReceived(message);
			break;
	}
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
	if (job == NULL)
		job = new Job(name);

	job->SetEnabled(!message.GetBool("disabled", !job->IsEnabled()));
	job->SetService(service);
	job->SetCreateDefaultPort(!message.GetBool("legacy", !service));
	job->SetTarget(target);

	_SetCondition(job, message);

	BMessage portMessage;
	for (int32 index = 0;
			message.FindMessage("port", index, &portMessage) == B_OK; index++) {
		job->AddPort(portMessage);
	}

	const char* argument;
	for (int32 index = 0;
			message.FindString("launch", index, &argument) == B_OK; index++) {
		job->AddArgument(argument);
	}

	const char* requirement;
	for (int32 index = 0;
			message.FindString("requires", index, &requirement) == B_OK;
			index++) {
		job->AddRequirement(requirement);
	}
	if (fInitTarget != NULL)
		job->AddRequirement(fInitTarget->Name());

	fJobs.insert(std::pair<BString, Job*>(job->Name(), job));
}


void
LaunchDaemon::_InitJobs()
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();) {
		Job* job = iterator->second;
		JobMap::iterator remove = iterator++;

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


void
LaunchDaemon::_LaunchJobs(Target* target)
{
	if (target != NULL && !target->CheckCondition(*this))
		return;

	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Job* job = iterator->second;
		if (job->Target() == target)
			_AddLaunchJob(job);
	}
}


void
LaunchDaemon::_AddLaunchJob(Job* job)
{
	if (!job->IsLaunched() && job->CheckCondition(*this))
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
	Condition* condition = NULL;

	BMessage conditions;
	if (message.FindMessage("if", &conditions) == B_OK)
		condition = Conditions::FromMessage(conditions);

	if (message.GetBool("no_safemode"))
		condition = Conditions::AddNotSafeMode(condition);

	if (condition != NULL)
		job->SetCondition(condition);
}


status_t
LaunchDaemon::_StartSession(const char* login, const char* password)
{
	Unlock();

	// TODO: enable user/group code and password authentication
	// The launch_daemon currently cannot talk to the registrar, though
/*
	struct passwd* passwd = getpwnam(login);
	if (passwd == NULL)
		return B_NAME_NOT_FOUND;
	if (strcmp(passwd->pw_name, login) != 0)
		return B_NAME_NOT_FOUND;

	// TODO: check for auto-login, and ignore password then
	if (!verify_password(passwd, getspnam(login), password))
		return B_PERMISSION_DENIED;

	// Check if there is a user session running already
	uid_t user = passwd->pw_uid;
	gid_t group = passwd->pw_gid;
*/

	if (fork() == 0) {
		if (setsid() < 0)
			exit(EXIT_FAILURE);

/*
debug_printf("session leader...\n");
		if (initgroups(login, group) == -1) {
debug_printf("1.ouch: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		//endgrent();
		if (setgid(group) != 0) {
debug_printf("2.ouch: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (setuid(user) != 0) {
debug_printf("3.ouch: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
*/

		// TODO: This leaks the parent application
		be_app = NULL;

		// TODO: take over system jobs, and reserve their names
		status_t status;
		LaunchDaemon* daemon = new LaunchDaemon(true, status);
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
	status_t status;
	LaunchDaemon* daemon = new LaunchDaemon(false, status);
	if (status == B_OK)
		daemon->Run();

	delete daemon;
	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
