/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include <debugger.h>
#include <image.h>

#include "MemoryReader.h"
#include "Syscall.h"
#include "TypeHandler.h"

extern void get_syscalls0(vector<Syscall*> &syscalls);
extern void get_syscalls1(vector<Syscall*> &syscalls);
extern void get_syscalls2(vector<Syscall*> &syscalls);
extern void get_syscalls3(vector<Syscall*> &syscalls);
extern void get_syscalls4(vector<Syscall*> &syscalls);
extern void get_syscalls5(vector<Syscall*> &syscalls);
extern void get_syscalls6(vector<Syscall*> &syscalls);
extern void get_syscalls7(vector<Syscall*> &syscalls);
extern void get_syscalls8(vector<Syscall*> &syscalls);
extern void get_syscalls9(vector<Syscall*> &syscalls);
extern void get_syscalls10(vector<Syscall*> &syscalls);
extern void get_syscalls11(vector<Syscall*> &syscalls);
extern void get_syscalls12(vector<Syscall*> &syscalls);
extern void get_syscalls13(vector<Syscall*> &syscalls);
extern void get_syscalls14(vector<Syscall*> &syscalls);
extern void get_syscalls15(vector<Syscall*> &syscalls);
extern void get_syscalls16(vector<Syscall*> &syscalls);
extern void get_syscalls17(vector<Syscall*> &syscalls);
extern void get_syscalls18(vector<Syscall*> &syscalls);
extern void get_syscalls19(vector<Syscall*> &syscalls);

static const char *kDefaultCommandName = "strace";

// usage
static const char *kUsage =
"Usage: %s [ <options> ] [ <thread or team ID> | <executable with args> ]\n"
"\n"
"Traces the the syscalls of a thread or a team. If an executable with\n"
"arguments is supplied, it is loaded and it's main thread traced.\n"
"\n"
"Options:\n"
"  -a             - Don't print syscall arguments.\n"
"  -c             - Don't colorize output.\n"
"  -f             - Fast mode. Syscall arguments contents aren't retrieved.\n"
"  -h, --help     - Print this text.\n"
"  -r             - Don't print syscall return values.\n"
"  -s             - Also trace all threads spawned by the supplied thread,\n"
"                   respectively the loaded executable's main thread.\n"
"  -T             - Trace all threads of the supplied or loaded executable's\n"
"                   team. If an ID is supplied, it is interpreted as a team\n"
"                   ID."
;

// command line args
static int sArgc;
static const char *const *sArgv;

// syscalls
static vector<Syscall*>			sSyscallVector;
static map<string, Syscall*>	sSyscallMap;


// print_usage
void
print_usage(bool error)
{
	// get nice program name
	const char *programName = (sArgc > 0 ? sArgv[0] : kDefaultCommandName);
	if (const char *lastSlash = strrchr(programName, '/'))
		programName = lastSlash + 1;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, programName);
}

// print_usage_and_exit
static
void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}

// get_id
static
bool
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

// load_program
thread_id
load_program(const char *const *args, int32 argCount)
{
// TODO: We need to find the program in the PATH, if no absolute or relative
// path has been given (i.e. only a name).
	return load_image(argCount, (const char**)args, (const char**)environ);
}


// set_team_debugging_flags
static
void
set_team_debugging_flags(port_id nubPort, int32 flags)
{
	debug_nub_set_team_flags message;
	message.flags = flags;

	while (true) {
		status_t error = write_port(nubPort, B_DEBUG_MESSAGE_SET_TEAM_FLAGS,
			&message, sizeof(message));
		if (error == B_OK)
			return;

		if (error != B_INTERRUPTED) {
			fprintf(stderr, "Failed to set team debug flags: %s\n",
				strerror(error));
			exit(1);
		}
	}
}

// set_thread_debugging_flags
static
void
set_thread_debugging_flags(port_id nubPort, thread_id thread, int32 flags)
{
	debug_nub_set_thread_flags message;
	message.thread = thread;
	message.flags = flags;

	while (true) {
		status_t error = write_port(nubPort, B_DEBUG_MESSAGE_SET_THREAD_FLAGS,
			&message, sizeof(message));
		if (error == B_OK)
			return;

		if (error != B_INTERRUPTED) {
			fprintf(stderr, "Failed to set thread debug flags: %s\n",
				strerror(error));
			exit(1);
		}
	}
}

