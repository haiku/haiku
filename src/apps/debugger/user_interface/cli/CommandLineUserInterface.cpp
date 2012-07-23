/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CommandLineUserInterface.h"

#include <stdio.h>

#include <algorithm>

#include <readline/history.h>
#include <readline/readline.h>

#include <ArgumentVector.h>
#include <AutoDeleter.h>
#include <Referenceable.h>

#include "CliCommand.h"
#include "CliContext.h"


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
		CliCommand("print a list of all commands",
			"%s\n"
			"Prints a list of all commands."),
		fUserInterface(userInterface)
	{
	}

	virtual void Execute(int argc, const char* const* argv, CliContext& context)
	{
		fUserInterface->_PrintHelp();
	}

private:
	CommandLineUserInterface* fUserInterface;
};


// #pragma mark - HelpCommand


struct CommandLineUserInterface::QuitCommand : CliCommand {
	QuitCommand(CommandLineUserInterface* userInterface)
		:
		CliCommand("quit Debugger",
			"%s\n"
			"Quits Debugger."),
		fUserInterface(userInterface)
	{
	}

	virtual void Execute(int argc, const char* const* argv, CliContext& context)
	{
		fUserInterface->fListener->UserInterfaceQuitRequested();
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
	fContext.Init(team, listener);

	status_t error = _RegisterCommands();
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
		// TODO: Signal the thread so it wakes up!

		// Wait for input loop to finish.
		while (acquire_sem(fShowSemaphore) == B_INTERRUPTED) {
		}
	} else {
		// The main thread will still be blocked in Run(). Unblock it.
		delete_sem(fShowSemaphore);
		fShowSemaphore = -1;
	}
}


status_t
CommandLineUserInterface::LoadSettings(const TeamUISettings* settings)
{
	return B_OK;
}


status_t
CommandLineUserInterface::SaveSettings(TeamUISettings*& settings) const
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
	return 0;
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
	while (!fTerminating) {
		// read a command line
		char* line = readline("debugger> ");
		if (line == NULL)
			break;
		MemoryDeleter lineDeleter(line);

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

		add_history(line);

		_ExecuteCommand(args.ArgumentCount(), args.Arguments());
	}

	return B_OK;
}


status_t
CommandLineUserInterface::_RegisterCommands()
{
	if (_RegisterCommand("help", new(std::nothrow) HelpCommand(this)) &&
		_RegisterCommand("quit", new(std::nothrow) QuitCommand(this))) {
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
	const char* commandName = argv[0];
	size_t commandNameLength = strlen(commandName);

	CommandEntry* firstEntry = NULL;
	for (int32 i = 0; CommandEntry* entry = fCommands.ItemAt(i); i++) {
		if (entry->Name().Compare(commandName, commandNameLength) == 0) {
			if (firstEntry != NULL) {
				printf("Ambiguous command \"%s\".\n", commandName);
				return;
			}

			firstEntry = entry;
		}
	}

	if (firstEntry == NULL) {
		printf("Unknown command \"%s\".\n", commandName);
		return;
	}

	firstEntry->Command()->Execute(argc, argv, fContext);
}


void
CommandLineUserInterface::_PrintHelp()
{
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
