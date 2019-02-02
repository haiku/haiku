/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "debug_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <string>
#include <string.h>

#include <debugger.h>

#include <libroot_private.h>
#include <syscalls.h>
#include <syscall_load_image.h>


extern const char* __progname;
static const char* kCommandName = __progname;


// find_program
static status_t
find_program(const char* programName, std::string& resolvedPath)
{
    // If the program name is absolute, then there's nothing to do.
    // If the program name consists of more than one path element, then we
    // consider it a relative path and don't search in PATH either.
    if (*programName == '/' || strchr(programName, '/')) {
        resolvedPath = programName;
        return B_OK;
    }

    // get the PATH environment variable
    const char* paths = getenv("PATH");
    if (!paths)
        return B_ENTRY_NOT_FOUND;

    // iterate through the paths
    do {
        const char* pathEnd = strchr(paths, ':');
        int pathLen = (pathEnd ? pathEnd - paths : strlen(paths));

        // We skip empty paths.
        if (pathLen > 0) {
            // get the program path
            std::string path(paths, pathLen);
            path += "/";
            path += programName;

            // stat() the path to be sure, there is a file
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            	resolvedPath = path;
                return B_OK;
            }
        }

        paths = (pathEnd ? pathEnd + 1 : NULL);
    } while (paths);

    // not found in PATH
    return B_ENTRY_NOT_FOUND;
}


// #pragma mark -


// load_program
thread_id
load_program(const char* const* args, int32 argCount, bool traceLoading)
{
	// clone the argument vector so that we can change it
	const char** mutableArgs = new const char*[argCount];
	for (int i = 0; i < argCount; i++)
		mutableArgs[i] = args[i];

	// resolve the program path
	std::string programPath;
	status_t error = find_program(args[0], programPath);
	if (error != B_OK) {
		delete[] mutableArgs;
		return error;
	}
	mutableArgs[0] = programPath.c_str();

	// count environment variables
	int32 envCount = 0;
	while (environ[envCount] != NULL)
		envCount++;

	// flatten the program args and environment
	char** flatArgs = NULL;
	size_t flatArgsSize;
	error = __flatten_process_args(mutableArgs, argCount, environ, &envCount,
		mutableArgs[0], &flatArgs, &flatArgsSize);

	// load the program
	thread_id thread;
	if (error == B_OK) {
		thread = _kern_load_image(flatArgs, flatArgsSize, argCount, envCount,
			B_NORMAL_PRIORITY, (traceLoading ? 0 : B_WAIT_TILL_LOADED), -1, 0);

		free(flatArgs);
	} else
		thread = error;

	delete[] mutableArgs;

	return thread;
}


// set_team_debugging_flags
status_t
set_team_debugging_flags(port_id nubPort, int32 flags)
{
	debug_nub_set_team_flags message;
	message.flags = flags;

	status_t error = B_OK;
	do {
		error = write_port(nubPort, B_DEBUG_MESSAGE_SET_TEAM_FLAGS,
			&message, sizeof(message));
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to set team debug flags: %s\n",
			kCommandName, strerror(error));
	}

	return error;
}


// set_thread_debugging_flags
status_t
set_thread_debugging_flags(port_id nubPort, thread_id thread, int32 flags)
{
	debug_nub_set_thread_flags message;
	message.thread = thread;
	message.flags = flags;

	status_t error = B_OK;
	do {
		error = write_port(nubPort, B_DEBUG_MESSAGE_SET_THREAD_FLAGS,
			&message, sizeof(message));
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to set thread debug flags: %s\n",
			kCommandName, strerror(error));
	}

	return error;
}


// continue_thread
status_t
continue_thread(port_id nubPort, thread_id thread)
{
	debug_nub_continue_thread message;
	message.thread = thread;
	message.handle_event = B_THREAD_DEBUG_HANDLE_EVENT;
	message.single_step = false;

	status_t error = B_OK;

	do {
		error = write_port(nubPort, B_DEBUG_MESSAGE_CONTINUE_THREAD,
			&message, sizeof(message));
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to run thread %" B_PRId32 ": %s\n",
			kCommandName, thread, strerror(error));
	}

	return error;
}
