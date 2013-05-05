/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CliDebugReportCommand.h"

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include "CliContext.h"
#include "UiUtils.h"
#include "UserInterface.h"


CliDebugReportCommand::CliDebugReportCommand()
	:
	CliCommand("save debug report",
		"%s\n"
		"Saves a debug information report for the current team.")
{
}


void
CliDebugReportCommand::Execute(int argc, const char* const* argv, CliContext& context)
{
	BPath path;
	if (argc > 1) {
		path.SetTo(argv[1]);
		if (path.InitCheck() != B_OK) {
			printf("Invalid report path %s given.\n", argv[1]);
			return;
		}
	} else {
		char buffer[B_FILE_NAME_LENGTH];
		UiUtils::ReportNameForTeam(context.GetTeam(), buffer, sizeof(buffer));
		find_directory(B_DESKTOP_DIRECTORY, &path);
		path.Append(buffer);
	}

	entry_ref ref;
	if (get_ref_for_path(path.Path(), &ref) == B_OK) {
		printf("Saving debug information report to %s...\n", path.Path());
		context.GetUserInterfaceListener()->DebugReportRequested(&ref);
	}
}
