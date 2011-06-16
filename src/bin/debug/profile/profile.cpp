/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <new>
#include <string>

#include <debugger.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <String.h>

#include <syscalls.h>
#include <system_profiler_defs.h>

#include <AutoDeleter.h>
#include <debug_support.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "BasicProfileResult.h"
#include "CallgrindProfileResult.h"
#include "debug_utils.h"
#include "Image.h"
#include "Options.h"
#include "SummaryProfileResult.h"
#include "Team.h"


// size of the sample buffer area for system profiling
#define PROFILE_ALL_SAMPLE_AREA_SIZE	(4 * 1024 * 1024)


extern const char* __progname;
const char* kCommandName = __progname;


class Image;
class Team;
class Thread;


static const char* kUsage =
	"Usage: %s [ <options> ] [ <command line> ]\n"
	"Profiles threads by periodically sampling the program counter. There are\n"
	"two different modes: One profiles the complete system. The other starts\n"
	"a program and profiles that and (optionally) its children. When a thread\n"
	"terminates, a list of the functions where the thread was encountered is\n"
	"printed.\n"
	"\n"
	"Options:\n"
	"  -a, --all      - Profile all teams.\n"
	"  -c             - Don't profile child threads. Default is to\n"
	"                   recursively profile all threads created by a profiled\n"
	"                   thread.\n"
	"  -C             - Don't profile child teams. Default is to recursively\n"
	"                   profile all teams created by a profiled team.\n"
	"  -f             - Always analyze the full caller stack. The hit count\n"
	"                   for every encountered function will be incremented.\n"
	"                   This increases the default for the caller stack depth\n"
	"                   (\"-s\") to 64.\n"
	"  -h, --help     - Print this usage info.\n"
	"  -i <interval>  - Use a tick interval of <interval> microseconds.\n"
	"                   Default is 1000 (1 ms). On a fast machine, a shorter\n"
	"                   interval might lead to better results, while it might\n"
	"                   make them worse on slow machines.\n"
	"  -k             - Don't check kernel images for hits.\n"
	"  -l             - Also profile loading the executable.\n"
	"  -o <output>    - Print the results to file <output>.\n"
	"  -r, --recorded - Don't profile, but evaluate a recorded kernel profile\n"
	"                   data.\n"
	"  -s <depth>     - Number of return address samples to take from the\n"
	"                   caller stack per tick. If the topmost address doesn't\n"
	"                   hit a known image, the next address will be matched\n"
	"                   (and so on).\n"
	"  -S             - Don't output results for individual threads, but\n"
	"                   produce a combined output at the end.\n"
	"  -v <directory> - Create valgrind/callgrind output. <directory> is the\n"
	"                   directory where to put the output files.\n"
;


Options gOptions;

static bool sCaughtDeadlySignal = false;


class ThreadManager : private ProfiledEntity {
public:
	ThreadManager(port_id debuggerPort)
		:
		fTeams(20),
		fThreads(20, true),
		fKernelTeam(NULL),
		fDebuggerPort(debuggerPort),
		fSummaryProfileResult(NULL)
	{
	}

	virtual ~ThreadManager()
	{
		// release image references
		for (ImageMap::iterator it = fImages.begin(); it != fImages.end(); ++it)
			it->second->ReleaseReference();

		if (fSummaryProfileResult != NULL)
			fSummaryProfileResult->ReleaseReference();

		for (int32 i = 0; Team* team = fTeams.ItemAt(i); i++)
			team->ReleaseReference();
	}

	status_t Init()
	{
		if (!gOptions.summary_result)
			return B_OK;

		ProfileResult* profileResult;
		status_t error = _CreateProfileResult(this, profileResult);
		if (error != B_OK)
			return error;

		BReference<ProfileResult> profileResultReference(profileResult, true);

		fSummaryProfileResult = new(std::nothrow) SummaryProfileResult(
			profileResult);
		if (fSummaryProfileResult == NULL)
			return B_NO_MEMORY;

		return fSummaryProfileResult->Init(profileResult->Entity());
	}

	status_t AddTeam(team_id teamID, Team** _team = NULL)
	{
		return _AddTeam(teamID, NULL, _team);
	}

