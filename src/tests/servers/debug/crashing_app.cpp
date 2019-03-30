/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#undef NDEBUG

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>

extern const char *__progname;

// usage
static const char *kUsage =
	"%s <options> [ <mode> ]\n"
	"Crashes in more or less inovative ways.\n"
	"\n"
	"Options:\n"
	"  -d                   - call disable_debugger() first\n"
	"  -f, --fork           - fork() and continue in the child\n"
	"  -h, --help           - print this info text\n"
	"  --multi              - crash in multiple threads\n"
	"  --signal             - crash in a signal handler\n"
	"  --thread             - crash in a separate thread\n"
	"\n"
	"Modes:\n"
	"[general]\n"
	"  segv                 - dereferences a null pointer (default)\n"
	"  segv2                - strcmp() using a 0x1 pointer\n"
	"  div                  - executes a division by zero\n"
	"  debugger             - invokes debugger()\n"
	"  assert               - failed assert(), which should invoke the\n"
	"                         debugger\n"
	"\n"
	"[x86 specific]\n"
	"  int3                 - executes the int3 (breakpoint) instruction\n"
	"  protection           - executes an instruction that causes a general\n"
	"                         protection exception\n";

// application name
const char *kAppName = __progname;

static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kAppName);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 0 : 1);
}


static int
crash_segv()
{
	int *a = 0;
	*a = 0;
	return 0;
}

static int
crash_segv2()
{
	const char *str = (const char*)0x1;
	return strcmp(str, "Test");
}

static int
crash_div()
{
	int i = 0;
	i = 1 / i;
	return i;
}

static int
crash_debugger()
{
	debugger("crashing_app() invoked debugger()");
	return 0;
}


static int
crash_assert()
{
	assert(0 > 1);
	return 0;
}


#if __i386__

static int
crash_int3()
{
	asm("int3");
	return 0;
}

static int
crash_protection()
{
	asm("movl %0, %%dr7" : : "r"(0));
	return 0;
}

#endif	// __i386__


typedef int crash_function_t();


struct Options {
	Options()
		:
		inThread(false),
		multipleThreads(false),
		inSignalHandler(false),
		disableDebugger(false),
		fork(false)
	{
	}

	crash_function_t*	function;
	bool				inThread;
	bool				multipleThreads;
	bool				inSignalHandler;
	bool				disableDebugger;
	bool				fork;
};

static Options sOptions;


static crash_function_t*
get_crash_function(const char* mode)
{
	if (strcmp(mode, "segv") == 0) {
		return crash_segv;
	} else if (strcmp(mode, "segv2") == 0) {
		return crash_segv2;
	} else if (strcmp(mode, "div") == 0) {
		return (crash_function_t*)crash_div;
	} else if (strcmp(mode, "debugger") == 0) {
		return crash_debugger;
	} else if (strcmp(mode, "assert") == 0) {
		return crash_assert;
#if __i386__
	} else if (strcmp(mode, "int3") == 0) {
		return crash_int3;
	} else if (strcmp(mode, "protection") == 0) {
		return crash_protection;
#endif	// __i386__
	}

	return NULL;
}


static void
signal_handler(int signal)
{
	sOptions.function();
}


static void
do_crash()
{
	if (sOptions.inSignalHandler) {
		signal(SIGUSR1, &signal_handler);
		send_signal(find_thread(NULL), SIGUSR1);
	} else
		sOptions.function();
}


static status_t
crashing_thread(void* data)
{
	snooze(100000);
	do_crash();
	return 0;
}


int
main(int argc, const char* const* argv)
{
	const char* mode = "segv";

	// parse args
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "-d") == 0) {
				sOptions.disableDebugger = true;
			} else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--fork") == 0) {
				sOptions.fork = true;
			} else if (strcmp(arg, "--multi") == 0) {
				sOptions.inThread = true;
				sOptions.multipleThreads = true;
			} else if (strcmp(arg, "--signal") == 0) {
				sOptions.inSignalHandler = true;
			} else if (strcmp(arg, "--thread") == 0) {
				sOptions.inThread = true;
			} else {
				fprintf(stderr, "Invalid option \"%s\"\n", arg);
				print_usage_and_exit(true);
			}
		} else {
			mode = arg;
		}
	}

	sOptions.function = get_crash_function(mode);
	if (sOptions.function == NULL) {
		fprintf(stderr, "Invalid mode \"%s\"\n", mode);
		print_usage_and_exit(true);
	}

	if (sOptions.disableDebugger)
		disable_debugger(true);

	if (sOptions.fork) {
		pid_t child = fork();
		if (child < 0) {
			fprintf(stderr, "fork() failed: %s\n", strerror(errno));
			exit(1);
		}

		if (child > 0) {
			// the parent exits
			exit(1);
		}

		// the child continues...
	}

	if (sOptions.inThread) {
		thread_id thread = spawn_thread(crashing_thread, "crashing thread",
			B_NORMAL_PRIORITY, NULL);
		if (thread < 0) {
			fprintf(stderr, "Error: Failed to spawn thread: %s\n",
				strerror(thread));
			exit(1);
		}

		resume_thread(thread);

		if (sOptions.multipleThreads) {
			snooze(200000);
			do_crash();
		} else {
			status_t result;
			while (wait_for_thread(thread, &result) == B_INTERRUPTED) {
			}
		}
	} else {
		do_crash();
	}

	return 0;
}
