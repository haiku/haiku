/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
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

#include "debug_utils.h"


extern const char* __progname;
static const char* kCommandName = __progname;


class Symbol {
public:
	Symbol(addr_t base, size_t size, const char* name)
		:
		base(base),
		size(size),
		name(name)
	{
	}

	const char* Name() const	{ return name.String(); }

	addr_t	base;
	size_t	size;
	BString	name;
};


struct hit_symbol {
	int64	hits;
	Symbol*	symbol;

	inline bool operator<(const hit_symbol& other) const
	{
		return hits > other.hits;
	}
};


struct Team;


struct Thread {
	thread_info	info;
	Team*		team;

	thread_id ID() const		{ return info.thread; }
	const char* Name() const	{ return info.name; }
};


class Team {
public:
	Team()
		:
		fNubPort(-1),
		fSymbols(1000, true),
		fStartProfilerSize(0),
		fStartProfiler(NULL)
	{
		fInfo.team = -1;
		fDebugContext.nub_port = -1;
	}

	~Team()
	{
		free(fStartProfiler);

		if (fDebugContext.nub_port >= 0)
			destroy_debug_context(&fDebugContext);

		if (fNubPort >= 0)
			remove_team_debugger(fInfo.team);
	}

	status_t Init(team_id teamID, port_id debuggerPort)
	{
		// get team info
		status_t error = get_team_info(teamID, &fInfo);
		if (error != B_OK)
			return error;

		// install ourselves as the team debugger
		fNubPort = install_team_debugger(teamID, debuggerPort);
		if (fNubPort < 0) {
			fprintf(stderr, "%s: Failed to install as debugger for team %ld: "
				"%s\n", kCommandName, teamID, strerror(fNubPort));
			return fNubPort;
		}

		// init debug context
		error = init_debug_context(&fDebugContext, teamID, fNubPort);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to init debug context for team %ld: "
				"%s\n", kCommandName, teamID, strerror(error));
			return error;
		}

		// create symbol lookup context
		debug_symbol_lookup_context* lookupContext;
		error = debug_create_symbol_lookup_context(&fDebugContext,
			&lookupContext);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to create symbol lookup context for "
				"team %ld: %s\n", kCommandName, teamID, strerror(error));
			return error;
		}

		error = _LoadSymbols(lookupContext);
		debug_delete_symbol_lookup_context(lookupContext);
		if (error != B_OK)
			return error;

		// prepare the start profiler message
		int32 symbolCount = fSymbols.CountItems();
		if (symbolCount == 0) {
			fprintf(stderr, "%s: Got no symbols at all...\n", kCommandName);
			return B_ERROR;
		}

		printf("Found %ld functions.\n", symbolCount);

		fStartProfilerSize = sizeof(debug_nub_start_profiler)
			+ (symbolCount - 1) * sizeof(debug_profile_function);
printf("start profiler message size: %lu\n", fStartProfilerSize);
		fStartProfiler = (debug_nub_start_profiler*)malloc(fStartProfilerSize);
		if (fStartProfiler == NULL) {
			fprintf(stderr, "%s: Out of memory\n", kCommandName);
			exit(1);
		}

		fStartProfiler->reply_port = fDebugContext.reply_port;
		fStartProfiler->interval = 1000;
		fStartProfiler->function_count = symbolCount;

		for (int32 i = 0; i < symbolCount; i++) {
			Symbol* symbol = fSymbols.ItemAt(i);
			debug_profile_function& function = fStartProfiler->functions[i];
			function.base = symbol->base;
			function.size = symbol->size;
		}

		// set team debugging flags
		int32 teamDebugFlags = B_TEAM_DEBUG_THREADS
			| B_TEAM_DEBUG_TEAM_CREATION;
		set_team_debugging_flags(fNubPort, teamDebugFlags);

		return B_OK;
	}

	status_t InitThread(Thread* thread)
	{
		// set thread debugging flags and start profiling
		int32 threadDebugFlags = 0;
//		if (!traceTeam) {
//			threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
//				| (traceChildThreads
//					? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
//		}
		set_thread_debugging_flags(fNubPort, thread->ID(), threadDebugFlags);

		// start profiling
		fStartProfiler->thread = thread->ID();
		debug_nub_start_profiler_reply reply;
		status_t error = send_debug_message(&fDebugContext,
			B_DEBUG_START_PROFILER, fStartProfiler, fStartProfilerSize, &reply,
			sizeof(reply));
		if (error != B_OK || (error = reply.error) != B_OK) {
			fprintf(stderr, "%s: Failed to start profiler for thread %ld: %s\n",
				kCommandName, thread->ID(), strerror(error));
			return error;
		}

		// resume the target thread to be sure, it's running
		resume_thread(thread->ID());

		thread->team = this;

		return B_OK;
	}

	team_id ID() const
	{
		return fInfo.team;
	}

	BObjectList<Symbol>& Symbols()
	{
		return fSymbols;
	}

	int32 SymbolCount() const
	{
		return fSymbols.CountItems();
	}