	status_t AddTeam(system_profiler_team_added* addedInfo, Team** _team = NULL)
	{
		return _AddTeam(addedInfo->team, addedInfo, _team);
	}

	status_t AddThread(thread_id threadID)
	{
		thread_info threadInfo;
		status_t error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK)
			return error;

		return AddThread(threadInfo.team, threadID, threadInfo.name);
	}

	status_t AddThread(team_id teamID, thread_id threadID, const char* name)
	{
		if (FindThread(threadID) != NULL)
			return B_BAD_VALUE;

		Team* team = FindTeam(teamID);
		if (team == NULL)
			return B_BAD_TEAM_ID;

		Thread* thread = new(std::nothrow) Thread(threadID, name, team);
		if (thread == NULL)
			return B_NO_MEMORY;

		status_t error = _CreateThreadProfileResult(thread);
		if (error != B_OK) {
			delete thread;
			return error;
		}

		error = team->InitThread(thread);
		if (error != B_OK) {
			delete thread;
			return error;
		}

		fThreads.AddItem(thread);
		return B_OK;
	}

	void RemoveTeam(team_id teamID)
	{
		if (Team* team = FindTeam(teamID)) {
			if (team == fKernelTeam)
				fKernelTeam = NULL;
			fTeams.RemoveItem(team);
			team->ReleaseReference();
		}
	}

	void RemoveThread(thread_id threadID)
	{
		if (Thread* thread = FindThread(threadID)) {
			thread->GetTeam()->RemoveThread(thread);
			fThreads.RemoveItem(thread, true);
		}
	}

	Team* FindTeam(team_id teamID) const
	{
		for (int32 i = 0; Team* team = fTeams.ItemAt(i); i++) {
			if (team->ID() == teamID)
				return team;
		}
		return NULL;
	}

	Thread* FindThread(thread_id threadID) const
	{
		for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++) {
			if (thread->ID() == threadID)
				return thread;
		}
		return NULL;
	}

	int32 CountThreads() const
	{
		return fThreads.CountItems();
	}

	Thread* ThreadAt(int32 index) const
	{
		return fThreads.ItemAt(index);
	}

	status_t AddImage(team_id teamID, const image_info& imageInfo, int32 event)
	{
		// get a shared image
		SharedImage* sharedImage = NULL;
		status_t error = _GetSharedImage(teamID, imageInfo, &sharedImage);
		if (error != B_OK)
			return error;

		if (teamID == B_SYSTEM_TEAM) {
			// a kernel image -- add it to all teams
			int32 count = fTeams.CountItems();
			for (int32 i = 0; i < count; i++) {
				fTeams.ItemAt(i)->AddImage(sharedImage, imageInfo, teamID,
					event);
			}
		}

		// a userland team image -- add it to that image
		if (Team* team = FindTeam(teamID))
			return team->AddImage(sharedImage, imageInfo, teamID, event);

		return B_BAD_TEAM_ID;
	}

	void RemoveImage(team_id teamID, image_id imageID, int32 event)
	{
		if (teamID == B_SYSTEM_TEAM) {
			// a kernel image -- remove it from all teams
			int32 count = fTeams.CountItems();
			for (int32 i = 0; i < count; i++)
				fTeams.ItemAt(i)->RemoveImage(imageID, event);
		} else {
			// a userland team image -- add it to that image
			if (Team* team = FindTeam(teamID))
				team->RemoveImage(imageID, event);
		}
	}

	void PrintSummaryResults()
	{
		if (fSummaryProfileResult != NULL)
			fSummaryProfileResult->PrintSummaryResults();
	}

private:
	virtual int32 EntityID() const
	{
		return 1;
	}

	virtual const char* EntityName() const
	{
		return "all";
	}

	virtual const char* EntityType() const
	{
		return "summary";
	}