// run_thread
static
void
run_thread(port_id nubPort, thread_id thread)
{
	debug_nub_run_thread message;
	message.thread = thread;

	while (true) {
		status_t error = write_port(nubPort, B_DEBUG_MESSAGE_RUN_THREAD,
			&message, sizeof(message));
		if (error == B_OK)
			return;

		if (error != B_INTERRUPTED) {
			fprintf(stderr, "Failed to run thread %ld: %s\n",
				thread, strerror(error));
			exit(1);
		}
	}
}

// init_syscalls
static
void
init_syscalls()
{
	// init the syscall vector
	get_syscalls0(sSyscallVector);
	get_syscalls1(sSyscallVector);
	get_syscalls2(sSyscallVector);
	get_syscalls3(sSyscallVector);
	get_syscalls4(sSyscallVector);
	get_syscalls5(sSyscallVector);
	get_syscalls6(sSyscallVector);
	get_syscalls7(sSyscallVector);
	get_syscalls8(sSyscallVector);
	get_syscalls9(sSyscallVector);
	get_syscalls10(sSyscallVector);
	get_syscalls11(sSyscallVector);
	get_syscalls12(sSyscallVector);
	get_syscalls13(sSyscallVector);
	get_syscalls14(sSyscallVector);
	get_syscalls15(sSyscallVector);
	get_syscalls16(sSyscallVector);
	get_syscalls17(sSyscallVector);
	get_syscalls18(sSyscallVector);
	get_syscalls19(sSyscallVector);

	// init the syscall map
	int32 count = sSyscallVector.size();
	for (int32 i = 0; i < count; i++) {
		Syscall *syscall = sSyscallVector[i];
		sSyscallMap[syscall->Name()] = syscall;
	}
}

// print_syscall
static
void
print_syscall(debug_pre_syscall &message, MemoryReader &memoryReader,
	bool printArguments, bool getContents, bool printReturnValue,
	bool colorize)
{
	int32 syscallNumber = message.syscall;
	Syscall *syscall = sSyscallVector[syscallNumber];

	// print syscall name
	printf("[%6ld] %s(", message.thread, syscall->Name().c_str());

	// print arguments
	if (printArguments) {
		int32 count = syscall->CountParameters();
		for (int32 i = 0; i < count; i++) {
			// get the value
			Parameter *parameter = syscall->ParameterAt(i);
			TypeHandler *handler = parameter->Handler();
			string value = handler->GetParameterValue(
				(char*)message.args + parameter->Offset(), getContents,
				memoryReader);

			printf((i > 0 ? ", %s" : "%s"), value.c_str());
		}
	}

	printf(") pre\n");
}

// print_syscall
static
void
print_syscall(debug_post_syscall &message, MemoryReader &memoryReader,
	bool printArguments, bool getContents, bool printReturnValue,
	bool colorize)
{
// TODO: colorize support
	int32 syscallNumber = message.syscall;
	Syscall *syscall = sSyscallVector[syscallNumber];

	// print syscall name
	printf("[%6ld] %s(", message.thread, syscall->Name().c_str());

	// print arguments
	if (printArguments) {
		int32 count = syscall->CountParameters();
		for (int32 i = 0; i < count; i++) {
			// get the value
			Parameter *parameter = syscall->ParameterAt(i);
			TypeHandler *handler = parameter->Handler();
			string value = handler->GetParameterValue(
				(char*)message.args + parameter->Offset(), getContents,
				memoryReader);

			printf((i > 0 ? ", %s" : "%s"), value.c_str());
		}
	}

	printf(")");

	// print return value
	if (printReturnValue) {
		TypeHandler *handler = syscall->ReturnType()->Handler();
		string value = handler->GetReturnValue(message.return_value,
			getContents, memoryReader);
		if (value.length() > 0)
			printf(" = %s", value.c_str());
// TODO: If the return value is a status_t, print the error code.
	}

	printf(" (%lld us)\n", message.end_time - message.start_time);

//for (int32 i = 0; i < 16; i++) {
//	if (i % 4 == 0) {
//		if (i > 0)
//			printf("\n");
//		printf("  ");
//	} else
//		printf(" ");
//	printf("%08lx", message.args[i]);
//}
//printf("\n");

}


