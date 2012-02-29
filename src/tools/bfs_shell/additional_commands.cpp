/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "fssh.h"

#include "command_checkfs.h"


namespace FSShell {


void
register_additional_commands()
{
	puts("************************ HERE I AM!!!!!!!!!!!!!!!!!");
	CommandManager::Default()->AddCommand(command_checkfs, "checkfs",
		"check file system");
}


}	// namespace FSShell