private:
	status_t _AddTeam(team_id teamID, system_profiler_team_added* addedInfo,
		Team** _team = NULL)
	{
		if (FindTeam(teamID) != NULL)
			return B_BAD_VALUE;

		Team* team = new(std::nothrow) Team;
		if (team == NULL)
			return B_NO_MEMORY;

		status_t error = addedInfo != NULL
			? _InitUndebuggedTeam(team, addedInfo)
			: _InitDebuggedTeam(team, teamID);
		if (error != B_OK) {
			team->ReleaseReference();
			return error;
		}

		fTeams.AddItem(team);

		if (teamID == B_SYSTEM_TEAM)
			fKernelTeam = team;

		if (_team != NULL)
			*_team = team;

		return B_OK;
	}

	status_t _InitDebuggedTeam(Team* team, team_id teamID)
	{
		// init the team
		status_t error = team->Init(teamID, fDebuggerPort);
		if (error != B_OK)
			return error;

		// add the team's images
		error = _LoadTeamImages(team, teamID);
		if (error != B_OK)
			return error;

		// add the kernel images
		return _LoadTeamImages(team, B_SYSTEM_TEAM);
	}

	status_t _InitUndebuggedTeam(Team* team,
		system_profiler_team_added* addedInfo)
	{
		// init the team
		status_t error = team->Init(addedInfo);
		if (error != B_OK)
			return error;

		// in case of a user team, add the kernel images
		if (team->ID() == B_SYSTEM_TEAM || fKernelTeam == NULL)
			return B_OK;

		const BObjectList<Image>& kernelImages = fKernelTeam->Images();
		int32 count = kernelImages.CountItems();
		for (int32 i = 0; i < count; i++) {
			SharedImage* sharedImage = kernelImages.ItemAt(i)->GetSharedImage();
			team->AddImage(sharedImage, sharedImage->Info(), B_SYSTEM_TEAM, 0);
		}

		return B_OK;
	}

	status_t _LoadTeamImages(Team* team, team_id teamID)
	{
		// iterate through the team's images and collect the symbols
		image_info imageInfo;
		int32 cookie = 0;
		while (get_next_image_info(teamID, &cookie, &imageInfo) == B_OK) {
			// get a shared image
			SharedImage* sharedImage;
			status_t error = _GetSharedImage(teamID, imageInfo, &sharedImage);
			if (error != B_OK)
				return error;

			// add the image to the team
			error = team->AddImage(sharedImage, imageInfo, teamID, 0);
			if (error != B_OK)
				return error;
		}

		return B_OK;
	}

	status_t _CreateThreadProfileResult(Thread* thread)
	{
		if (fSummaryProfileResult != NULL) {
			thread->SetProfileResult(fSummaryProfileResult);
			return B_OK;
		}

		ProfileResult* profileResult;
		status_t error = _CreateProfileResult(thread, profileResult);
		if (error != B_OK)
			return error;

		thread->SetProfileResult(profileResult);

		return B_OK;
	}

	status_t _CreateProfileResult(ProfiledEntity* profiledEntity,
		ProfileResult*& _profileResult)
	{
		ProfileResult* profileResult;

		if (gOptions.callgrind_directory != NULL)
			profileResult = new(std::nothrow) CallgrindProfileResult;
		else if (gOptions.analyze_full_stack)
			profileResult = new(std::nothrow) InclusiveProfileResult;
		else
			profileResult = new(std::nothrow) ExclusiveProfileResult;

		if (profileResult == NULL)
			return B_NO_MEMORY;

		BReference<ProfileResult> profileResultReference(profileResult, true);

		status_t error = profileResult->Init(profiledEntity);
		if (error != B_OK)
			return error;

		_profileResult = profileResultReference.Detach();
		return B_OK;
	}

	status_t _GetSharedImage(team_id teamID, const image_info& imageInfo,
		SharedImage** _sharedImage)
	{
		// check whether the image has already been loaded
		ImageMap::iterator it = fImages.find(imageInfo.name);
		if (it != fImages.end()) {
			*_sharedImage = it->second;
			return B_OK;
		}

		// create the shared image
		SharedImage* sharedImage = new(std::nothrow) SharedImage;
		if (sharedImage == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<SharedImage> imageDeleter(sharedImage);

		// load the symbols
		status_t error;
		if (teamID == B_SYSTEM_TEAM) {
			error = sharedImage->Init(teamID, imageInfo.id);
			if (error != B_OK) {
				// The image has obviously been unloaded already, try to get
				// it by path.
				BString name = imageInfo.name;
				if (name.FindFirst('/') == -1) {
					// modules without a path are likely to be boot modules
					BPath bootAddonPath;
					if (find_directory(B_SYSTEM_ADDONS_DIRECTORY,
							&bootAddonPath) == B_OK
						&& bootAddonPath.Append("kernel") == B_OK
						&& bootAddonPath.Append("boot") == B_OK) {
						name = BString(bootAddonPath.Path()) << "/" << name;
				}
				}

				error = sharedImage->Init(name.String());
			}
		} else
			error = sharedImage->Init(imageInfo.name);
		if (error != B_OK)
			return error;

		try {
			fImages[sharedImage->Name()] = sharedImage;
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}

		imageDeleter.Detach();
		*_sharedImage = sharedImage;
		return B_OK;
	}

private:
	typedef std::map<std::string, SharedImage*> ImageMap;

private:
	BObjectList<Team>				fTeams;
	BObjectList<Thread>				fThreads;
	ImageMap						fImages;
	Team*							fKernelTeam;
	port_id							fDebuggerPort;
	SummaryProfileResult*			fSummaryProfileResult;
};


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, __progname);
    exit(error ? 1 : 0);
}


