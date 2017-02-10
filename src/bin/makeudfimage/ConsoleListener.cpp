//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ConsoleListener.cpp

	Console-based implementation of ProgressListener interface.
*/

#include "ConsoleListener.h"

#include <stdio.h>
#include <string.h>

static const char * const kDivider =
	"----------------------------------------------------------------------";

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
ConsoleListener::OnStart(const char *sourceDirectory, const char *outputFile,
	                     const char *udfVolumeName, uint16 udfRevision) const
{
	if (Level() > VERBOSITY_NONE) {
		printf("%s\n", kDivider);
		printf("Source directory: `%s'\n", sourceDirectory);
		printf("Output file:      `%s'\n", outputFile);
		printf("UDF Volume Name:  `%s'\n", udfVolumeName);
		printf("UDF Revision:     %01x.%01x%01x\n",
		                          (udfRevision & 0x0f00) >> 8,
		                          (udfRevision & 0x00f0) >> 4,
		                          (udfRevision & 0x000f));
		printf("%s\n", kDivider);
	}
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
			uint64 directories = statistics.Directories();
			uint64 files = statistics.Files();
			uint64 symlinks = statistics.Symlinks();
			printf("Finished\n");
			printf("- Build time:  %s\n", statistics.ElapsedTimeString().c_str());
			printf("- Directories: %Ld director%s in %s\n",
			       directories, directories == 1 ? "y" : "ies",
			       statistics.DirectoryBytesString().c_str());
			printf("- Files:       %Ld file%s in %s\n",
			       files, files == 1 ? "" : "s",
			       statistics.FileBytesString().c_str());
			if (symlinks > 0)
				printf("- Symlinks:    No symlink support yet; %Ld symlink%s ommitted\n", symlinks,
				       symlinks == 1 ? "" : "s");
			printf("- Image size:  %s\n", statistics.ImageSizeString().c_str());
		} else {
			printf("----------------------------------------------------------------------\n");
			printf("Build failed with error: 0x%lx, `%s'\n", result,
			       strerror(result));
			printf("----------------------------------------------------------------------\n");
		}	
	}
}
