/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CommandLineUserInterface.h"

#include <stdio.h>

#include <algorithm>

#include <ArgumentVector.h>
#include <AutoDeleter.h>
#include <Referenceable.h>

#include "CliContext.h"
#include "CliQuitCommand.h"
#include "CliThreadsCommand.h"


static const char* kDebuggerPrompt = "debugger> ";


// #pragma mark - CommandEntry


struct CommandLineUserInterface::CommandEntry {
	CommandEntry(const BString& name, CliCommand* command)
		:
		fName(name),
		fCommand(command)
	{
	}

	const BString& Name() const
	{
		return fName;
	}

	CliCommand* Command() const
	{
		return fCommand.Get();
	}

private:
	BString					fName;
	BReference<CliCommand>	fCommand;
};


// #pragma mark - HelpCommand


struct CommandLineUserInterface::HelpCommand : CliCommand {
	HelpCommand(CommandLineUserInterface* userInterface)
		:
		CliCommand("print help for a command or a list of all commands",
			"%s [ <command> ]\n"
			"Prints help for command <command>, if given, or a list of all "
				"commands\n"
			"otherwise."),
		fUserInterface(userInterface)
	{
	}

	virtual void Execute(int argc, const char* const* argv, CliContext& context)
	{
		if (argc > 2) {
			PrintUsage(argv[0]);
			return;
		}

		fUserInterface->_PrintHelp(argc == 2 ? argv[1] : NULL);
	}

private:
	CommandLineUserInterface* fUserInterface;
};


// #pragma mark - CommandLineUserInterface


CommandLineUserInterface::CommandLineUserInterface()
	:
	fCommands(20, true),
	fShowSemaphore(-1),
	fShown(false),
	fTerminating(false)
{
}


CommandLineUserInterface::~CommandLineUserInterface()
{
	if (fShowSemaphore >= 0)
		delete_sem(fShowSemaphore);
}


const char*
CommandLineUserInterface::ID() const
{
	return "BasicCommandLineUserInterface";
}


status_t
CommandLineUserInterface::Init(Team* team, UserInterfaceListener* listener)
{
	status_t error = fContext.Init(team, listener);
	if (error != B_OK)
		return error;

	error = _RegisterCommands();
	if (error != B_OK)
		return error;

	fShowSemaphore = create_sem(0, "show CLI");
	if (fShowSemaphore < 0)
		return fShowSemaphore;

	return B_OK;
}


void
CommandLineUserInterface::Show()
{
	fShown = true;
	release_sem(fShowSemaphore);
}


void
CommandLineUserInterface::Terminate()
{
	fTerminating = true;

	if (fShown) {
		fContext.Terminating();

		// Wait for input loop to finish.
		while (acquire_sem(fShowSemaphore) == B_INTERRUPTED) {
		}
	} else {
		// The main thread will still be blocked in Run(). Unblock it.
		delete_sem(fShowSemaphore);
		fShowSemaphore = -1;
	}

	fContext.Cleanup();
}


status_t
CommandLineUserInterface::LoadSettings(const TeamUiSettings* settings)
{
	return B_OK;
}


status_t
CommandLineUserInterface::SaveSettings(TeamUiSettings*& settings) const
{
	return B_OK;
}


void
CommandLineUserInterface::NotifyUser(const char* title, const char* message,
	user_notification_type type)
{
}


int32
CommandLineUserInterface::SynchronouslyAskUser(const char* title,
	const char* message, const char* choice1, const char* choice2,
	const char* choice3)
{
	return -1;
}


void
CommandLineUserInterface::Run()
{
	// Wait for the Show() semaphore to be released.
	status_t error;
	do {
		error = acquire_sem(fShowSemaphore);
	} while (error == B_INTERRUPTED);

	if (error != B_OK)
		return;

	_InputLoop();

	// Release the Show() semaphore to signal Terminate().
	release_sem(fShowSemaphore);
}


/*static*/ status_t
CommandLineUserInterface::_InputLoopEntry(void* data)
{
	return ((CommandLineUserInterface*)data)->_InputLoop();
}