/*
// get_id
static bool
get_id(const char *str, int32 &id)
{
	int32 len = strlen(str);
	for (int32 i = 0; i < len; i++) {
		if (!isdigit(str[i]))
			return false;
	}

	id = atol(str);
	return true;
}
*/


static bool
process_event_buffer(ThreadManager& threadManager, uint8* buffer,
	size_t bufferSize, team_id mainTeam)
{
//printf("process_event_buffer(%p, %lu)\n", buffer, bufferSize);
	const uint8* bufferEnd = buffer + bufferSize;

	while (buffer < bufferEnd) {
		system_profiler_event_header* header
			= (system_profiler_event_header*)buffer;

		buffer += sizeof(system_profiler_event_header);

		switch (header->event) {
			case B_SYSTEM_PROFILER_TEAM_ADDED:
			{
				system_profiler_team_added* event
					= (system_profiler_team_added*)buffer;

				threadManager.AddTeam(event);
				break;
			}

			case B_SYSTEM_PROFILER_TEAM_REMOVED:
			{
				system_profiler_team_removed* event
					= (system_profiler_team_removed*)buffer;

				threadManager.RemoveTeam(event->team);

				// quit, if the main team we're interested in is gone
				if (mainTeam >= 0 && event->team == mainTeam)
					return true;

				break;
			}

			case B_SYSTEM_PROFILER_TEAM_EXEC:
			{
				system_profiler_team_exec* event
					= (system_profiler_team_exec*)buffer;

				if (Team* team = threadManager.FindTeam(event->team))
					team->Exec(0, event->args, event->thread_name);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_ADDED:
			{
				system_profiler_thread_added* event
					= (system_profiler_thread_added*)buffer;

				threadManager.AddThread(event->team, event->thread,
					event->name);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_REMOVED:
			{
				system_profiler_thread_removed* event
					= (system_profiler_thread_removed*)buffer;

				if (Thread* thread = threadManager.FindThread(event->thread)) {
					thread->PrintResults();
					threadManager.RemoveThread(event->thread);
				}
				break;
			}

			case B_SYSTEM_PROFILER_IMAGE_ADDED:
			{
				system_profiler_image_added* event
					= (system_profiler_image_added*)buffer;

				threadManager.AddImage(event->team, event->info, 0);
				break;
			}

			case B_SYSTEM_PROFILER_IMAGE_REMOVED:
			{
				system_profiler_image_removed* event
					= (system_profiler_image_removed*)buffer;

 				threadManager.RemoveImage(event->team, event->image, 0);
				break;
			}

			case B_SYSTEM_PROFILER_SAMPLES:
			{
				system_profiler_samples* event
					= (system_profiler_samples*)buffer;

				Thread* thread = threadManager.FindThread(event->thread);
				if (thread != NULL) {
					thread->AddSamples(event->samples,
						(addr_t*)(buffer + header->size) - event->samples);
				}

				break;
			}

			case B_SYSTEM_PROFILER_BUFFER_END:
			{
				// Marks the end of the ring buffer -- we need to ignore the
				// remaining bytes.
				return false;
			}
		}

		buffer += header->size;
	}

	return false;
}


static void
signal_handler(int signal, void* data)
{
	sCaughtDeadlySignal = true;
}


static void
profile_all(const char* const* programArgs, int programArgCount)
{
	// Load the executable, if we have to.
	thread_id threadID = -1;
	if (programArgCount >= 1) {
		threadID = load_program(programArgs, programArgCount,
			gOptions.profile_loading);
		if (threadID < 0) {
			fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
				programArgs[0], strerror(threadID));
			exit(1);
		}
	}

	// install signal handlers so we can exit gracefully
    struct sigaction action;
    action.sa_handler = (__sighandler_t)signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_userdata = NULL;
    if (sigaction(SIGHUP, &action, NULL) < 0
		|| sigaction(SIGINT, &action, NULL) < 0
		|| sigaction(SIGQUIT, &action, NULL) < 0) {
		fprintf(stderr, "%s: Failed to install signal handlers: %s\n",
			kCommandName, strerror(errno));
		exit(1);
    }

	// create an area for the sample buffer
	system_profiler_buffer_header* bufferHeader;
	area_id area = create_area("profiling buffer", (void**)&bufferHeader,
		B_ANY_ADDRESS, PROFILE_ALL_SAMPLE_AREA_SIZE, B_NO_LOCK,
		B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		fprintf(stderr, "%s: Failed to create sample area: %s\n", kCommandName,
			strerror(area));
		exit(1);
	}

	uint8* bufferBase = (uint8*)(bufferHeader + 1);
	size_t totalBufferSize = PROFILE_ALL_SAMPLE_AREA_SIZE
		- (bufferBase - (uint8*)bufferHeader);

	// create a thread manager
	ThreadManager threadManager(-1);	// TODO: We don't need a debugger port!
	status_t error = threadManager.Init();
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to init thread manager: %s\n", kCommandName,
			strerror(error));
		exit(1);
	}

	// start profiling
	system_profiler_parameters profilerParameters;
	profilerParameters.buffer_area = area;
	profilerParameters.flags = B_SYSTEM_PROFILER_TEAM_EVENTS
		| B_SYSTEM_PROFILER_THREAD_EVENTS | B_SYSTEM_PROFILER_IMAGE_EVENTS
		| B_SYSTEM_PROFILER_SAMPLING_EVENTS;
	profilerParameters.interval = gOptions.interval;
	profilerParameters.stack_depth = gOptions.stack_depth;

	error = _kern_system_profiler_start(&profilerParameters);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to start profiling: %s\n", kCommandName,
			strerror(error));
		exit(1);
	}

	// resume the loaded team, if we have one
	if (threadID >= 0)
		resume_thread(threadID);

	// main event loop
	while (true) {
		// get the current buffer
		size_t bufferStart = bufferHeader->start;
		size_t bufferSize = bufferHeader->size;
		uint8* buffer = bufferBase + bufferStart;
//printf("processing buffer of size %lu bytes\n", bufferSize);

		bool quit;
		if (bufferStart + bufferSize <= totalBufferSize) {
			quit = process_event_buffer(threadManager, buffer, bufferSize,
				threadID);
		} else {
			size_t remainingSize = bufferStart + bufferSize - totalBufferSize;
			quit = process_event_buffer(threadManager, buffer,
					bufferSize - remainingSize, threadID)
				|| process_event_buffer(threadManager, bufferBase,
					remainingSize, threadID);
		}

		if (quit)
			break;

		// get next buffer
		uint64 droppedEvents = 0;
		error = _kern_system_profiler_next_buffer(bufferSize, &droppedEvents);

		if (error != B_OK) {
			if (error == B_INTERRUPTED) {
				if (sCaughtDeadlySignal)
					break;
				continue;
			}

			fprintf(stderr, "%s: Failed to get next sample buffer: %s\n",
				kCommandName, strerror(error));
			break;
		}
	}

	// stop profiling
	_kern_system_profiler_stop();

	// print results
	int32 threadCount = threadManager.CountThreads();
	for (int32 i = 0; i < threadCount; i++) {
		Thread* thread = threadManager.ThreadAt(i);
		thread->PrintResults();
	}

	threadManager.PrintSummaryResults();
}


