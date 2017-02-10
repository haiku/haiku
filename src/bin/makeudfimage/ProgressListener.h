//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ProgressListener.h

	Interface for receiving progress information updates from an
	executing UdfBuilder object.
*/

#ifndef _PROGRESS_LISTENER_H
#define _PROGRESS_LISTENER_H

#include "Statistics.h"

enum VerbosityLevel {
	VERBOSITY_NONE,
	VERBOSITY_LOW,
	VERBOSITY_MEDIUM,
	VERBOSITY_HIGH,
};

class ProgressListener {
public:
	virtual void OnStart(const char *sourceDirectory, const char *outputFile,
	                     const char *udfVolumeName, uint16 udfRevision) const = 0;
	virtual void OnError(const char *message) const = 0;
	virtual void OnWarning(const char *message) const = 0;
	virtual void OnUpdate(VerbosityLevel level, const char *message) const = 0;
	virtual void OnCompletion(status_t result, const Statistics &statistics) const = 0;		
};

#endif	// _PROGRESS_LISTENER_H
