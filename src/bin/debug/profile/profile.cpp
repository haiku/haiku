/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <debugger.h>
#include <OS.h>
#include <String.h>

#include <syscalls.h>
#include <system_profiler_defs.h>

#include <debug_support.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "BasicThreadProfileResult.h"
#include "CallgrindThreadProfileResult.h"
#include "debug_utils.h"
#include "Image.h"
#include "Options.h"
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
	"  -s <depth>     - Number of return address samples to take from the\n"
	"                   caller stack per tick. If the topmost address doesn't\n"
	"                   hit a known image, the next address will be matched\n"
	"                   (and so on).\n"
	"  -v <directory> - Create valgrind/callgrind output. <directory> is the\n"
	"                   directory where to put the output files.\n"
;


Options gOptions;

static bool sCaughtDeadlySignal = false;


class ThreadManager {
public:
	ThreadManager(port_id debuggerPort)
		:
		fTeams(20, true),
		fThreads(20, true),
		fDebuggerPort(debuggerPort)
	{
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
		if (FindThread(threadID) != NULL)
			return B_BAD_VALUE;

		thread_info threadInfo;
		status_t error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK)
			return error;

		Team* team = FindTeam(threadInfo.team);
		if (team == NULL)
			return B_BAD_TEAM_ID;

		Thread* thread = new(std::nothrow) Thread(threadInfo, team);
		if (thread == NULL)
			return B_NO_MEMORY;

		error = _CreateThreadProfileResult(thread);
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
		if (Team* team = FindTeam(teamID))
			fTeams.RemoveItem(team, true);
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
		if (teamID == B_SYSTEM_TEAM) {
			// a kernel image -- add it to all teams
			int32 count = fTeams.CountItems();
			for (int32 i = 0; i < count; i++)
				fTeams.ItemAt(i)->AddImage(imageInfo, teamID, event);
		}

		// a userland team image -- add it to that image
		if (Team* team = FindTeam(teamID))
			return team->AddImage(imageInfo, teamID, event);

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
			? team->Init(addedInfo)
			: team->Init(teamID, fDebuggerPort);
		if (error != B_OK) {
			delete team;
			return error;
		}

		fTeams.AddItem(team);

		if (_team != NULL)
			*_team = team;

		return B_OK;
	}

	status_t _CreateThreadProfileResult(Thread* thread)
	{
		ThreadProfileResult* profileResult;
		if (gOptions.callgrind_directory != NULL)
			profileResult = new(std::nothrow) CallgrindThreadProfileResult;
		else if (gOptions.analyze_full_stack)
			profileResult = new(std::nothrow) InclusiveThreadProfileResult;
		else
			profileResult = new(std::nothrow) ExclusiveThreadProfileResult;

		if (profileResult == NULL)
			return B_NO_MEMORY;

		status_t error = profileResult->Init(thread);
		if (error != B_OK) {
			delete profileResult;
			return error;
		}

		thread->SetProfileResult(profileResult);
		return B_OK;
	}

private:
	BObjectList<Team>				fTeams;
	BObjectList<Thread>				fThreads;
	port_id							fDebuggerPort;
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


static void
process_event_buffer(ThreadManager& threadManager, uint8* buffer,
	size_t bufferSize)
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
// TODO: ATM we're not adding kernel images to the team, if this is a team
// created after we started profiling!
				break;
			}

			case B_SYSTEM_PROFILER_TEAM_REMOVED:
			{
				system_profiler_team_removed* event
					= (system_profiler_team_removed*)buffer;

				// TODO: Print results!
				threadManager.RemoveTeam(event->team);
				break;
			}

			case B_SYSTEM_PROFILER_TEAM_EXEC:
			{
				system_profiler_team_exec* event
					= (system_profiler_team_exec*)event;

				if (Team* team = threadManager.FindTeam(event->team))
					team->Exec(0);
				break;
			}

			case B_SYSTEM_PROFILER_THREAD_ADDED:
			{
				system_profiler_thread_added* event
					= (system_profiler_thread_added*)buffer;

				threadManager.AddThread(event->thread);
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

			case B_SYSTEM_PROFILER_SAMPLES_END:
			{
				// Marks the end of the ring buffer -- we need to ignore the
				// remaining bytes.
				return;
			}
		}

		buffer += header->size;
	}
}


