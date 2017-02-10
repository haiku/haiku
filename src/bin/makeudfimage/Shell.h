//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Shell.h
*/

#ifndef _SHELL_H
#define _SHELL_H

#include <string>
#include <SupportDefs.h>

#include "ProgressListener.h"

class Shell {
public:
	Shell();
	status_t Run(int argc, char *argv[]);
	
private:
	status_t _ProcessArguments(int argc, char *argv[]);
	void _PrintHelp();
	void _PrintTitle();
	
	VerbosityLevel fVerbosityLevel;
	uint32 fBlockSize;
	bool fDoUdf;
	bool fDoIso;
	std::string fSourceDirectory;
	std::string fOutputFile;
	std::string fUdfVolumeName;
	uint16 fUdfRevision;
	bool fTruncate;
};


#endif	// _SHELL_H
