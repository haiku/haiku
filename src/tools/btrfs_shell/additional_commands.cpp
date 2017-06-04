#include "fssh.h"

#include "command_cat.h"


namespace FSShell {


void
register_additional_commands()
{
	CommandManager::Default()->AddCommands(
		command_cat,	"cat",	"concatenate file(s) to stdout",
		NULL
	);
}


}	// namespace FSShell
