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
	// for all the command-line options (except for the last
	// three vars).
	: fVerbosityLevel(VERBOSITY_LOW)
	, fBlockSize(2048)
	, fDoUdf(true)
	, fDoIso(false)
	, fSourceDirectory("")
	, fOutputFile("")
	, fUdfVolumeName("")
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
		                   fUdfVolumeName.c_str(), "ISO_VOLUME",
		                   fSourceDirectory.c_str(), listener);
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

	bool foundSourceDirectory = false;
	bool foundOutputFile = false;
	bool foundUdfVolumeName = false;
		
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
		} else if (arg == "-v0" || arg == "--quiet") {
			fVerbosityLevel = VERBOSITY_NONE;
		} else if (arg == "-v1") {
			fVerbosityLevel = VERBOSITY_LOW;
		} else if (arg == "-v2") {
			fVerbosityLevel = VERBOSITY_MEDIUM;
		} else if (arg == "-v3") {
			fVerbosityLevel = VERBOSITY_HIGH;
		} else {
			if (index == argumentCount-3) {
				// Take this argument as the source dir
				fSourceDirectory = arg;
				foundSourceDirectory = true;
			} else if (index == argumentCount-2) {
				// Take this argument as the output filename
				fOutputFile = arg;
				foundOutputFile = true;
			} else if (index == argumentCount-1) {
				// Take this argument as the udf volume name
				fUdfVolumeName = arg;
				foundUdfVolumeName = true;
			} else {
				printf("ERROR: invalid argument `%s'\n", arg.c_str());
				printf("\n");
				RETURN(B_ERROR);
			}
		}
		i++;
		index++;
	}
	
	status_t error = B_OK;
	if (!foundSourceDirectory) {
		printf("ERROR: no source directory specified\n");
		error = B_ERROR;
	}	
	if (!foundOutputFile) {
		printf("ERROR: no output file specified\n");
		error = B_ERROR;
	}	
	if (!foundUdfVolumeName) {
		printf("ERROR: no volume name specified\n");
		error = B_ERROR;
	}	
	
	if (error)
		printf("\n");
	RETURN(error);
}

void
Shell::_PrintHelp() {
	printf("usage: makeudfimage [options] <source-directory> <output-file> <udf-volume-name>\n");
	printf("\n");
	printf("VALID ARGUMENTS:\n");
	printf("  --help:   Displays this help text\n");
	printf("  --quiet:  Turns off console output\n");
	printf("\n");
}