private:
	status_t _LoadSymbols(debug_symbol_lookup_context* lookupContext)
	{
		// iterate through the team's images and collect the symbols
		image_info imageInfo;
		int32 cookie = 0;
		while (get_next_image_info(fInfo.team, &cookie, &imageInfo) == B_OK) {
			printf("Loading symbols of image \"%s\" (%ld)...\n",
				imageInfo.name, imageInfo.id);

			// create symbol iterator
			debug_symbol_iterator* iterator;
			status_t error = debug_create_image_symbol_iterator(lookupContext,
				imageInfo.id, &iterator);
			if (error != B_OK) {
				printf("Failed to init symbol iterator: %s\n", strerror(error));
				continue;
			}

			// iterate through the images
			char symbolName[1024];
			int32 symbolType;
			void* symbolLocation;
			size_t symbolSize;
			while (debug_next_image_symbol(iterator, symbolName, sizeof(symbolName),
					&symbolType, &symbolLocation, &symbolSize) == B_OK) {
	//			printf("  %s %p (%6lu) %s\n",
	//				symbolType == B_SYMBOL_TYPE_TEXT ? "text" : "data",
	//				symbolLocation, symbolSize, symbolName);
				if (symbolSize > 0 && symbolType == B_SYMBOL_TYPE_TEXT) {
					Symbol* symbol = new(std::nothrow) Symbol(
						(addr_t)symbolLocation, symbolSize, symbolName);
					if (symbol == NULL || !fSymbols.AddItem(symbol)) {
						delete symbol;
						fprintf(stderr, "%s: Out of memory\n", kCommandName);
						debug_delete_image_symbol_iterator(iterator);
						return B_NO_MEMORY;
					}
				}
			}

			debug_delete_image_symbol_iterator(iterator);
		}

		return B_OK;
	}

private:
	team_info					fInfo;
	port_id						fNubPort;
	debug_context				fDebugContext;
	BObjectList<Symbol>			fSymbols;
	size_t						fStartProfilerSize;
	debug_nub_start_profiler*	fStartProfiler;
};


class ThreadManager {
public:
	ThreadManager(port_id debuggerPort)
		:
		fTeams(20, true),
		fThreads(20, true),
		fDebuggerPort(debuggerPort),
		fDebuggerMessage(NULL),
		fMaxDebuggerMessageSize(0)
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
		if (error == B_OK)
			error = _ReallocateDebuggerMessage(team->SymbolCount());

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

		Thread* thread = new(std::nothrow) Thread;
		if (thread == NULL)
			return B_NO_MEMORY;

		thread->info = threadInfo;

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
		if (Thread* thread = FindThread(threadID))
			fThreads.RemoveItem(thread, true);
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
			if (thread->info.thread == threadID)
				return thread;
		}
		return NULL;
	}

	debug_debugger_message_data* DebuggerMessage() const
	{
		return fDebuggerMessage;
	}

	size_t MaxDebuggerMessageSize() const
	{
		return fMaxDebuggerMessageSize;
	}

private:
	status_t _ReallocateDebuggerMessage(int32 symbolCount)
	{
		// allocate memory for the reply
		size_t maxMessageSize = max_c(sizeof(debug_debugger_message_data),
			sizeof(debug_profiler_stopped) + 8 * symbolCount);
		if (maxMessageSize <= fMaxDebuggerMessageSize)
			return B_OK;

		debug_debugger_message_data* message = (debug_debugger_message_data*)
			realloc(fDebuggerMessage, maxMessageSize);
		if (message == NULL) {
			fprintf(stderr, "%s: Out of memory\n", kCommandName);
			return B_NO_MEMORY;
		}

		fDebuggerMessage = message;
		fMaxDebuggerMessageSize = maxMessageSize;

		return B_OK;
	}

private:
	BObjectList<Team>				fTeams;
	BObjectList<Thread>				fThreads;
	port_id							fDebuggerPort;
	debug_debugger_message_data*	fDebuggerMessage;
	size_t							fMaxDebuggerMessageSize;
};


