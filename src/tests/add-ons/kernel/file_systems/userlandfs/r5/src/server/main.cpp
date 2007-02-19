// main.cpp

#include <stdio.h>
#include <string.h>

#include "Debug.h"
#include "ServerDefs.h"
#include "UserlandFSDispatcher.h"
#include "UserlandFSServer.h"

// server signature
static const char* kServerSignature
	= "application/x-vnd.bonefish.userlandfs-server";

// usage
static const char* kUsage =
"Usage: %s <options>\n"
"       %s <options> <file system>\n"
"\n"
"The first version runs the server as the dispatcher, i.e. as the singleton\n"
"app the kernel add-on contacts when it is looking for a file system.\n"
"The dispatcher uses the second version to start a server for a specific file\n"
"system.\n"
"\n"
"Options:\n"
"  --debug     - the file system server enters the debugger after the\n"
"                userland file system add-on has been loaded and is\n"
"                ready to be used. If specified for the dispatcher, it\n"
"                passes the flag to all file system servers it starts.\n"
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
	// parse options
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
	if (argi < argc) {
		print_usage();
		return 1;
	}
	// create and init the application
	BApplication* app = NULL;
	status_t error = B_OK;
	if (dispatcher) {
		UserlandFSDispatcher* dispatcher
			= new(nothrow) UserlandFSDispatcher(kServerSignature);
		if (!dispatcher) {
			fprintf(stderr, "Failed to create dispatcher.\n");
			return 1;
		}
		error = dispatcher->Init();
		app = dispatcher;
	} else {
		UserlandFSServer* server
			= new(nothrow) UserlandFSServer(kServerSignature);
		if (!server) {
			fprintf(stderr, "Failed to create server.\n");
			return 1;
		}
		error = server->Init(fileSystem);
		app = server;
	}
	// run it, if everything went fine
	if (error == B_OK)
		app->Run();
	delete app;
	return 0;
}