// main
int
main(int argc, const char *const *argv)
{
	sArgc = argc;
	sArgv = argv;

	// parameters
	const char *const *programArgs = NULL;
	int32 programArgCount = 0;
	bool printArguments = true;
	bool colorize = true;
	bool fastMode = false;
	bool printReturnValues = true;
	bool traceChildThreads = false;
	bool traceTeam = false;

	// parse arguments
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "-a") == 0) {
				printArguments = false;
			} else if (strcmp(arg, "-c") == 0) {
				colorize = false;
			} else if (strcmp(arg, "-f") == 0) {
				fastMode = true;
			} else if (strcmp(arg, "-r") == 0) {
				printReturnValues = false;
			} else if (strcmp(arg, "-s") == 0) {
				traceChildThreads = true;
			} else if (strcmp(arg, "-T") == 0) {
				traceTeam = true;
			} else {
				print_usage_and_exit(true);
			}
		} else {
			programArgs = argv + argi;
			programArgCount = argc - argi;
			break;
		}
	}

	// check parameters
	if (!programArgs)
		print_usage_and_exit(true);

	// initialize our syscalls vector and map
	init_syscalls();

	// get thread/team to be debugged
	thread_id thread = -1;
	team_id team = -1;
	if (programArgCount > 1
		|| !get_id(*programArgs, (traceTeam ? team : thread))) {
		// we've been given an executable and need to load it
		thread = load_program(programArgs, programArgCount);
	}		

	// get the team ID, if we have none yet
	if (team < 0) {
		thread_info threadInfo;
		status_t error = get_thread_info(thread, &threadInfo);
		if (error != B_OK) {
			fprintf(stderr, "Failed to get info for thread %ld: %s\n", thread,
				strerror(error));
			exit(1);
		}
		team = threadInfo.team;
	}

	// create a debugger port
	port_id debuggerPort = create_port(10, "debugger port");
	if (debuggerPort < 0) {
		fprintf(stderr, "Failed to create debugger port: %s\n",
			strerror(debuggerPort));
		exit(1);
	}

	// install ourselves as the team debugger
	port_id nubPort = install_team_debugger(team, debuggerPort);
	if (nubPort < 0) {
		fprintf(stderr, "Failed to install team debugger: %s\n",
			strerror(nubPort));
		exit(1);
	}

	// set team debugging flags
	int32 teamDebugFlags
		= (printArguments ? 0 : B_TEAM_DEBUG_SYSCALL_FAST_TRACE)
		| (traceTeam ? B_TEAM_DEBUG_POST_SYSCALL : 0);
	set_team_debugging_flags(nubPort, teamDebugFlags);

	// set thread debugging flags
	if (thread >= 0) {
		int32 threadDebugFlags = 0;
		if (!traceTeam) {
			threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
				| (traceChildThreads
					? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
		}
		set_thread_debugging_flags(nubPort, thread, threadDebugFlags);

		// resume the target thread to be sure, it's running
		resume_thread(thread);
	}

	MemoryReader memoryReader(nubPort);

	// debug loop
	while (true) {
		int32 code;
		debug_debugger_message_data message;
		ssize_t messageSize = read_port(debuggerPort, &code, &message,
			sizeof(message));

		if (messageSize < 0) {
			if (messageSize == B_INTERRUPTED)
				continue;

			fprintf(stderr, "Reading from debugger port failed: %s\n",
				strerror(messageSize));
			exit(1);
		}

		thread_id concernedThread = -1;
		switch (code) {
			case B_DEBUGGER_MESSAGE_THREAD_STOPPED:
				concernedThread = message.thread_stopped.thread;
printf("B_DEBUGGER_MESSAGE_THREAD_STOPPED: thread: %ld\n", concernedThread);
				break;
			case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			{
				concernedThread = message.pre_syscall.thread;
//printf("B_DEBUGGER_MESSAGE_PRE_SYSCALL: thread: %ld\n", concernedThread);
print_syscall(message.pre_syscall, memoryReader,
	printArguments, !fastMode, printReturnValues, colorize);

				break;
			}
			case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
				concernedThread = message.signal_received.thread;
printf("B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED: thread: %ld\n", concernedThread);
				break;
			case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			{
				concernedThread = message.post_syscall.thread;
				print_syscall(message.post_syscall, memoryReader,
					printArguments, !fastMode, printReturnValues, colorize);

				break;
			}
			case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			{
				printf("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %ld\n",
					message.team_deleted.team);
				exit(0);
			}
		}

		// tell the thread to continue
		if (concernedThread >= 0) {
			run_thread(nubPort, concernedThread);
		}
	}
	
	return 0;
}
