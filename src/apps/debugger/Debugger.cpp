/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <Application.h>
#include <Entry.h>
#include <Message.h>
#include <ObjectList.h>
#include <Path.h>

#include <ArgumentVector.h>
#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "CommandLineUserInterface.h"
#include "ConnectionConfigHandlerRoster.h"
#include "ConnectionConfigWindow.h"
#include "DebuggerGlobals.h"
#include "DebuggerSettingsManager.h"
#include "DebuggerUiSettingsFactory.h"
#include "ElfFile.h"
#include "GraphicalUserInterface.h"
#include "MessageCodes.h"
#include "ReportUserInterface.h"
#include "SignalSet.h"
#include "StartTeamWindow.h"
#include "TargetHostInterface.h"
#include "TargetHostInterfaceRoster.h"
#include "TeamDebugger.h"
#include "TeamsWindow.h"
#include "ValueHandlerRoster.h"


extern const char* __progname;
const char* kProgramName = __progname;

static const char* const kDebuggerSignature
	= "application/x-vnd.Haiku-Debugger";


static const char* const kUsage =
	"Usage: %s [ <options> ]\n"
	"       %s [ <options> ] <command line>\n"
	"       %s [ <options> ] --team <team>\n"
	"       %s [ <options> ] --thread <thread>\n"
	"       %s [ <options> ] --core <file>\n"
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
	"The fifth form loads a core file.\n"
	"\n"
	"Options:\n"
	"  -h, --help        - Print this usage info and exit.\n"
	"  -c, --cli         - Use command line user interface\n"
	"  -s, --save-report - Save crash report for the targetted team and exit.\n"
	"                      Implies --cli.\n"
;


static void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kProgramName, kProgramName,
	kProgramName, kProgramName, kProgramName);
	exit(error ? 1 : 0);
}


struct Options {
	int					commandLineArgc;
	const char* const*	commandLineArgv;
	team_id				team;
	thread_id			thread;
	bool				useCLI;
	bool				saveReport;
	const char*			reportPath;
	const char*			coreFilePath;

	Options()
		:
		commandLineArgc(0),
		commandLineArgv(NULL),
		team(-1),
		thread(-1),
		useCLI(false),
		saveReport(false),
		reportPath(NULL),
		coreFilePath(NULL)
	{
	}
};


static void
set_debugger_options_from_options(TeamDebuggerOptions& _debuggerOptions,
	const Options& options)
{
	_debuggerOptions.commandLineArgc = options.commandLineArgc;
	_debuggerOptions.commandLineArgv = options.commandLineArgv;
	_debuggerOptions.team = options.team;
	_debuggerOptions.thread = options.thread;
	_debuggerOptions.coreFilePath = options.coreFilePath;

	if (options.coreFilePath != NULL)
		_debuggerOptions.requestType = TEAM_DEBUGGER_REQUEST_LOAD_CORE;
	else if (options.commandLineArgc != 0)
		_debuggerOptions.requestType = TEAM_DEBUGGER_REQUEST_CREATE;
	else
		_debuggerOptions.requestType = TEAM_DEBUGGER_REQUEST_ATTACH;
}



static bool
parse_arguments(int argc, const char* const* argv, bool noOutput,
	Options& options)
{
	optind = 1;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "cli", no_argument, 0, 'c' },
			{ "save-report", optional_argument, 0, 's' },
			{ "team", required_argument, 0, 't' },
			{ "thread", required_argument, 0, 'T' },
			{ "core", required_argument, 0, 'C' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors

		int c = getopt_long(argc, (char**)argv, "+chs", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'c':
				options.useCLI = true;
				break;

			case 'C':
				options.coreFilePath = optarg;
				break;

			case 'h':
				if (noOutput)
					return false;
				print_usage_and_exit(false);
				break;

			case 's':
			{
				options.saveReport = true;
				options.reportPath = optarg;
				break;
			}

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
		return true;
	} else if (exclusiveParams != 1) {
		if (noOutput)
			return false;
		print_usage_and_exit(true);
	}

	return true;
}


// #pragma mark - Debugger application class


class Debugger : public BApplication,
	private TargetHostInterfaceRoster::Listener {
public:
								Debugger();
								~Debugger();

			status_t			Init();
	virtual void 				MessageReceived(BMessage* message);
	virtual void 				ReadyToRun();
	virtual void 				ArgvReceived(int32 argc, char** argv);
	virtual	void				RefsReceived(BMessage* message);

private:
	virtual bool 				QuitRequested();
	virtual void 				Quit();

	// TargetHostInterfaceRoster::Listener
	virtual	void				TeamDebuggerCountChanged(int32 count);

private:
			status_t			_StartNewTeam(TargetHostInterface* interface,
									const char* teamPath, const char* args);
			status_t			_HandleOptions(const Options& options);

private:
			DebuggerSettingsManager fSettingsManager;
			ConnectionConfigWindow* fConnectionWindow;
			TeamsWindow*		fTeamsWindow;
			StartTeamWindow*	fStartTeamWindow;
};


