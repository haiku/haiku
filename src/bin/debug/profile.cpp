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
	thread_id thread = -1;
	team_id team = -1;
//	if (programArgCount > 1
//		|| !get_id(*programArgs, (traceTeam ? team : thread))) {
		// we've been given an executable and need to load it
		thread = load_program(programArgs, programArgCount, false);
		if (thread < 0) {
			fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
				programArgs[0], strerror(thread));
			exit(1);
		}
//	}

	// get the team ID, if we have none yet
	if (team < 0) {
		thread_info threadInfo;
		status_t error = get_thread_info(thread, &threadInfo);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to get info for thread %ld: %s\n",
				kCommandName, thread, strerror(error));
			exit(1);
		}
		team = threadInfo.team;
	}

	// create a debugger port
	port_id debuggerPort = create_port(10, "debugger port");
	if (debuggerPort < 0) {
		fprintf(stderr, "%s: Failed to create debugger port: %s\n",
			kCommandName, strerror(debuggerPort));
		exit(1);
	}

	// install ourselves as the team debugger
	port_id nubPort = install_team_debugger(team, debuggerPort);
	if (nubPort < 0) {
		fprintf(stderr, "%s: Failed to install team debugger: %s\n",
			kCommandName, strerror(nubPort));
		exit(1);
	}

	// init debug context
	debug_context debugContext;
	status_t error = init_debug_context(&debugContext, team, nubPort);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to init debug context: %s\n",
			kCommandName, strerror(error));
		exit(1);
	}

	// create symbol lookup context
	debug_symbol_lookup_context* lookupContext;
	error = debug_create_symbol_lookup_context(&debugContext,
		&lookupContext);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to create symbol lookup context: %s\n",
			kCommandName, strerror(error));
		exit(1);
	}

	// iterate through the team's images and collect the symbols
	BObjectList<Symbol> symbols(1000, true);
	image_info imageInfo;
	int32 cookie = 0;
	while (get_next_image_info(team, &cookie, &imageInfo) == B_OK) {
		printf("Loading symbols of image \"%s\" (%ld)...\n",
			imageInfo.name, imageInfo.id);
		// create symbol iterator
		debug_symbol_iterator* iterator;
		error = debug_create_image_symbol_iterator(lookupContext, imageInfo.id,
			&iterator);
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
				if (symbol == NULL || !symbols.AddItem(symbol)) {
					fprintf(stderr, "%s: Out of memory\n", kCommandName);
					exit(1);
				}
			}
		}

		debug_delete_image_symbol_iterator(iterator);
	}

	debug_delete_symbol_lookup_context(lookupContext);

	// prepare the start profiler message
	int32 symbolCount = symbols.CountItems();
	if (symbolCount == 0) {
		fprintf(stderr, "%s: Got no symbols at all, exiting...\n",
			kCommandName);
		exit(1);
	}

	size_t startProfilerSize = sizeof(debug_nub_start_profiler)
		+ (symbolCount - 1) * sizeof(debug_profile_function);
	debug_nub_start_profiler* startProfiler
		= (debug_nub_start_profiler*)malloc(startProfilerSize);
	if (startProfiler == NULL) {
		fprintf(stderr, "%s: Out of memory\n", kCommandName);
		exit(1);
	}

	startProfiler->reply_port = debugContext.reply_port;
	startProfiler->thread = thread;
	startProfiler->interval = 1000;
	startProfiler->function_count = symbolCount;

	for (int32 i = 0; i < symbolCount; i++) {
		Symbol* symbol = symbols.ItemAt(i);
		debug_profile_function& function = startProfiler->functions[i];
		function.base = symbol->base;
		function.size = symbol->size;
	}

	// allocate memory for the reply
	size_t maxMessageSize = max_c(sizeof(debug_debugger_message_data),
		sizeof(debug_profiler_stopped) + 8 * symbolCount);
	debug_debugger_message_data* message = (debug_debugger_message_data*)
		malloc(maxMessageSize);
	if (message == NULL) {
		fprintf(stderr, "%s: Out of memory\n", kCommandName);
		exit(1);
	}

	// set team debugging flags
	int32 teamDebugFlags = 0;
	set_team_debugging_flags(nubPort, teamDebugFlags);

	// set thread debugging flags and start profiling
	if (thread >= 0) {
		int32 threadDebugFlags = 0;
//		if (!traceTeam) {
//			threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
//				| (traceChildThreads
//					? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
//		}
		set_thread_debugging_flags(nubPort, thread, threadDebugFlags);

		// start profiling
	 	debug_nub_start_profiler_reply reply;
		error = send_debug_message(&debugContext, B_DEBUG_START_PROFILER,
            startProfiler, startProfilerSize, &reply, sizeof(reply));
		if (error != B_OK || (error = reply.error) != B_OK) {
			fprintf(stderr, "%s: Failed to start profiler: %s\n",
				kCommandName, strerror(error));
			exit(1);
		}

		// resume the target thread to be sure, it's running
		resume_thread(thread);
	}

	// debug loop
	while (true) {
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
				int64 totalTicks = message->profiler_stopped.total_ticks;
				int64 missedTicks = message->profiler_stopped.missed_ticks;
				bigtime_t interval = message->profiler_stopped.interval;

				printf("\nprofiling results for thread %ld:\n",
					message->profiler_stopped.origin.thread);
				printf("  tick interval: %lld us\n", interval);
				printf("  total ticks:   %lld (%lld us)\n", totalTicks,
					totalTicks * interval);
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

				break;
			}

			case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			case B_DEBUGGER_MESSAGE_TEAM_CREATED:
			case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				break;

			case B_DEBUGGER_MESSAGE_TEAM_DELETED:
				// the debugged team is gone
				quitLoop = true;
				break;
		}

		if (quitLoop)
			break;

		// tell the thread to continue (only when there is a thread and the
		// message was synchronous)
		if (message->origin.thread >= 0 && message->origin.nub_port >= 0)
			continue_thread(message->origin.nub_port, message->origin.thread);
	}

	destroy_debug_context(&debugContext);

	return 0;
}
