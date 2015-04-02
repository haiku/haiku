/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <map>

#include <stdio.h>
#include <stdlib.h>

#include <Directory.h>
#include <driver_settings.h>
#include <Entry.h>
#include <ObjectList.h>
#include <Path.h>
#include <PathFinder.h>
#include <Server.h>

#include <DriverSettingsMessageAdapter.h>
#include <LaunchDaemonDefs.h>
#include <syscalls.h>


using namespace BPrivate;


static const char* kLaunchDirectory = "launch";


const static settings_template kJobTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_BOOL_TYPE, "disabled", NULL},
	{B_STRING_TYPE, "launch", NULL},
	{B_BOOL_TYPE, "create_port", NULL},
	{B_BOOL_TYPE, "no_safemode", NULL},
	{0, NULL, NULL}
};

const static settings_template kSettingsTemplate[] = {
	{B_MESSAGE_TYPE, "job", kJobTemplate},
	{B_MESSAGE_TYPE, "service", kJobTemplate},
	{0, NULL, NULL}
};


class Job {
public:
								Job(const char* name);
	virtual						~Job();

			const char*			Name() const;

			bool				IsEnabled() const;
			void				SetEnabled(bool enable);

			bool				IsService() const;
			void				SetService(bool service);

			bool				CreatePort() const;
			void				SetCreatePort(bool createPort);

			bool				LaunchInSafeMode() const;
			void				SetLaunchInSafeMode(bool launch);

			const BStringList&	Arguments() const;
			BStringList&		Arguments();
			void				AddArgument(const char* argument);

			status_t			Init();
			status_t			InitCheck() const;

			port_id				Port() const;

			status_t			Launch();
			bool				IsLaunched() const;

private:
			BString				fName;
			BStringList			fArguments;
			bool				fEnabled;
			bool				fService;
			bool				fCreatePort;
			bool				fLaunchInSafeMode;
			port_id				fPort;
			status_t			fInitStatus;
			team_id				fTeam;
};


typedef std::map<BString, Job*> JobMap;


class LaunchDaemon : public BServer {
public:
								LaunchDaemon(status_t& error);
	virtual						~LaunchDaemon();

	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_ReadPaths(const BStringList& paths);
			void				_ReadEntry(const char* context, BEntry& entry);
			void				_ReadDirectory(const char* context,
									BEntry& directory);
			status_t			_ReadFile(const char* context, BEntry& entry);

			void				_AddJob(bool service, BMessage& message);
			Job*				_Job(const char* name);
			void				_InitJobs();
			void				_LaunchJobs();

			void				_SetupEnvironment();
			bool				_IsSafeMode() const;

private:
			JobMap				fJobs;
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


Job::Job(const char* name)
	:
	fName(name),
	fEnabled(true),
	fService(false),
	fCreatePort(false),
	fLaunchInSafeMode(true),
	fPort(-1),
	fInitStatus(B_NO_INIT),
	fTeam(-1)
{
}


Job::~Job()
{
	if (fPort >= 0)
		delete_port(fPort);
}


const char*
Job::Name() const
{
	return fName.String();
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
Job::CreatePort() const
{
	return fCreatePort;
}


void
Job::SetCreatePort(bool createPort)
{
	fCreatePort = createPort;
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


status_t
Job::Init()
{
	fInitStatus = B_OK;

	if (fCreatePort) {
		// TODO: prefix system ports with "system:"
		fPort = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY, Name());
		if (fPort < 0)
			fInitStatus = fPort;

		printf("PORT %s: %s\n", Name(), strerror(fPort));
	}

	return fInitStatus;
}


status_t
Job::InitCheck() const
{
	return fInitStatus;
}


port_id
Job::Port() const
{
	return fPort;
}


status_t
Job::Launch()
{
	if (fArguments.IsEmpty()) {
		// TODO: launch via signature
	} else {
		printf("LAUNCH %s\n", fArguments.StringAt(0).String());
	}
	return B_OK;
}


bool
Job::IsLaunched() const
{
	return fTeam >= 0;
}


// #pragma mark -


LaunchDaemon::LaunchDaemon(status_t& error)
	:
	BServer(kLaunchDaemonSignature, NULL,
		create_port(B_LOOPER_PORT_DEFAULT_CAPACITY,
			B_LAUNCH_DAEMON_PORT_NAME), false, &error)
{
}


LaunchDaemon::~LaunchDaemon()
{
}


void
LaunchDaemon::ReadyToRun()
{
	_SetupEnvironment();

	BStringList paths;
	BPathFinder::FindPaths(B_FIND_PATH_DATA_DIRECTORY, kLaunchDirectory,
		B_FIND_PATHS_SYSTEM_ONLY, paths);
	_ReadPaths(paths);

	BPathFinder::FindPaths(B_FIND_PATH_SETTINGS_DIRECTORY, kLaunchDirectory,
		B_FIND_PATHS_SYSTEM_ONLY, paths);
	_ReadPaths(paths);

	_InitJobs();
	_LaunchJobs();
}


void
LaunchDaemon::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_GET_LAUNCH_CONNECTIONS:
		{
			BMessage reply;
			Job* job = _Job(get_leaf(message->GetString("name")));
			if (job == NULL) {
				reply.AddInt32("error", B_NAME_NOT_FOUND);
			} else {
				if (job->CreatePort())
					reply.AddInt32("port", job->Port());

				// Launch job now if it isn't running yet
				if (!job->IsLaunched())
					job->Launch();
			}
			message->SendReply(&reply);
			break;
		}

		default:
			BServer::MessageReceived(message);
			break;
	}
}


