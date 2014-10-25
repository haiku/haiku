/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Command.h"

#include <stdio.h>
#include <stdlib.h>


static int
compare_commands_by_name(const Command* a, const Command* b)
{
	return a->Name().Compare(b->Name());
}


// #pragma mark - Command


Command::Command(const BString& name, const BString& shortUsage,
	const BString& longUsage, const BString& category)
	:
	fCommonOptions(),
	fName(name),
	fShortUsage(shortUsage),
	fLongUsage(longUsage),
	fCategory(category)
{
	fShortUsage.ReplaceAll("%command%", fName);
	fLongUsage.ReplaceAll("%command%", fName);
}


Command::~Command()
{
}


void
Command::Init(const char* programName)
{
	fShortUsage.ReplaceAll("%program%", programName);
	fLongUsage.ReplaceAll("%program%", programName);
}


void
Command::PrintUsage(bool error) const
{
	fprintf(error ? stderr : stdout, "%s", fLongUsage.String());
}


void
Command::PrintUsageAndExit(bool error) const
{
	PrintUsage(error);
	exit(error ? 1 : 0);
}


// #pragma mark - CommandManager


/*static*/ CommandManager*
CommandManager::Default()
{
	static CommandManager* manager = new CommandManager;
	return manager;
}


void
CommandManager::RegisterCommand(Command* command)
{
	fCommands.AddItem(command);
}


void
CommandManager::InitCommands(const char* programName)
{
	for (int32 i = 0; Command* command = fCommands.ItemAt(i); i++)
		command->Init(programName);

	fCommands.SortItems(&compare_commands_by_name);
}


void
CommandManager::GetCommands(const char* prefix, CommandList& _commands)
{
	for (int32 i = 0; Command* command = fCommands.ItemAt(i); i++) {
		if (command->Name().StartsWith(prefix))
			_commands.AddItem(command);
	}
}


void
CommandManager::GetCommandsForCategory(const char* category,
	CommandList& _commands)
{
	for (int32 i = 0; Command* command = fCommands.ItemAt(i); i++) {
		if (command->Category() == category)
			_commands.AddItem(command);
	}
}


CommandManager::CommandManager()
	:
	fCommands(20, true)
{
}
