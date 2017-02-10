//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
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
	, fUdfRevision(0x0201)
	, fTruncate(true)
{
}

status_t
Shell::Run(int argc, char *argv[])
{
	DEBUG_INIT("Shell");
	status_t error = _ProcessArguments(argc, argv);
	if (!error) {
		if (fUdfVolumeName == "")
			fUdfVolumeName = "(Unnamed UDF Volume)";
		ConsoleListener listener(fVerbosityLevel);
		UdfBuilder builder(fOutputFile.c_str(), fBlockSize, fDoUdf, 
		                   fUdfVolumeName.c_str(), fUdfRevision, fDoIso, "ISO_VOLUME",
		                   fSourceDirectory.c_str(), listener, fTruncate);
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
		if (arg == "-h" || arg == "--help") {
			_PrintTitle();
			RETURN(B_ERROR);
		} else if (arg == "-v0" || arg == "--quiet") {
			fVerbosityLevel = VERBOSITY_NONE;
		} else if (arg == "-v1") {
			fVerbosityLevel = VERBOSITY_LOW;
		} else if (arg == "-v2") {
			fVerbosityLevel = VERBOSITY_MEDIUM;
		} else if (arg == "-v3") {
			fVerbosityLevel = VERBOSITY_HIGH;
		} else if (arg == "-r" || arg == "--revision") {
			i++;
			index++;
			if (*i == "1.50") 
				fUdfRevision = 0x0150;
			else if (*i == "2.01")
				fUdfRevision = 0x0201;
			else {
				printf("ERROR: invalid UDF revision `%s'; please specify `1.50' "
				       "or `2.01'\n", i->c_str());
				RETURN(B_ERROR);
			}
		} else if (arg == "-t" || arg == "--no-truncate") {
			fTruncate = 0;
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
	printf("example: makeudfimage /boot/home/mail mail.udf \"Mail Backup\"\n");
	printf("\n");
	printf("VALID OPTIONS:\n");
	printf("  -h, --help                      Displays this help text.\n");
	printf("  --quiet                         Turns off console output.\n");
	printf("  -r, --revision <udf-revision>   Selects the UDF revision to use. Supported\n");
	printf("                                  revisions are 1.50 and 2.01. Defaults to 2.01.\n");
	printf("  -t, --no-trunc                  Don't truncate output file if it already\n");
	printf("                                  exists.\n");
	printf("\n");
}

#define MAKEUDFIMAGE_VERSION "1.0.0"
#ifndef MAKEUDFIMAGE_VERSION
#	define MAKEUDFIMAGE_VERSION ("development version " __DATE__ ", " __TIME__)
#endif

void
Shell::_PrintTitle() {
	printf("makeudfimage %s\n", MAKEUDFIMAGE_VERSION);
	printf("Copyright © 2004 Tyler Dauwalder\n");
	printf("\n");
}