// #pragma mark - CliDebugger


class CliDebugger : private TargetHostInterfaceRoster::Listener {
public:
								CliDebugger();
								~CliDebugger();

			bool				Run(const Options& options);
};


class ReportDebugger : private TargetHostInterfaceRoster::Listener  {
public:
								ReportDebugger();
								~ReportDebugger();
			bool				Run(const Options& options);
};


// #pragma mark - Debugger application class


Debugger::Debugger()
	:
	BApplication(kDebuggerSignature),
	TargetHostInterfaceRoster::Listener(),
	fConnectionWindow(NULL),
	fTeamsWindow(NULL),
	fStartTeamWindow(NULL)
{
}


Debugger::~Debugger()
{
	DebuggerUiSettingsFactory::DeleteDefault();
	ValueHandlerRoster::DeleteDefault();
	ConnectionConfigHandlerRoster::DeleteDefault();

	debugger_global_uninit();
}


status_t
Debugger::Init()
{
	status_t error = debugger_global_init(this);
	if (error != B_OK)
		return error;

	error = DebuggerUiSettingsFactory::CreateDefault();
	if (error != B_OK)
		return error;

	error = ValueHandlerRoster::CreateDefault();
	if (error != B_OK)
		return error;

	error = ConnectionConfigHandlerRoster::CreateDefault();
	if (error != B_OK)
		return error;

	return fSettingsManager.Init(DebuggerUiSettingsFactory::Default());
}


void
Debugger::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SHOW_TEAMS_WINDOW:
		{
			if (fTeamsWindow) {
				fTeamsWindow->Activate(true);
				break;
			}

			try {
				fTeamsWindow = TeamsWindow::Create(&fSettingsManager);
				if (fTeamsWindow != NULL)
					fTeamsWindow->Show();
			} catch (...) {
				// TODO: Notify the user!
				fprintf(stderr, "Error: Failed to create Teams window\n");
			}
			break;
		}
		case MSG_TEAMS_WINDOW_CLOSED:
		{
			fTeamsWindow = NULL;
			Quit();
			break;
		}
		case MSG_SHOW_START_TEAM_WINDOW:
		{
			TargetHostInterface* hostInterface;
			if (message->FindPointer("interface",
					reinterpret_cast<void**>(&hostInterface)) != B_OK) {
				// if an interface isn't explicitly supplied, fall back to
				// the default local interface.
				hostInterface = TargetHostInterfaceRoster::Default()
					->ActiveInterfaceAt(0);
			}

			BMessenger messenger(fStartTeamWindow);
			if (!messenger.IsValid()) {
				fStartTeamWindow = StartTeamWindow::Create(hostInterface);
				if (fStartTeamWindow == NULL)
					break;
				fStartTeamWindow->Show();
			} else
				fStartTeamWindow->Activate();
			break;
		}
		case MSG_START_TEAM_WINDOW_CLOSED:
		{
			fStartTeamWindow = NULL;
			break;
		}
		case MSG_SHOW_CONNECTION_CONFIG_WINDOW:
		{
			if (fConnectionWindow != NULL) {
				fConnectionWindow->Activate(true);
				break;
			}

			try {
				fConnectionWindow = ConnectionConfigWindow::Create();
				if (fConnectionWindow != NULL)
					fConnectionWindow->Show();
			} catch (...) {
				// TODO: Notify the user!
				fprintf(stderr, "Error: Failed to create Teams window\n");
			}
			break;
		}
		case MSG_CONNECTION_CONFIG_WINDOW_CLOSED:
		{
			fConnectionWindow = NULL;
			break;
		}
		case MSG_DEBUG_THIS_TEAM:
		{
			team_id teamID;
			if (message->FindInt32("team", &teamID) != B_OK)
				break;

			TargetHostInterface* interface;
			if (message->FindPointer("interface", reinterpret_cast<void**>(
					&interface)) != B_OK) {
				break;
			}

			TeamDebuggerOptions options;
			options.requestType = TEAM_DEBUGGER_REQUEST_ATTACH;
			options.settingsManager = &fSettingsManager;
			options.team = teamID;
			options.userInterface = new(std::nothrow) GraphicalUserInterface;
			if (options.userInterface == NULL) {
				// TODO: notify user.
				break;
			}
			BReference<UserInterface> uiReference(options.userInterface, true);
			status_t error = interface->StartTeamDebugger(options);
			if (error != B_OK) {
				// TODO: notify user.
			}
			break;
		}
		case MSG_START_NEW_TEAM:
		{
			TargetHostInterface* interface;
			if (message->FindPointer("interface", reinterpret_cast<void**>(
					&interface)) != B_OK) {
				break;
			}

			const char* teamPath = NULL;
			const char* args = NULL;

			message->FindString("path", &teamPath);
			message->FindString("arguments", &args);

			status_t result = _StartNewTeam(interface, teamPath, args);
			BMessage reply;
			reply.AddInt32("status", result);
			message->SendReply(&reply);
			break;
		}
		case MSG_LOAD_CORE_TEAM:
		{
			TargetHostInterface* interface;
			if (message->FindPointer("interface", reinterpret_cast<void**>(
					&interface)) != B_OK) {
				break;
			}

			entry_ref ref;
			if (message->FindRef("core", &ref) != B_OK)
				break;

			BPath path(&ref);
			if (path.InitCheck() != B_OK)
				break;

			Options options;
			options.coreFilePath = path.Path();
			_HandleOptions(options);
			break;
		}
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
Debugger::ReadyToRun()
{
	TargetHostInterfaceRoster* roster = TargetHostInterfaceRoster::Default();
	AutoLocker<TargetHostInterfaceRoster> lock(roster);
	if (roster->CountRunningTeamDebuggers() == 0)
		PostMessage(MSG_SHOW_TEAMS_WINDOW);
}


