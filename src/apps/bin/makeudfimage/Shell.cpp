//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Shell.cpp

	Command-line shell for makeudfimage
*/

#include "Shell.h"

#include <stdio.h>

#include "Debug.h"

Shell::Shell()
{
}

void
Shell::Run(int argc, char *argv[])
{
	DEBUG_INIT("Shell");
	_ProcessArguments(argc, argv);
	PRINT(("finished\n"));
}

status_t
Shell::_ProcessArguments(int argc, char *argv[]) {
	DEBUG_INIT_ETC("Shell", ("argc: %d", argc));
	
	// If we're given no parameters, the default settings
	// will do just fine
	if (argc < 2)
		return B_OK;

	// Handle each command line argument (skipping the first
	// which is just the app name)
	for (int i = 1; i < argc; i++) {
		std::string str(argv[i]);
		
		status_t error = _ProcessArgument(str, argc, argv);
		if (error)
			RETURN(error);		
	}
	
	RETURN(B_OK);
}

status_t
Shell::_ProcessArgument(std::string arg, int argc, char *argv[]) {
	DEBUG_INIT_ETC("Shell", ("arg: `%s'", arg.c_str()));
	if (arg == "--help") {
		_PrintHelp();
		RETURN(B_ERROR);
	} 
	RETURN(B_ERROR);
}			

void
Shell::_PrintHelp() {
	printf("usage: makeudfimage\n");
}