static void
dump_recorded()
{
	// retrieve recorded samples and parameters
	system_profiler_parameters profilerParameters;
	status_t error = _kern_system_profiler_recorded(&profilerParameters);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to get recorded profiling buffer: %s\n",
			kCommandName, strerror(error));
		exit(1);
	}

	// set global options to those of the profiler parameters
	gOptions.interval = profilerParameters.interval;
	gOptions.stack_depth = profilerParameters.stack_depth;

	// create an area for the sample buffer
	area_info info;
	error = get_area_info(profilerParameters.buffer_area, &info);
	if (error != B_OK) {
		fprintf(stderr, "%s: Recorded profiling buffer invalid: %s\n",
			kCommandName, strerror(error));
		exit(1);
	}

	system_profiler_buffer_header* bufferHeader
		= (system_profiler_buffer_header*)info.address;

	uint8* bufferBase = (uint8*)(bufferHeader + 1);
	size_t totalBufferSize = info.size - (bufferBase - (uint8*)bufferHeader);

	// create a thread manager
	ThreadManager threadManager(-1);	// TODO: We don't need a debugger port!
	error = threadManager.Init();
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to init thread manager: %s\n", kCommandName,
			strerror(error));
		exit(1);
	}

	// get the current buffer
	size_t bufferStart = bufferHeader->start;
	size_t bufferSize = bufferHeader->size;
	uint8* buffer = bufferBase + bufferStart;

	if (bufferStart + bufferSize <= totalBufferSize) {
		process_event_buffer(threadManager, buffer, bufferSize, -1);
	} else {
		size_t remainingSize = bufferStart + bufferSize - totalBufferSize;
		if (!process_event_buffer(threadManager, buffer,
				bufferSize - remainingSize, -1)) {
			process_event_buffer(threadManager, bufferBase, remainingSize, -1);
		}
	}

	// print results
	int32 threadCount = threadManager.CountThreads();
	for (int32 i = 0; i < threadCount; i++) {
		Thread* thread = threadManager.ThreadAt(i);
		thread->PrintResults();
	}

	threadManager.PrintSummaryResults();
}


