/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "pkgman.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <package/manager/Exceptions.h>

#include "Command.h"


using namespace BPackageKit::BManager::BPrivate;


extern const char* __progname;
const char* kProgramName = __progname;


static const char* const kUsage =
	"Usage: %s <command> <command args>\n"
	"Manages packages and package repositories.\n"
	"\n"
	"Package management commands:\n"
	"%s"
	"Repository management commands:\n"
	"%s"
	"Other commands:\n"
	"%s"
	"Common options:\n"
	"  -h, --help   - Print usage info for a specific command.\n"
;


static BString
get_commands_usage_for_category(const char* category)
{
	BString commandsUsage;
	CommandList commands;
	CommandManager::Default()->GetCommandsForCategory(category, commands);
	for (int32 i = 0; Command* command = commands.ItemAt(i); i++)
		commandsUsage << command->ShortUsage() << '\n';
	return commandsUsage;
}


void
print_usage_and_exit(bool error)
{
	BString packageCommandsUsage
		= get_commands_usage_for_category(COMMAND_CATEGORY_PACKAGES);
	BString repositoryCommandsUsage
		= get_commands_usage_for_category(COMMAND_CATEGORY_REPOSITORIES);
	BString otherCommandsUsage
		= get_commands_usage_for_category(COMMAND_CATEGORY_OTHER);

    fprintf(error ? stderr : stdout, kUsage, kProgramName,
    	packageCommandsUsage.String(), repositoryCommandsUsage.String(),
    	otherCommandsUsage.String());

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

	try {
		return commands.ItemAt(0)->Execute(argc - 1, argv + 1);
	} catch (BNothingToDoException&) {
		fprintf(stderr, "Nothing to do.\n");
		return 0;
	} catch (std::bad_alloc&) {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	} catch (BFatalErrorException& exception) {
		if (!exception.Details().IsEmpty())
			fprintf(stderr, "%s", exception.Details().String());
		if (exception.Error() == B_OK) {
			fprintf(stderr, "*** %s\n", exception.Message().String());
		} else {
			fprintf(stderr, "*** %s: %s\n", exception.Message().String(),
				strerror(exception.Error()));
		}
		return 1;
	} catch (BAbortedByUserException&) {
		return 0;
	}
}
