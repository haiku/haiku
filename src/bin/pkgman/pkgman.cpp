/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "pkgman.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Command.h"


extern const char* __progname;
const char* kProgramName = __progname;


static const char* const kUsage =
	"Usage: %s <command> <command args>\n"
	"Manages packages and package repository.\n"
	"\n"
	"Commands:\n"
	"%s"
	"Common Options:\n"
	"  -h, --help   - Print this usage info.\n"
;


void
print_usage_and_exit(bool error)
{
	BString commandsUsage;
	const CommandList& commands = CommandManager::Default()->Commands();
	for (int32 i = 0; Command* command = commands.ItemAt(i); i++)
		commandsUsage << command->ShortUsage() << '\n';

    fprintf(error ? stderr : stdout, kUsage, kProgramName,
    	commandsUsage.String());

    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	CommandManager::Default()->InitCommands(kProgramName);

	if (argc < 2)
		print_usage_and_exit(true);

	const char* command = argv[1];
	if (strcmp(command, "help") == 0)
		print_usage_and_exit(false);

	CommandList commands;
	CommandManager::Default()->GetCommands(command, commands);
	if (commands.CountItems() != 1)
		print_usage_and_exit(true);

	return commands.ItemAt(0)->Execute(argc - 1, argv + 1);
}
