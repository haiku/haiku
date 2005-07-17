
#include <stdio.h>

#include <OS.h>

extern const char *__progname;

// usage
static const char *kUsage =
	"%s <options> [ <mode> ]\n"
	"Crashes in more or less inovative ways.\n"
	"\n"
	"Options:\n"
	"  -h, --help           - print this info text\n"
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


static void
crash_segv()
{
	int *a = 0;
	*a = 0;
}

static void
crash_segv2()
{
	const char *str = (const char*)0x1;
	strcmp(str, "Test");
}

static void
crash_div()
{
	int i = 0;
	i = 1 / i;
}

static void
crash_debugger()
{
	debugger("crashing_app() invoked debugger()");
}

#if __INTEL__

static void
crash_int3()
{
	asm("int3");
}

static void
crash_protection()
{
	asm("movl %0, %%dr7" : : "r"(0));
}

#endif	// __INTEL__


int
main(int argc, const char *const *argv)
{
	const char *mode = "segv";

	// parse args
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else {
				fprintf(stderr, "Invalid option \"%s\"\n", arg);
				print_usage_and_exit(true);
			}
		} else {
			mode = arg;
		}
	}

	if (strcmp(mode, "segv") == 0) {
		crash_segv();
	} else if (strcmp(mode, "segv2") == 0) {
		crash_segv2();
	} else if (strcmp(mode, "div") == 0) {
		crash_div();
	} else if (strcmp(mode, "debugger") == 0) {
		crash_debugger();
#if __INTEL__
	} else if (strcmp(mode, "int3") == 0) {
		crash_int3();
	} else if (strcmp(mode, "protection") == 0) {
		crash_protection();
#endif	// __INTEL__
	} else {
		fprintf(stderr, "Invalid mode \"%s\"\n", mode);
		print_usage_and_exit(true);
	}

	return 0;
}
