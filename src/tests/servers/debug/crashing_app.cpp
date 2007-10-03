
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

extern const char *__progname;

// usage
static const char *kUsage =
	"%s <options> [ <mode> ]\n"
	"Crashes in more or less inovative ways.\n"
	"\n"
	"Options:\n"
	"  -h, --help           - print this info text\n"
	"  --thread             - crash in a separate thread\n"
	"\n"
	"Modes:\n"
	"[general]\n"
	"  segv                 - dereferences a null pointer (default)\n"
	"  segv2                - strcmp() using a 0x1 pointer\n"
	"  div                  - executes a division by zero\n"
	"  debugger             - invokes debugger()\n"
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

#if __INTEL__

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

#endif	// __INTEL__


typedef int crash_function_t();


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
#if __INTEL__
	} else if (strcmp(mode, "int3") == 0) {
		return crash_int3;
	} else if (strcmp(mode, "protection") == 0) {
		return crash_protection;
#endif	// __INTEL__
	}

	return NULL;
}


static status_t
crashing_thread(void* data)
{
	crash_function_t* doCrash = (crash_function_t*)data;
	snooze(100000);
	doCrash();
}


int
main(int argc, const char* const* argv)
{
	bool inThread = false;
	const char* mode = "segv";

	// parse args
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "--thread") == 0) {
				inThread = true;
			} else {
				fprintf(stderr, "Invalid option \"%s\"\n", arg);
				print_usage_and_exit(true);
			}
		} else {
			mode = arg;
		}
	}

	crash_function_t* doCrash = get_crash_function(mode);
	if (doCrash == NULL) {
		fprintf(stderr, "Invalid mode \"%s\"\n", mode);
		print_usage_and_exit(true);
	}

	if (inThread) {
		thread_id thread = spawn_thread(crashing_thread, "crashing thread",
			B_NORMAL_PRIORITY, (void*)doCrash);
		if (thread < 0) {
			fprintf(stderr, "Error: Failed to spawn thread: %s\n",
				strerror(thread));
			exit(1);
		}

		resume_thread(thread);
		status_t result;
		while (wait_for_thread(thread, &result) == B_INTERRUPTED) {
		}
	} else {
		doCrash();
	}

	return 0;
}