void
Debugger::ArgvReceived(int32 argc, char** argv)
{
	Options options;
	if (!parse_arguments(argc, argv, true, options)) {
		printf("Debugger::ArgvReceived(): parsing args failed!\n");
		return;
	}

	_HandleOptions(options);
}


void
Debugger::RefsReceived(BMessage* message)
{
	// iterate through the refs and handle the files we can handle
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BPath path;
		if (path.SetTo(&ref) != B_OK)
			continue;

		// check, whether this is a core file
		{
			ElfFile elfFile;
			if (elfFile.Init(path.Path()) != B_OK || elfFile.Type() != ET_CORE)
				continue;
		}

		// handle the core file
		Options options;
		options.coreFilePath = path.Path();
		_HandleOptions(options);
	}
}


bool
Debugger::QuitRequested()
{
	// NOTE: The default implementation will just ask all windows'
	// QuitRequested() hooks. This in turn will ask the TeamWindows.
	// For now, this is what we want. If we have more windows later,
	// like the global TeamsWindow, then we want to just ask the
	// TeamDebuggers, the TeamsWindow should of course not go away already
	// if one or more TeamDebuggers want to stay later. There are multiple
	// ways how to do this. For example, TeamDebugger could get a
	// QuitRequested() hook or the TeamsWindow and other global windows
	// could always return false in their QuitRequested().
	return BApplication::QuitRequested();
		// TODO: This is ugly. The team debuggers own the windows, not the
		// other way around.
}

void
Debugger::Quit()
{
	TargetHostInterfaceRoster* roster = TargetHostInterfaceRoster::Default();
	AutoLocker<TargetHostInterfaceRoster> lock(roster);
	// don't quit before all team debuggers have been quit
	if (roster->CountRunningTeamDebuggers() == 0 && fTeamsWindow == NULL)
		BApplication::Quit();
}


void
Debugger::TeamDebuggerCountChanged(int32 count)
{
	if (count == 0) {
		AutoLocker<Debugger> lock(this);
		Quit();
	}
}


status_t
Debugger::_StartNewTeam(TargetHostInterface* interface, const char* path,
	const char* args)
{
	if (path == NULL)
		return B_BAD_VALUE;

	BString data;
	data.SetToFormat("\"%s\" %s", path, args);
	if (data.Length() == 0)
		return B_NO_MEMORY;

	ArgumentVector argVector;
	argVector.Parse(data.String());

	TeamDebuggerOptions options;
	options.requestType = TEAM_DEBUGGER_REQUEST_CREATE;
	options.settingsManager = &fSettingsManager;
	options.userInterface = new(std::nothrow) GraphicalUserInterface;
	if (options.userInterface == NULL)
		return B_NO_MEMORY;
	BReference<UserInterface> uiReference(options.userInterface, true);
	options.commandLineArgc = argVector.ArgumentCount();
	if (options.commandLineArgc <= 0)
		return B_BAD_VALUE;

	char** argv = argVector.DetachArguments();

	options.commandLineArgv = argv;
	MemoryDeleter deleter(argv);

	status_t error = interface->StartTeamDebugger(options);
	if (error == B_OK) {
		deleter.Detach();
	}

	return error;
}


status_t
Debugger::_HandleOptions(const Options& options)
{
	TeamDebuggerOptions debuggerOptions;
	set_debugger_options_from_options(debuggerOptions, options);
	debuggerOptions.settingsManager = &fSettingsManager;
	debuggerOptions.userInterface = new(std::nothrow) GraphicalUserInterface;
	if (debuggerOptions.userInterface == NULL)
		return B_NO_MEMORY;
	BReference<UserInterface> uiReference(debuggerOptions.userInterface, true);
	TargetHostInterface* hostInterface
		= TargetHostInterfaceRoster::Default()->ActiveInterfaceAt(0);
	return hostInterface->StartTeamDebugger(debuggerOptions);
}


