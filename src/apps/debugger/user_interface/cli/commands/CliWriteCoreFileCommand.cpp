/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CliWriteCoreFileCommand.h"

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include "CliContext.h"
#include "UiUtils.h"
#include "UserInterface.h"


CliWriteCoreFileCommand::CliWriteCoreFileCommand()
	:
	CliCommand("write core file",
		"%s\n"
		"Writes a core dump file for the current team.")
{
}


void
CliWriteCoreFileCommand::Execute(int argc, const char* const* argv, CliContext& context)
{
	BPath path;
	if (argc > 1) {
		path.SetTo(argv[1]);
		if (path.InitCheck() != B_OK) {
			printf("Invalid core file path %s given.\n", argv[1]);
			return;
		}
	} else {
		char buffer[B_FILE_NAME_LENGTH];
		UiUtils::CoreFileNameForTeam(context.GetTeam(), buffer,
			sizeof(buffer));
		find_directory(B_DESKTOP_DIRECTORY, &path);
		path.Append(buffer);
	}

	entry_ref ref;
	if (get_ref_for_path(path.Path(), &ref) == B_OK) {
		printf("Writing core file to %s...\n", path.Path());
		context.GetUserInterfaceListener()->WriteCoreFileRequested(&ref);
	}
}