// TODO: Adjust!
static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and print an analysis of\n"
	"the user and kernel times of all threads that ran during that time.\n"
	"\n"
	"Options:\n"
	"  -h, --help   - Print this usage info.\n"
	"  -o <output>  - Print the results to file <output>.\n"
	"  -s           - Also perform a scheduling analysis over the time the\n"
	"                 child process ran. This requires that scheduler kernel\n"
	"                 tracing had been enabled at compile time.\n"
;


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
//	const char* outputFile = NULL;
//	bool schedulingAnalysis = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
//			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;
/*			case 'o':
				outputFile = optarg;
				break;
			case 's':
				schedulingAnalysis = true;
				break;
*/

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	// get thread/team to be debugged
	thread_id threadID = -1;
	team_id teamID = -1;
//	if (programArgCount > 1
//		|| !get_id(*programArgs, (traceTeam ? teamID : thread))) {
		// we've been given an executable and need to load it
		threadID = load_program(programArgs, programArgCount, false);
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
		size_t maxMessageSize = threadManager.MaxDebuggerMessageSize();
		debug_debugger_message_data* message = threadManager.DebuggerMessage();
		bool quitLoop = false;
		int32 code;
		ssize_t messageSize = read_port(debuggerPort, &code, message,
			maxMessageSize);

		if (messageSize < 0) {
			if (messageSize == B_INTERRUPTED)
				continue;

			fprintf(stderr, "%s: Reading from debugger port failed: %s\n",
				kCommandName, strerror(messageSize));
			exit(1);
		}

		switch (code) {
			case B_DEBUGGER_MESSAGE_PROFILER_STOPPED:
			{
				Thread* thread = threadManager.FindThread(
					message->profiler_stopped.origin.thread);
				if (thread == NULL)
					break;

				BObjectList<Symbol>& symbols = thread->team->Symbols();
				int32 symbolCount = symbols.CountItems();

				int64 totalTicks = message->profiler_stopped.total_ticks;
				int64 missedTicks = message->profiler_stopped.missed_ticks;
				bigtime_t interval = message->profiler_stopped.interval;

				printf("\nprofiling results for thread \"%s\" (%ld):\n",
					thread->Name(), thread->ID());
				printf("  tick interval: %lld us\n", interval);
				printf("  total ticks:   %lld (%lld us)\n", totalTicks,
					totalTicks * interval);
				if (totalTicks == 0)
					totalTicks = 1;
				printf("  missed ticks:  %lld (%lld us, %6.2f%%)\n",
					missedTicks, missedTicks * interval,
					100.0 * missedTicks / totalTicks);

				// find and sort the hit symbols
				hit_symbol hitSymbols[symbolCount];
				int32 hitSymbolCount = 0;

				for (int32 i = 0; i < symbolCount; i++) {
					int64 hits = message->profiler_stopped.function_ticks[i];
					if (hits > 0) {
						hit_symbol& hitSymbol = hitSymbols[hitSymbolCount++];
						hitSymbol.hits = hits;
						hitSymbol.symbol = symbols.ItemAt(i);
					}
				}

				if (hitSymbolCount > 1)
					std::sort(hitSymbols, hitSymbols + hitSymbolCount);

				if (hitSymbolCount > 0) {
					printf("\n");
					printf("        hits       in us    in %%  function\n");
					printf("  -------------------------------------------------"
						"-----------------------------\n");
					for (int32 i = 0; i < hitSymbolCount; i++) {
						const hit_symbol& hitSymbol = hitSymbols[i];
						const Symbol* symbol = hitSymbol.symbol;
						printf("  %10lld  %10lld  %6.2f  %s\n", hitSymbol.hits,
							hitSymbol.hits * interval,
							100.0 * hitSymbol.hits / totalTicks,
							symbol->Name());
					}
				} else
					printf("  no functions were hit\n");

				threadManager.RemoveThread(thread->ID());
				break;
			}

			case B_DEBUGGER_MESSAGE_TEAM_CREATED:
				threadManager.AddTeam(message->team_created.new_team);
				break;
			case B_DEBUGGER_MESSAGE_TEAM_DELETED:
				// a debugged team is gone -- quit, if it is our team
				quitLoop = message->origin.team == teamID;
				break;
			case B_DEBUGGER_MESSAGE_THREAD_CREATED:
				threadManager.AddThread(message->thread_created.new_thread);
				break;
			case B_DEBUGGER_MESSAGE_THREAD_DELETED:
				threadManager.RemoveThread(message->origin.thread);
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
			case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				break;
		}

		if (quitLoop)
			break;

		// tell the thread to continue (only when there is a thread and the
		// message was synchronous)
		if (message->origin.thread >= 0 && message->origin.nub_port >= 0)
			continue_thread(message->origin.nub_port, message->origin.thread);
	}

	return 0;
}
