/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <Application.h>
#include <Message.h>

#include <ObjectList.h>

#include "debug_utils.h"

#include "MessageCodes.h"
#include "TeamDebugger.h"


extern const char* __progname;
const char* kProgramName = __progname;

static const char* const kDebuggerSignature
	= "application/x-vnd.Haiku-Debugger";


static const char* kUsage =
	"Usage: %s [ <options> ]\n"
	"       %s [ <options> ] <command line>\n"
	"       %s [ <options> ] --team <team>\n"
	"       %s [ <options> ] --thread <thread>\n"
	"\n"
	"The first form starts the debugger displaying a requester to choose a\n"
	"running team to debug respectively to specify the program to run and\n"
	"debug.\n"
	"\n"
	"The second form runs the given command line and attaches the debugger to\n"
	"the new team. Unless specified otherwise the program will be stopped at\n"
	"the beginning of its main() function.\n"
	"\n"
	"The third and fourth forms attach the debugger to a running team. The\n"
	"fourth form additionally stops the specified thread.\n"
	"\n"
	"Options:\n"
	"  -h, --help    - Print this usage info and exit.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kProgramName, kProgramName,
    	kProgramName, kProgramName);
    exit(error ? 1 : 0);
}


struct Options {
	int					commandLineArgc;
	const char* const*	commandLineArgv;
	team_id				team;
	thread_id			thread;

	Options()
		:
		commandLineArgc(0),
		commandLineArgv(NULL),
		team(-1),
		thread(-1)
	{
	}
};


static bool
parse_arguments(int argc, const char* const* argv, bool noOutput,
	Options& options)
{
	optind = 1;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "team", required_argument, 0, 't' },
			{ "thread", required_argument, 0, 'T' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors

		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				if (noOutput)
					return false;
				print_usage_and_exit(false);
				break;

			case 't':
			{
				options.team = strtol(optarg, NULL, 0);
				if (options.team <= 0) {
					if (noOutput)
						return false;
					print_usage_and_exit(true);
				}
				break;
			}

			case 'T':
			{
				options.thread = strtol(optarg, NULL, 0);
				if (options.thread <= 0) {
					if (noOutput)
						return false;
					print_usage_and_exit(true);
				}
				break;
			}

			default:
				if (noOutput)
					return false;
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind < argc) {
		options.commandLineArgc = argc - optind;
		options.commandLineArgv = argv + optind;
	}

	int exclusiveParams = 0;
	if (options.team > 0)
		exclusiveParams++;
	if (options.thread > 0)
		exclusiveParams++;
	if (options.commandLineArgc > 0)
		exclusiveParams++;

	if (exclusiveParams == 0) {
		// TODO: Support!
		if (noOutput)
			return false;
		fprintf(stderr, "Sorry, running without team/thread to debug not "
			"supported yet.\n");
		exit(1);
	} else if (exclusiveParams != 1) {
		if (noOutput)
			return false;
		print_usage_and_exit(true);
	}

	return true;
}


class Debugger : public BApplication {
public:
	Debugger()
		:
		BApplication(kDebuggerSignature)
	{
	}

	~Debugger()
	{
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_DEBUGGER_QUIT_REQUESTED:
			{
				TeamDebugger* debugger = NULL;
				if (message->FindPointer("debugger",
					(void**)&debugger) == B_OK
					&& fTeamDebuggers.HasItem(debugger)) {
					fTeamDebuggers.RemoveItem(debugger);
					debugger->DeleteSelf();
					if (fTeamDebuggers.CountItems() == 0)
						PostMessage(B_QUIT_REQUESTED);
				}
				break;
			}
			default:
				BApplication::MessageReceived(message);
				break;
		}
	}

	virtual void ReadyToRun()
	{
	}

	virtual void ArgvReceived(int32 argc, char** argv)
	{
		Options options;
		if (!parse_arguments(argc, argv, true, options))
{
printf("Debugger::ArgvReceived(): parsing args failed!\n");
			return;
}

		team_id team = options.team;
		thread_id thread = options.thread;
		bool stopInMain = false;

		// If command line arguments were given, start the program.
		if (options.commandLineArgc > 0) {
printf("loading program: \"%s\" ...\n", options.commandLineArgv[0]);
			// TODO: What about the CWD?
			thread = load_program(options.commandLineArgv,
				options.commandLineArgc, false);
			if (thread < 0) {
				// TODO: Notify the user!
				fprintf(stderr, "Error: Failed to load program \"%s\": %s\n",
					options.commandLineArgv[0], strerror(thread));
				return;
			}

			team = thread;
				// main thread ID == team ID
			stopInMain = true;
		}

		// If we've got
		if (team < 0) {
printf("no team yet, getting thread info...\n");
			thread_info threadInfo;
			status_t error = get_thread_info(thread, &threadInfo);
			if (error != B_OK) {
				// TODO: Notify the user!
				fprintf(stderr, "Error: Failed to get info for thread \"%ld\": "
					"%s\n", thread, strerror(error));
				return;
			}

			team = threadInfo.team;
		}
printf("team: %ld, thread: %ld\n", team, thread);

		TeamDebugger* debugger = _TeamDebuggerForTeam(team);
		if (debugger != NULL) {
			// TODO: Activate the respective window!
printf("There's already a debugger for team: %ld\n", team);
			return;
		}

		debugger = new(std::nothrow) TeamDebugger;
		if (debugger == NULL) {
			// TODO: Notify the user!
			fprintf(stderr, "Error: Out of memory!\n");
		}

		if (debugger->Init(team, thread, stopInMain) == B_OK
			&& fTeamDebuggers.AddItem(debugger)) {
printf("debugger for team %ld created and initialized successfully!\n", team);
		} else
			delete debugger;
	}

	virtual bool QuitRequested()
	{
		// TODO:...
//		return true;
		// NOTE: The default implementation will just ask all windows'
		// QuitRequested() hooks. This in turn will ask the TeamWindows.
		// For now, this is what we want. If we have more windows later,
		// like the global TeamsWindow, then we want to just ask the
		// TeamDebuggers, the TeamsWindow should of course not go away already
		// if one or more TeamDebuggers want to stay later. There are multiple
		// ways how to do this. For examaple, TeamDebugger could get a
		// QuitReqested() hook or the TeamsWindow and other global windows
		// could always return false in their QuitRequested().
		return BApplication::QuitRequested();
	}

private:
	typedef BObjectList<TeamDebugger>	TeamDebuggerList;

private:
	TeamDebugger* _TeamDebuggerForTeam(team_id teamID) const
	{
		for (int32 i = 0; TeamDebugger* debugger = fTeamDebuggers.ItemAt(i);
				i++) {
			if (debugger->TeamID() == teamID)
				return debugger;
		}

		return NULL;
	}

private:
	TeamDebuggerList	fTeamDebuggers;
};


int
main(int argc, const char* const* argv)
{
	// We test-parse the arguments here, so, when we're started from the
	// terminal and there's an instance already running, we can print an error
	// message to the terminal, if something's wrong with the arguments.
	{
		Options options;
		parse_arguments(argc, argv, false, options);
	}

	Debugger app;
	app.Run();
	return 0;
}