static void
profile_single(const char* const* programArgs, int programArgCount)
{
	// get thread/team to be debugged
	thread_id threadID = load_program(programArgs, programArgCount,
		gOptions.profile_loading);
	if (threadID < 0) {
		fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
			programArgs[0], strerror(threadID));
		exit(1);
	}

	// get the team ID
	thread_info threadInfo;
	status_t error = get_thread_info(threadID, &threadInfo);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to get info for thread %ld: %s\n",
			kCommandName, threadID, strerror(error));
		exit(1);
	}
	team_id teamID = threadInfo.team;

	// create a debugger port
	port_id debuggerPort = create_port(10, "debugger port");
	if (debuggerPort < 0) {
		fprintf(stderr, "%s: Failed to create debugger port: %s\n",
			kCommandName, strerror(debuggerPort));
		exit(1);
	}

	// add team and thread to the thread manager
	ThreadManager threadManager(debuggerPort);
	error = threadManager.Init();
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to init thread manager: %s\n", kCommandName,
			strerror(error));
		exit(1);
	}

	if (threadManager.AddTeam(teamID) != B_OK
		|| threadManager.AddThread(threadID) != B_OK) {
		exit(1);
	}

	// debug loop
	while (true) {
		debug_debugger_message_data message;
		bool quitLoop = false;
		int32 code;
		ssize_t messageSize = read_port(debuggerPort, &code, &message,
			sizeof(message));

		if (messageSize < 0) {
			if (messageSize == B_INTERRUPTED)
				continue;

			fprintf(stderr, "%s: Reading from debugger port failed: %s\n",
				kCommandName, strerror(messageSize));
			exit(1);
		}

		switch (code) {
			case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
			{
				Thread* thread = threadManager.FindThread(
					message.profiler_update.origin.thread);
				if (thread == NULL)
					break;

				thread->AddSamples(message.profiler_update.sample_count,
					message.profiler_update.dropped_ticks,
					message.profiler_update.stack_depth,
					message.profiler_update.variable_stack_depth,
					message.profiler_update.image_event);

				if (message.profiler_update.stopped) {
					thread->PrintResults();
					threadManager.RemoveThread(thread->ID());
				}
				break;
			}

			case B_DEBUGGER_MESSAGE_TEAM_CREATED:
				if (!gOptions.profile_teams)
					break;

				if (threadManager.AddTeam(message.team_created.new_team)
						== B_OK) {
					threadManager.AddThread(message.team_created.new_team);
				}
				break;
			case B_DEBUGGER_MESSAGE_TEAM_DELETED:
				// a debugged team is gone -- quit, if it is our team
				threadManager.RemoveTeam(message.origin.team);
				quitLoop = message.origin.team == teamID;
				break;
			case B_DEBUGGER_MESSAGE_TEAM_EXEC:
				if (Team* team = threadManager.FindTeam(message.origin.team)) {
					team_info teamInfo;
					thread_info threadInfo;
					if (get_team_info(message.origin.team, &teamInfo) == B_OK
						&& get_thread_info(message.origin.team, &threadInfo)
							== B_OK) {
						team->Exec(message.team_exec.image_event, teamInfo.args,
							threadInfo.name);
					}
				}
				break;

			case B_DEBUGGER_MESSAGE_THREAD_CREATED:
				if (!gOptions.profile_threads)
					break;

				threadManager.AddThread(message.thread_created.new_thread);
				break;
			case B_DEBUGGER_MESSAGE_THREAD_DELETED:
				threadManager.RemoveThread(message.origin.thread);
				break;

			case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
				threadManager.AddImage(message.origin.team,
					message.image_created.info,
					message.image_created.image_event);
				break;
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				threadManager.RemoveImage(message.origin.team,
					message.image_deleted.info.id,
					message.image_deleted.image_event);
				break;

			case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
				break;
		}

		if (quitLoop)
			break;

		// tell the thread to continue (only when there is a thread and the
		// message was synchronous)
		if (message.origin.thread >= 0 && message.origin.nub_port >= 0)
			continue_thread(message.origin.nub_port, message.origin.thread);
	}

	// prints summary results
	threadManager.PrintSummaryResults();
}


