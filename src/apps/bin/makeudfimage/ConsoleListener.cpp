//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ConsoleListener.cpp

	Console-based implementation of ProgressListener interface.
*/

#include "ConsoleListener.h"

#include <stdio.h>
#include <string.h>

/*! \brief Creates a new ConsoleListener object with the given verbosity level.

	All output from said listener is sent to standard output via printf().

	\param level All update messages with verbosity levels below this value
	             will be ignored. If level is \c VERBOSITY_NONE, no output
	             whatsoever (including errors and warnings) will be
	             generated.
*/	                    
ConsoleListener::ConsoleListener(VerbosityLevel level)
	: fLevel(level)
{
}

void
ConsoleListener::OnError(const char *message) const 
{
	if (Level() > VERBOSITY_NONE)
		printf("ERROR: %s\n", message);
}

void
ConsoleListener::OnWarning(const char *message) const
{
	if (Level() > VERBOSITY_NONE)
		printf("WARNING: %s\n", message);
}

void
ConsoleListener::OnUpdate(VerbosityLevel level, const char *message) const
{
	if (Level() > VERBOSITY_NONE && level <= Level()) {
		switch (level) {
			case VERBOSITY_MEDIUM:
				printf("  ");
				break;
			case VERBOSITY_HIGH:
				printf("    ");
				break;
			default:
				break;
		}
		printf("%s\n", message);
	}
}

void
ConsoleListener::OnCompletion(status_t result, const Statistics &statistics) const
{
	if (Level() > VERBOSITY_NONE) {
		if (result == B_OK) {
			printf("Finished\n");
			printf("- Build time:  %ld seconds\n", statistics.ElapsedTime());
			printf("- Directories: %Ld directories in %s\n",
			       statistics.Directories(), statistics.DirectoryBytesString().c_str());
			printf("- Files:       %Ld files in %s\n",
			       statistics.Files(), statistics.FileBytesString().c_str());
			printf("- Image size:  %s\n", statistics.ImageSizeString().c_str());
		} else {
			printf("----------------------------------------------------------------------\n");
			printf("Build failed with error: 0x%lx, `%s'\n", result,
			       strerror(result));
			printf("----------------------------------------------------------------------\n");
		}	
	}
}
