/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <string.h>

#include <Drivers.h>
#include <KernelExport.h>

#include <AutoDeleter.h>

#include <kernel.h>
#include <thread.h>

#include "TestContext.h"
#include "TestManager.h"
#include "TestOutput.h"

#include "lock/LockTestSuite.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

static const char* sDeviceNames[] = {
	"kernel_unit_tests",
	NULL
};

static const char* const kUsage =
	"Usage:\n"
	"  help\n"
	"    Print usage info.\n"
	"\n"
	"  list\n"
	"    Print available tests.\n"
	"\n"
	"  run [ <options> ] [ <tests> ]\n"
	"    Run tests. The tests to be run can be given as arguments. When no "
		"tests\n"
	"	 are specified, all tests are run.\n"
	"\n"
	"    Options:\n"
	"	  -p    - panic(), when a test check fails.\n"
	"	  -q    - Don't run any further tests after the first test failure.\n";

static TestManager* sTestManager = NULL;


// #pragma mark -


static bool
parse_command_line(char* buffer, char**& _argv, int& _argc)
{
	// Process the argument string. We split at whitespace, heeding quotes and
	// escaped characters. The processed arguments are written to the given
	// buffer, separated by single null chars.
	char* start = buffer;
	char* out = buffer;
	bool pendingArgument = false;
	int argc = 0;
	while (*start != '\0') {
		// ignore whitespace
		if (isspace(*start)) {
			if (pendingArgument) {
				*out = '\0';
				out++;
				argc++;
				pendingArgument = false;
			}
			start++;
			continue;
		}

		pendingArgument = true;

		if (*start == '"' || *start == '\'') {
			// quoted text -- continue until closing quote
			char quote = *start;
			start++;
			while (*start != '\0' && *start != quote) {
				if (*start == '\\' && quote == '"') {
					start++;
					if (*start == '\0')
						break;
				}
				*out = *start;
				start++;
				out++;
			}

			if (*start != '\0')
				start++;
		} else {
			// unquoted text
			if (*start == '\\') {
				// escaped char
				start++;
				if (start == '\0')
					break;
			}

			*out = *start;
			start++;
			out++;
		}
	}

	if (pendingArgument) {
		*out = '\0';
		argc++;
	}

	// allocate argument vector
	char** argv = new(std::nothrow) char*[argc + 1];
	if (argv == NULL)
		return false;

	// fill vector
	start = buffer;
	for (int i = 0; i < argc; i++) {
		argv[i] = start;
		start += strlen(start) + 1;
	}
	argv[argc] = NULL;

	_argv = argv;
	_argc = argc;
	return true;
}


// #pragma mark - device hooks


static status_t
device_open(const char* name, uint32 openMode, void** _cookie)
{
	*_cookie = NULL;
	return B_OK;
}


static status_t
device_close(void* cookie)
{
	return B_OK;
}


static status_t
device_free(void* cookie)
{
	return B_OK;
}


static status_t
device_control(void* cookie, uint32 op, void* arg, size_t length)
{
	return B_BAD_VALUE;
}


static status_t
device_read(void* cookie, off_t position, void* data, size_t* numBytes)
{
	*numBytes = 0;
	return B_OK;
}


static status_t
device_write(void* cookie, off_t position, const void* data, size_t* numBytes)
{
	if (position != 0)
		return B_BAD_VALUE;

	// copy data to stack buffer
	char* buffer = (char*)malloc(*numBytes + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	Thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_SYSCALL) != 0) {
		if (!IS_USER_ADDRESS(data)
			|| user_memcpy(buffer, data, *numBytes) != B_OK) {
			return B_BAD_ADDRESS;
		}
	} else
		memcpy(buffer, data, *numBytes);

	buffer[*numBytes] = '\0';

	// create an output
	TestOutput* output = new(std::nothrow) DebugTestOutput;
	if (output == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<TestOutput> outputDeleter(output);

	// parse arguments
	char** argv;
	int argc;
	if (!parse_command_line(buffer, argv, argc))
		return B_NO_MEMORY;
	ArrayDeleter<char*> argvDeleter(argv);

	if (argc == 0)
		return B_BAD_VALUE;

	// execute command
	if (strcmp(argv[0], "help") == 0) {
		// print usage -- print individual lines to avoid dprintf() buffer
		// overflows
		const char* usage = kUsage;
		while (*usage != '\0') {
			const char* lineEnd = strchr(usage, '\n');
			if (lineEnd != NULL)
				lineEnd++;
			else
				lineEnd = usage + strlen(usage);

			output->Print("%.*s", (int)(lineEnd - usage), usage);
			usage = lineEnd;
		}
	} else if (strcmp(argv[0], "list") == 0) {
		sTestManager->ListTests(*output);
	} else if (strcmp(argv[0], "run") == 0) {
		// parse options
		TestOptions options;
		int argi = 1;
		while (argi < argc) {
			const char* arg = argv[argi];
			if (*arg == '-') {
				for (arg++; *arg != '\0'; arg++) {
					switch (*arg) {
						case 'p':
							options.panicOnFailure = true;
							break;
						case 'q':
							options.quitAfterFailure = true;
							break;
						default:
							output->Print("Invalid option: \"-%c\"\n", *arg);
							return B_BAD_VALUE;
					}
				}

				argi++;
			} else
				break;
		}

		GlobalTestContext globalContext(*output, options);
		sTestManager->RunTests(globalContext, argv + argi, argc - argi);
	} else {
		output->Print("Invalid command \"%s\"!\n", argv[0]);
		return B_BAD_VALUE;
	}

	return B_OK;
}


// #pragma mark - driver interface


static device_hooks sDeviceHooks = {
	device_open,
	device_close,
	device_free,
	device_control,
	device_read,
	device_write
};


status_t
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	sTestManager = new(std::nothrow) TestManager;
	if (sTestManager == NULL)
		return B_NO_MEMORY;

	// register test suites
	sTestManager->AddTest(create_lock_test_suite());

	return B_OK;
}


void
uninit_driver(void)
{
	delete sTestManager;
}


const char**
publish_devices(void)
{
	return sDeviceNames;
}


device_hooks*
find_device(const char* name)
{
	return strcmp(name, sDeviceNames[0]) == 0 ? &sDeviceHooks : NULL;
}
