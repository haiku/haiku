/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <TeamDebugger.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <string.h>

#include <Path.h>
#include <String.h>

#include <libroot_private.h>
#include <syscalls.h>
#include <syscall_load_image.h>


BTeamDebugger::BTeamDebugger()
	:
	fDebuggerPort(-1)
{
}


BTeamDebugger::~BTeamDebugger()
{
	Uninstall();
}


status_t
BTeamDebugger::Install(team_id team)
{
	Uninstall();

	// create a debugger port
	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "debugger for team %" B_PRId32, team);
	fDebuggerPort = create_port(100, name);
	if (fDebuggerPort < 0)
		return fDebuggerPort;

	port_id nubPort = install_team_debugger(team, fDebuggerPort);
	if (nubPort < 0) {
		delete_port(fDebuggerPort);
		fDebuggerPort = -1;
		return nubPort;
	}

	status_t error = BDebugContext::Init(team, nubPort);
	if (error != B_OK) {
		remove_team_debugger(team);
		delete_port(fDebuggerPort);
		fDebuggerPort = -1;
		return error;
	}

	return B_OK;
}


status_t
BTeamDebugger::Uninstall()
{
	if (Team() < 0)
		return B_BAD_VALUE;

	remove_team_debugger(Team());

	delete_port(fDebuggerPort);

	BDebugContext::Uninit();

	fDebuggerPort = -1;

	return B_OK;
}


status_t
BTeamDebugger::LoadProgram(const char* const* args, int32 argCount,
	bool traceLoading)
{
	// load the program
	thread_id thread = _LoadProgram(args, argCount, traceLoading);
	if (thread < 0)
		return thread;

	// install the debugger
	status_t error = Install(thread);
	if (error != B_OK) {
		kill_team(thread);
		return error;
	}

	return B_OK;
}


status_t
BTeamDebugger::ReadDebugMessage(int32& _messageCode,
	debug_debugger_message_data& messageBuffer)
{
	ssize_t bytesRead = read_port(fDebuggerPort, &_messageCode, &messageBuffer,
		sizeof(messageBuffer));
	if (bytesRead < 0)
		return bytesRead;

	return B_OK;
}



/*static*/ thread_id
BTeamDebugger::_LoadProgram(const char* const* args, int32 argCount,
	bool traceLoading)
{
	// clone the argument vector so that we can change it
	const char** mutableArgs = new const char*[argCount];
	for (int i = 0; i < argCount; i++)
		mutableArgs[i] = args[i];

	// resolve the program path
	BPath programPath;
	status_t error = _FindProgram(args[0], programPath);
	if (error != B_OK) {
		delete[] mutableArgs;
		return error;
	}
	mutableArgs[0] = programPath.Path();

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


/*static*/ status_t
BTeamDebugger::_FindProgram(const char* programName, BPath& resolvedPath)
{
    // If the program name is absolute, then there's nothing to do.
    // If the program name consists of more than one path element, then we
    // consider it a relative path and don't search in PATH either.
    if (*programName == '/' || strchr(programName, '/'))
        return resolvedPath.SetTo(programName);

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
			BString directory(paths, pathLen);
			if (directory.Length() == 0)
				return B_NO_MEMORY;

			BPath path;
			status_t error = path.SetTo(directory, programName);
			if (error != B_OK)
				continue;

            // stat() the path to be sure, there is a file
            struct stat st;
            if (stat(path.Path(), &st) == 0 && S_ISREG(st.st_mode)) {
            	resolvedPath = path;
                return B_OK;
            }
        }

        paths = (pathEnd ? pathEnd + 1 : NULL);
    } while (paths);

    // not found in PATH
    return B_ENTRY_NOT_FOUND;
}