status_t
CommandLineUserInterface::_InputLoop()
{
	thread_id currentThread = -1;

	while (!fTerminating) {
		// Wait for a thread or Ctrl-C.
		fContext.WaitForThreadOrUser();
		if (fContext.IsTerminating())
			break;

		// Print the active thread, if it changed.
		if (fContext.CurrentThreadID() != currentThread) {
			fContext.PrintCurrentThread();
			currentThread = fContext.CurrentThreadID();
		}

		// read a command line
		const char* line = fContext.PromptUser(kDebuggerPrompt);
		if (line == NULL)
			break;

		// parse the command line
		ArgumentVector args;
		const char* parseErrorLocation;
		switch (args.Parse(line, &parseErrorLocation)) {
			case ArgumentVector::NO_ERROR:
				break;
			case ArgumentVector::NO_MEMORY:
				printf("Insufficient memory parsing the command line.\n");
				continue;
			case ArgumentVector::UNTERMINATED_QUOTED_STRING:
				printf("Parse error: Unterminated quoted string starting at "
					"character %zu.\n", parseErrorLocation - line + 1);
				continue;
			case ArgumentVector::TRAILING_BACKSPACE:
				printf("Parse error: trailing backspace.\n");
				continue;
		}

		if (args.ArgumentCount() == 0)
			continue;

		// add line to history
		fContext.AddLineToInputHistory(line);

		// execute command
		_ExecuteCommand(args.ArgumentCount(), args.Arguments());
	}

	return B_OK;
}


status_t
CommandLineUserInterface::_RegisterCommands()
{
	if (_RegisterCommand("help", new(std::nothrow) HelpCommand(this)) &&
		_RegisterCommand("quit", new(std::nothrow) CliQuitCommand) &&
		_RegisterCommand("threads", new(std::nothrow) CliThreadsCommand)) {
		return B_OK;
	}

	return B_NO_MEMORY;
}


bool
CommandLineUserInterface::_RegisterCommand(const BString& name,
	CliCommand* command)
{
	BReference<CliCommand> commandReference(command, true);
	if (name.IsEmpty() || command == NULL)
		return false;

	CommandEntry* entry = new(std::nothrow) CommandEntry(name, command);
	if (entry == NULL || !fCommands.AddItem(entry)) {
		delete entry;
		return false;
	}

	return true;
}


void
CommandLineUserInterface::_ExecuteCommand(int argc, const char* const* argv)
{
	CommandEntry* commandEntry = _FindCommand(argv[0]);
	if (commandEntry != NULL)
		commandEntry->Command()->Execute(argc, argv, fContext);
}


CommandLineUserInterface::CommandEntry*
CommandLineUserInterface::_FindCommand(const char* commandName)
{
	size_t commandNameLength = strlen(commandName);

	// try to find an exact match first
	CommandEntry* commandEntry = NULL;
	for (int32 i = 0; CommandEntry* entry = fCommands.ItemAt(i); i++) {
		if (entry->Name() == commandName) {
			commandEntry = entry;
			break;
		}
	}

	// If nothing found yet, try partial matches, but only, if they are
	// unambiguous.
	if (commandEntry == NULL) {
		for (int32 i = 0; CommandEntry* entry = fCommands.ItemAt(i); i++) {
			if (entry->Name().Compare(commandName, commandNameLength) == 0) {
				if (commandEntry != NULL) {
					printf("Error: Ambiguous command \"%s\".\n", commandName);
					return NULL;
				}

				commandEntry = entry;
			}
		}
	}

	if (commandEntry == NULL) {
		printf("Error: Unknown command \"%s\".\n", commandName);
		return NULL;
	}

	return commandEntry;
}


void
CommandLineUserInterface::_PrintHelp(const char* commandName)
{
	// If a command name is given, print the usage for that one.
	if (commandName != NULL) {
		CommandEntry* commandEntry = _FindCommand(commandName);
		if (commandEntry != NULL)
			commandEntry->Command()->PrintUsage(commandEntry->Name().String());
		return;
	}

	// No command name given -- print a list of all commands.

	// determine longest command name
	int32 longestCommandName = 0;
	for (int32 i = 0; CommandEntry* entry = fCommands.ItemAt(i); i++) {
		longestCommandName
			= std::max(longestCommandName, entry->Name().Length());
	}

	// print the command list
	for (int32 i = 0; CommandEntry* entry = fCommands.ItemAt(i); i++) {
		printf("%*s  -  %s\n", (int)longestCommandName, entry->Name().String(),
			entry->Command()->Summary());
	}
}
