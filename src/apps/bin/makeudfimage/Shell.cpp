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
#include "UdfDebug.h"
#include "UdfBuilder.h"

Shell::Shell()
	// The following settings are essentially default values
	// for all the command-line options.
	: fVerbosityLevel(VERBOSITY_HIGH)
	, fBlockSize(2048)
	, fDoUdf(true)
	, fDoIso(true)
	, fOutputFile("")
{
}

status_t
Shell::Run(int argc, char *argv[])
{
	DEBUG_INIT("Shell");
	status_t error = _ProcessArguments(argc, argv);
	if (!error) {
		ConsoleListener listener(fVerbosityLevel);
		UdfBuilder builder(fOutputFile.c_str(), fBlockSize, fDoUdf, fDoIso,
		                   "(Unnamed UDF volume)", "ISO_VOLUME",
		                   "/boot/home/Desktop/udftest", listener);
		error = builder.InitCheck();
		if (!error) 
			error = builder.Build();
	}
	
	if (error)
		_PrintHelp();

	RETURN(error);
}

status_t
Shell::_ProcessArguments(int argc, char *argv[]) {
	DEBUG_INIT_ETC("Shell", ("argc: %d", argc));
	
	// Throw all the arguments into a handy list
	std::list<std::string> argumentList;
	for (int i = 1; i < argc; i++) 
		argumentList.push_back(std::string(argv[i]));

	bool foundOutputFile = false;
		
	// Now bust out some processing
	int argumentCount = argumentList.size();
	int index = 0;
	for(std::list<std::string>::iterator i = argumentList.begin();
	      i != argumentList.end();
	      	)
	{
		std::string &arg = *i;
		if (arg == "--help") {
			RETURN(B_ERROR);
		} else if (arg == "-v0") {
			fVerbosityLevel = VERBOSITY_NONE;
		} else if (arg == "-v1") {
			fVerbosityLevel = VERBOSITY_LOW;
		} else if (arg == "-v2") {
			fVerbosityLevel = VERBOSITY_MEDIUM;
		} else if (arg == "-v3") {
			fVerbosityLevel = VERBOSITY_HIGH;
		} else {
			if (index == argumentCount-1) {
				// Take this argument as the output filename
				fOutputFile = arg;
				foundOutputFile = true;
			} else {
				printf("ERROR: invalid argument `%s'\n", arg.c_str());
				printf("\n");
				RETURN(B_ERROR);
			}
		}
		i++;
		index++;
	}
	
	if (!foundOutputFile) {
		printf("ERROR: no output file specified\n");
		printf("\n");
		RETURN(B_ERROR);
	}	
	
	RETURN(B_OK);
}

void
Shell::_PrintHelp() {
	printf("usage: makeudfimage [options] <output-filename>\n");
	printf("\n");
	printf("VALID ARGUMENTS:\n");
	printf("  --help:   Displays this help text\n");
	printf("  -v0:      Sets verbosity level to 0 (silent)\n"); 
	printf("  -v1:      Sets verbosity level to 1 (low)\n"); 
	printf("  -v2:      Sets verbosity level to 2 (medium)\n"); 
	printf("  -v3:      Sets verbosity level to 3 (high, *default*)\n");
	printf("\n");
}
