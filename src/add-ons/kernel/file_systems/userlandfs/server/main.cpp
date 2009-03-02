// main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include "Debug.h"
#include "ServerDefs.h"
#include "UserlandFSServer.h"

// server signature
static const char* kServerSignature
	= "application/x-vnd.haiku.userlandfs-server";

// usage
static const char* kUsage =
"Usage: %s <options> <file system> [ <port> ]\n"
"\n"
"Runs the userlandfs server for a given file system. Typically this is done\n"
"automatically by the kernel module when a volume is requested to be mounted,\n"
"but running the server manually can be useful for debugging purposes. The\n"
"<file system> argument specifies the name of the file system to be loaded.\n"
"<port> should not be given when starting the server manually; it is used by\n"
"the kernel module only.\n"
"\n"
"Options:\n"
"  --debug     - the file system server enters the debugger after the\n"
"                userland file system add-on has been loaded and is\n"
"                ready to be used.\n"
"  -h, --help  - print this text\n"
;

static int kArgC;
static char** kArgV;

// print_usage
void
print_usage(bool toStdErr = true)
{
	fprintf((toStdErr ? stderr : stdout), kUsage, kArgV[0], kArgV[0]);
}

// main
int
main(int argc, char** argv)
{
	kArgC = argc;
	kArgV = argv;

	// init debugging
	init_debugging();
	struct DebuggingExiter {
		DebuggingExiter()	{}
		~DebuggingExiter()	{ exit_debugging(); }
	} _;

	// parse arguments
	int argi = 1;
	for (; argi < argc; argi++) {
		const char* arg = argv[argi];
		int32 argLen = strlen(arg);
		if (argLen == 0) {
			print_usage();
			return 1;
		}
		if (arg[0] != '-')
			break;
		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage(false);
			return 0;
		} else if (strcmp(arg, "--debug") == 0) {
			gServerSettings.SetEnterDebugger(true);
		}
	}

	// get file system, if any
	bool dispatcher = true;
	const char* fileSystem = NULL;
	if (argi < argc) {
		fileSystem = argv[argi++];
		dispatcher = false;
	}

	// get the port, if any
	int32 port = -1;
	if (argi < argc) {
		port = atol(argv[argi++]);
		if (port <= 0) {
			print_usage();
			return 1;
		}
	}

	if (argi < argc) {
		print_usage();
		return 1;
	}

	// create and init the application
	status_t error = B_OK;
	UserlandFSServer* server
		= new(std::nothrow) UserlandFSServer(kServerSignature);
	if (!server) {
		fprintf(stderr, "Failed to create server.\n");
		return 1;
	}
	error = server->Init(fileSystem, port);

	// run it, if everything went fine
	if (error == B_OK)
		server->Run();

	delete server;
	return 0;
}