int
main(int argc, const char* const* argv)
{
	int32 stackDepth = 0;
	bool dumpRecorded = false;
	const char* outputFile = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'a' },
			{ "help", no_argument, 0, 'h' },
			{ "recorded", no_argument, 0, 'r' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+acCfhi:klo:rs:Sv:",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				gOptions.profile_all = true;
				break;
			case 'c':
				gOptions.profile_threads = false;
				break;
			case 'C':
				gOptions.profile_teams = false;
				break;
			case 'f':
				gOptions.stack_depth = 64;
				gOptions.analyze_full_stack = true;
				break;
			case 'h':
				print_usage_and_exit(false);
				break;
			case 'i':
				gOptions.interval = atol(optarg);
				break;
			case 'k':
				gOptions.profile_kernel = false;
				break;
			case 'l':
				gOptions.profile_loading = true;
				break;
			case 'o':
				outputFile = optarg;
				break;
			case 'r':
				dumpRecorded = true;
				break;
			case 's':
				stackDepth = atol(optarg);
				break;
			case 'S':
				gOptions.summary_result = true;
				break;
			case 'v':
				gOptions.callgrind_directory = optarg;
				gOptions.analyze_full_stack = true;
				gOptions.stack_depth = 64;
				break;
			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if ((!gOptions.profile_all && !dumpRecorded && optind >= argc)
		|| (dumpRecorded && optind != argc))
		print_usage_and_exit(true);

	if (stackDepth != 0)
		gOptions.stack_depth = stackDepth;

	if (outputFile != NULL) {
		gOptions.output = fopen(outputFile, "w+");
		if (gOptions.output == NULL) {
			fprintf(stderr, "%s: Failed to open output file \"%s\": %s\n",
				kCommandName, outputFile, strerror(errno));
			exit(1);
		}
	} else
		gOptions.output = stdout;

	if (dumpRecorded) {
		dump_recorded();
		return 0;
	}

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	if (gOptions.profile_all) {
		profile_all(programArgs, programArgCount);
		return 0;
	}

	profile_single(programArgs, programArgCount);
	return 0;
}