static void
signal_handler(int signal, void* data)
{
	sCaughtDeadlySignal = true;
}


static void
profile_all()
{
	// install signal handlers so we can exit gracefully
    struct sigaction action;
    action.sa_handler = (sighandler_t)signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_userdata = NULL;
    if (sigaction(SIGHUP, &action, NULL) < 0
		|| sigaction(SIGINT, &action, NULL) < 0
		|| sigaction(SIGQUIT, &action, NULL) < 0) {
		fprintf(stderr, "%s: Failed to install signal handlers: %s\n",
			kCommandName, strerror(errno));
		exit(1);
    }

	// create and area for the sample buffer
	system_profiler_buffer_header* bufferHeader;
	area_id area = create_area("profiling buffer", (void**)&bufferHeader,
		B_ANY_ADDRESS, PROFILE_ALL_SAMPLE_AREA_SIZE, B_NO_LOCK, B_READ_AREA);
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

	// start profiling
	status_t error = _kern_system_profiler_start(area, gOptions.interval,
		gOptions.stack_depth);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to start profiling: %s\n", kCommandName,
			strerror(error));
		exit(1);
	}

	// main event loop
	while (true) {
		// get the current buffer
		size_t bufferStart = bufferHeader->start;
		size_t bufferSize = bufferHeader->size;
		uint8* buffer = bufferBase + bufferStart;
//printf("processing buffer of size %lu bytes\n", bufferSize);

		if (bufferStart + bufferSize <= totalBufferSize) {
			process_event_buffer(threadManager, buffer, bufferSize);
		} else {
			size_t remainingSize = bufferStart + bufferSize - totalBufferSize;
			process_event_buffer(threadManager, buffer,
				bufferSize - remainingSize);
			process_event_buffer(threadManager, bufferBase, remainingSize);
		}

		// get next buffer
		error = _kern_system_profiler_next_buffer(bufferSize);

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
}


int
main(int argc, const char* const* argv)
{
	int32 stackDepth = 0;
	const char* outputFile = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'a' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+acCfhi:klo:s:v:",
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
			case 's':
				stackDepth = atol(optarg);
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

	if (!gOptions.profile_all && optind >= argc)
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

	if (gOptions.profile_all) {
		profile_all();
		return 0;
	}

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	// get thread/team to be debugged
	thread_id threadID = -1;
	team_id teamID = -1;
//	if (programArgCount > 1
//		|| !get_id(*programArgs, (traceTeam ? teamID : thread))) {
		// we've been given an executable and need to load it
		threadID = load_program(programArgs, programArgCount,
			gOptions.profile_loading);
		if (threadID < 0) {
			fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
				programArgs[0], strerror(threadID));
			exit(1);
		}
//	}

	// get the team ID, if we have none yet
	if (teamID < 0) {
		thread_info threadInfo;
		status_t error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to get info for thread %ld: %s\n",
				kCommandName, threadID, strerror(error));
			exit(1);
		}
		teamID = threadInfo.team;
	}

	// create a debugger port
	port_id debuggerPort = create_port(10, "debugger port");
	if (debuggerPort < 0) {
		fprintf(stderr, "%s: Failed to create debugger port: %s\n",
			kCommandName, strerror(debuggerPort));
		exit(1);
	}

	// add team and thread to the thread manager
	ThreadManager threadManager(debuggerPort);
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
				quitLoop = message.origin.team == teamID;
				break;
			case B_DEBUGGER_MESSAGE_TEAM_EXEC:
				if (Team* team = threadManager.FindTeam(message.origin.team))
					team->Exec(message.team_exec.image_event);
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

	return 0;
}
