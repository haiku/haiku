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

#include "ConsoleListener.h"
#include "Debug.h"
#include "UdfBuilder.h"

Shell::Shell()
	: fVerbosityLevel(VERBOSITY_HIGH)
{
}

status_t
Shell::Run(int argc, char *argv[])
{
	DEBUG_INIT("Shell");
	status_t error = _ProcessArguments(argc, argv);
	if (!error) {
		ConsoleListener listener(fVerbosityLevel);
		UdfBuilder builder(listener);
		builder.Build();
	} else {
		_PrintHelp();
	}
	RETURN(error);
}

status_t
Shell::_ProcessArguments(int argc, char *argv[]) {
	DEBUG_INIT_ETC("Shell", ("argc: %d", argc));
	
	// If we're given no parameters, the default settings
	// will do just fine
	if (argc < 2)
		RETURN(B_OK);

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
		RETURN(B_ERROR);
	} else if (arg == "-v0") {
		fVerbosityLevel = VERBOSITY_NONE;
	} else if (arg == "-v1") {
		fVerbosityLevel = VERBOSITY_LOW;
	} else if (arg == "-v2") {
		fVerbosityLevel = VERBOSITY_HIGH;
	} else {
		printf("ERROR: invalid argument `%s'\n", arg.c_str());
		printf("\n");
		RETURN(B_ERROR);
	}
	RETURN(B_OK);
}			

void
Shell::_PrintHelp() {
	printf("usage: makeudfimage [options]\n");
	printf("\n");
	printf("VALID ARGUMENTS:\n");
	printf("  --help:   Displays this help text\n");
	printf("  -v0:      Sets verbosity level to 0 (silent)\n"); 
	printf("  -v1:      Sets verbosity level to 1 (low)\n"); 
	printf("  -v2:      Sets verbosity level to 2 (high, *default*)\n");
	printf("\n");
}