void
LaunchDaemon::_ReadPaths(const BStringList& paths)
{
	for (int32 i = 0; i < paths.CountStrings(); i++) {
		printf("----- %s -------\n", paths.StringAt(i).String());
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
	printf("DIR %s\n", directoryEntry.Name());

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		_ReadEntry(context, entry);
	}
}


status_t
LaunchDaemon::_ReadFile(const char* context, BEntry& entry)
{
	printf("FILE %s\n", entry.Name());
	DriverSettingsMessageAdapter adapter;

	BPath path;
	status_t status = path.SetTo(&entry);
	if (status != B_OK)
		return status;

	BMessage message;
	status = adapter.ConvertFromDriverSettings(path.Path(), kSettingsTemplate,
			message);
	if (status == B_OK) {
		message.PrintToStream();
		BMessage job;
		for (int32 index = 0; message.FindMessage("service", index,
				&job) == B_OK; index++) {
			_AddJob(false, job);
		}

		for (int32 index = 0; message.FindMessage("job", index, &job) == B_OK;
				index++) {
			_AddJob(false, job);
		}
	}

	return status;
}


void
LaunchDaemon::_AddJob(bool service, BMessage& message)
{
	const char* name = message.GetString("name");
printf("JOB %s\n", name);
	if (name == NULL || name[0] == '\0') {
		// Invalid job description
		return;
	}

	Job* job = _Job(name);
	if (job == NULL)
		job = new Job(name);

	job->SetEnabled(!message.GetBool("disabled", !job->IsEnabled()));
	job->SetService(service);
	job->SetCreatePort(message.GetBool("create_port", job->CreatePort()));
	job->SetLaunchInSafeMode(
		!message.GetBool("no_safemode", !job->LaunchInSafeMode()));

	const char* argument;
	for (int32 index = 0;
			message.FindString("launch", index, &argument) == B_OK; index++) {
		job->AddArgument(argument);
	}

	fJobs.insert(std::pair<BString, Job*>(job->Name(), job));
}


Job*
LaunchDaemon::_Job(const char* name)
{
	if (name == NULL)
		return NULL;

	JobMap::const_iterator found = fJobs.find(name);
	if (found != fJobs.end())
		return found->second;

	return NULL;
}


void
LaunchDaemon::_InitJobs()
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Job* job = iterator->second;
printf("  enabled? %s - %d\n", job->Name(), job->IsEnabled());
		if (job->IsEnabled() && (!_IsSafeMode() || job->LaunchInSafeMode()))
			job->Init();
	}
}


void
LaunchDaemon::_LaunchJobs()
{
	for (JobMap::iterator iterator = fJobs.begin(); iterator != fJobs.end();
			iterator++) {
		Job* job = iterator->second;
		if (job->IsEnabled() && job->InitCheck() == B_OK)
			job->Launch();
	}
}


void
LaunchDaemon::_SetupEnvironment()
{
	// Determine safemode kernel option
	BString safemode = "SAFEMODE=";
	safemode << _IsSafeMode() ? "yes" : "no";
}


bool
LaunchDaemon::_IsSafeMode() const
{
	char buffer[32];
	size_t size = sizeof(buffer);
	status_t status = _kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, buffer,
		&size);
	if (status == B_OK) {
		return !strncasecmp(buffer, "true", size)
			|| !strncasecmp(buffer, "yes", size)
			|| !strncasecmp(buffer, "on", size)
			|| !strncasecmp(buffer, "enabled", size);
	}

	return false;
}


// #pragma mark -


int
main()
{
	status_t status;
	LaunchDaemon* daemon = new LaunchDaemon(status);
	if (status == B_OK)
		daemon->Run();

	delete daemon;
	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
