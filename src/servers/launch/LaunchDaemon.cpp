/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <map>

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
#include <DriverSettingsMessageAdapter.h>
#include <JobQueue.h>
#include <LaunchDaemonDefs.h>
#include <syscalls.h>

#include "InitRealTimeClockJob.h"
#include "InitSharedMemoryDirectoryJob.h"
#include "InitTemporaryDirectoryJob.h"


using namespace BPrivate;
using namespace BSupportKit;
using BSupportKit::BPrivate::JobQueue;


static const char* kLaunchDirectory = "launch";


const static settings_template kPortTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_INT32_TYPE, "capacity", NULL},
};

const static settings_template kJobTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_BOOL_TYPE, "disabled", NULL},
	{B_STRING_TYPE, "launch", NULL},
	{B_BOOL_TYPE, "legacy", NULL},
	{B_MESSAGE_TYPE, "port", kPortTemplate},
	{B_BOOL_TYPE, "no_safemode", NULL},
	{0, NULL, NULL}
};

const static settings_template kSettingsTemplate[] = {
	{B_MESSAGE_TYPE, "job", kJobTemplate},
	{B_MESSAGE_TYPE, "service", kJobTemplate},
	{0, NULL, NULL}
};


typedef std::map<BString, BMessage> PortMap;


class Job {
public:
								Job(const char* name);
	virtual						~Job();

			const char*			Name() const;

			bool				IsEnabled() const;
			void				SetEnabled(bool enable);

			bool				IsService() const;
			void				SetService(bool service);

			bool				CreateDefaultPort() const;
			void				SetCreateDefaultPort(bool createPort);

			void				AddPort(BMessage& data);

			bool				LaunchInSafeMode() const;
			void				SetLaunchInSafeMode(bool launch);

			const BStringList&	Arguments() const;
			BStringList&		Arguments();
			void				AddArgument(const char* argument);

			status_t			Init();
			status_t			InitCheck() const;

			team_id				Team() const;

			const PortMap&		Ports() const;
			port_id				Port(const char* name = NULL) const;

			status_t			Launch();
			bool				IsLaunched() const;

private:
			BString				fName;
			BStringList			fArguments;
			bool				fEnabled;
			bool				fService;
			bool				fCreateDefaultPort;
			bool				fLaunchInSafeMode;
			PortMap				fPortMap;
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

			void				_RetrieveKernelOptions();
			void				_SetupEnvironment();
			void				_InitSystem();

			bool				_IsSafeMode() const;

private:
			JobMap				fJobs;
			JobQueue			fJobQueue;
			bool				fSafeMode;
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
	fCreateDefaultPort(false),
	fLaunchInSafeMode(true),
	fInitStatus(B_NO_INIT),
	fTeam(-1)
{
	fName.ToLower();
}


Job::~Job()
{
	PortMap::const_iterator iterator = Ports().begin();
	for (; iterator != Ports().end(); iterator++) {
		port_id port = iterator->second.GetInt32("port", -1);
		if (port >= 0)
			delete_port(port);
	}
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


status_t
Job::Init()
{
	fInitStatus = B_OK;

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
		if (port < 0)
			fInitStatus = port;
		else {
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
		// TODO: Launch via signature
		// We cannot use the BRoster here as it tries to pre-register
		// the application.
		BString signature("application/");
		signature << fName;
		return B_NOT_SUPPORTED;
		//return be_roster->Launch(signature.String(), (BMessage*)NULL, &fTeam);
	}

	entry_ref ref;
	status_t status = get_ref_for_path(fArguments.StringAt(0).String(), &ref);
	if (status != B_OK)
		return status;

	size_t count = fArguments.CountStrings();
	const char* args[count + 1];
	for (int32 i = 0; i < fArguments.CountStrings(); i++) {
		args[i] = fArguments.StringAt(i);
	}
	args[count] = NULL;

	thread_id thread = load_image(count, args,
		const_cast<const char**>(environ));
	if (thread >= 0)
		resume_thread(thread);

	thread_info info;
	if (get_thread_info(thread, &info) == B_OK)
		fTeam = info.team;
	return B_OK;
//	return be_roster->Launch(&ref, count, args, &fTeam);
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
	_RetrieveKernelOptions();
	_SetupEnvironment();
	_InitSystem();

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
		case B_GET_LAUNCH_DATA:
		{
			BMessage reply((uint32)B_OK);
			Job* job = _Job(get_leaf(message->GetString("name")));
			if (job == NULL) {
				reply.what = B_NAME_NOT_FOUND;
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
			_AddJob(true, job);
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
	if (name == NULL || name[0] == '\0') {
		// Invalid job description
		return;
	}

	Job* job = _Job(name);
	if (job == NULL)
		job = new Job(name);

	job->SetEnabled(!message.GetBool("disabled", !job->IsEnabled()));
	job->SetService(service);
	job->SetCreateDefaultPort(!message.GetBool("legacy", !service));
	job->SetLaunchInSafeMode(
		!message.GetBool("no_safemode", !job->LaunchInSafeMode()));

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

	fJobs.insert(std::pair<BString, Job*>(job->Name(), job));
}


Job*
LaunchDaemon::_Job(const char* name)
{
	if (name == NULL)
		return NULL;

	JobMap::const_iterator found = fJobs.find(BString(name).ToLower());
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
	safemode << _IsSafeMode() ? "yes" : "no";

	putenv(safemode.String());
}


/*!	Basic system initialization that must happen before any jobs are launched.
*/
void
LaunchDaemon::_InitSystem()
{
	fJobQueue.AddJob(new InitRealTimeClockJob());
	fJobQueue.AddJob(new InitSharedMemoryDirectoryJob());
	fJobQueue.AddJob(new InitTemporaryDirectoryJob());

	// TODO: these should be done in parallel
	while (BJob* job = fJobQueue.Pop())
		job->Run();
}


bool
LaunchDaemon::_IsSafeMode() const
{
	return fSafeMode;
}


// #pragma mark -


int
main()
{
	// TODO: remove this again
	close(STDOUT_FILENO);
	int fd = open("/dev/dprintf", O_WRONLY);
	if (fd != STDOUT_FILENO)
		dup2(fd, STDOUT_FILENO);
	puts("launch_daemon is alive and kicking.");
	fflush(stdout);

	status_t status;
	LaunchDaemon* daemon = new LaunchDaemon(status);
	if (status == B_OK)
		daemon->Run();

	delete daemon;
	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
