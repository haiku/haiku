/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
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

#include <debug_support.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "debug_utils.h"
#include "Image.h"
#include "Options.h"
#include "Team.h"


extern const char* __progname;
const char* kCommandName = __progname;


class Image;
class Team;
class Thread;


static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and periodically samples\n"
	"all started threads' program counters. When a thread terminates, a list\n"
	"of the functions where the thread was encountered is printed.\n"
	"\n"
	"Options:\n"
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
;


Options gOptions;


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
		if (FindTeam(teamID) != NULL)
			return B_BAD_VALUE;

		Team* team = new(std::nothrow) Team;
		if (team == NULL)
			return B_NO_MEMORY;

		status_t error = team->Init(teamID, fDebuggerPort);
		if (error != B_OK) {
			delete team;
			return error;
		}

		fTeams.AddItem(team);

		if (_team != NULL)
			*_team = team;

		return B_OK;
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


int
main(int argc, const char* const* argv)
{
	int32 stackDepth = 0;
	const char* outputFile = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+cCfhi:klo:s:", sLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
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
			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	if (stackDepth != 0)
		gOptions.stack_depth = stackDepth;

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	if (outputFile != NULL) {
		gOptions.output = fopen(outputFile, "w+");
		if (gOptions.output == NULL) {
			fprintf(stderr, "%s: Failed to open output file \"%s\": %s\n",
				kCommandName, outputFile, strerror(errno));
			exit(1);
		}
	} else
		gOptions.output = stdout;

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
				if (!gOptions.profile_teams)
					break;

				if (Team* team = threadManager.FindTeam(message.origin.team)) {
					team->AddImage(message.image_created.info,
						message.image_created.image_event);
				}
				break;
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				if (Team* team = threadManager.FindTeam(message.origin.team)) {
					team->RemoveImage(message.image_deleted.info,
						message.image_deleted.image_event);
				}
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