// #pragma mark - CliDebugger


CliDebugger::CliDebugger()
{
}


CliDebugger::~CliDebugger()
{
	DebuggerUiSettingsFactory::DeleteDefault();
	debugger_global_uninit();
}


bool
CliDebugger::Run(const Options& options)
{
	// Block SIGINT, in this thread so all threads created by it inherit the
	// a block mask with the signal blocked. In the input loop the signal will
	// be unblocked again.
	SignalSet(SIGINT).BlockInCurrentThread();

	// initialize global objects and settings manager
	status_t error = debugger_global_init(this);
	if (error != B_OK) {
		fprintf(stderr, "Error: Global initialization failed: %s\n",
			strerror(error));
		return false;
	}

	error = DebuggerUiSettingsFactory::CreateDefault();
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create default settings factory: "
			"%s\n",	strerror(error));
		return false;
	}


	DebuggerSettingsManager settingsManager;
	error = settingsManager.Init(DebuggerUiSettingsFactory::Default());
	if (error != B_OK) {
		fprintf(stderr, "Error: Settings manager initialization failed: "
			"%s\n", strerror(error));
		return false;
	}

	// create the command line UI
	CommandLineUserInterface* userInterface
		= new(std::nothrow) CommandLineUserInterface();
	if (userInterface == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return false;
	}
	BReference<UserInterface> userInterfaceReference(userInterface, true);

	// TODO: once we support specifying a remote interface via command line
	// args, this needs to be adjusted. For now, always assume actions
	// are being taken via the local interface.
	TargetHostInterface* hostInterface
		= TargetHostInterfaceRoster::Default()->ActiveInterfaceAt(0);

	TeamDebuggerOptions debuggerOptions;
	set_debugger_options_from_options(debuggerOptions, options);
	debuggerOptions.userInterface = userInterface;
	debuggerOptions.settingsManager = &settingsManager;
	error = hostInterface->StartTeamDebugger(debuggerOptions);
	if (error != B_OK)
		return false;

	TeamDebugger* teamDebugger = hostInterface->TeamDebuggerAt(0);
	thread_id teamDebuggerThread = teamDebugger->Thread();

	// run the input loop
	userInterface->Run();

	// wait for the team debugger thread to terminate
	wait_for_thread(teamDebuggerThread, NULL);

	return true;
}


// #pragma mark - ReportDebugger


ReportDebugger::ReportDebugger()
{
}


ReportDebugger::~ReportDebugger()
{
	debugger_global_uninit();
}


bool
ReportDebugger::Run(const Options& options)
{
	// initialize global objects and settings manager
	status_t error = debugger_global_init(this);
	if (error != B_OK) {
		fprintf(stderr, "Error: Global initialization failed: %s\n",
			strerror(error));
		return false;
	}

	// create the report UI
	ReportUserInterface* userInterface
		= new(std::nothrow) ReportUserInterface(options.thread, options.reportPath);
	if (userInterface == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return false;
	}
	BReference<UserInterface> userInterfaceReference(userInterface, true);

	TargetHostInterface* hostInterface
		= TargetHostInterfaceRoster::Default()->ActiveInterfaceAt(0);

	TeamDebuggerOptions debuggerOptions;
	set_debugger_options_from_options(debuggerOptions, options);
	debuggerOptions.userInterface = userInterface;
	error = hostInterface->StartTeamDebugger(debuggerOptions);
	if (error != B_OK)
		return false;

	TeamDebugger* teamDebugger = hostInterface->TeamDebuggerAt(0);
	thread_id teamDebuggerThread = teamDebugger->Thread();

	// run the input loop
	userInterface->Run();

	// wait for the team debugger thread to terminate
	wait_for_thread(teamDebuggerThread, NULL);

	return true;
}


// #pragma mark -


int
main(int argc, const char* const* argv)
{
	// We test-parse the arguments here, so that, when we're started from the
	// terminal and there's an instance already running, we can print an error
	// message to the terminal, if something's wrong with the arguments.
	// Otherwise, the arguments are reparsed in the actual application,
	// unless the option to use the command line interface was chosen.

	Options options;
	parse_arguments(argc, argv, false, options);

	if (options.useCLI) {
		CliDebugger debugger;
		return debugger.Run(options) ? 0 : 1;
	} else if (options.saveReport) {
		ReportDebugger debugger;
		return debugger.Run(options) ? 0 : 1;
	}

	Debugger app;
	status_t error = app.Init();
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to init application: %s\n",
			strerror(error));
		return 1;
	}

	app.Run();

	return 0;
}
