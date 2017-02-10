//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ConsoleListener.h

	Declarations for a console-based implementation of ProgressListener
	interface.
*/

#ifndef _CONSOLE_LISTENER_H
#define _CONSOLE_LISTENER_H

#include "ProgressListener.h"

class ConsoleListener : public ProgressListener {
public:
	ConsoleListener(VerbosityLevel level);
	virtual void OnStart(const char *sourceDirectory, const char *outputFile,
	                     const char *udfVolumeName, uint16 udfRevision) const;
	virtual void OnError(const char *message) const;
	virtual void OnWarning(const char *message) const;
	virtual void OnUpdate(VerbosityLevel level, const char *message) const;
	virtual void OnCompletion(status_t result, const Statistics &statistics) const;
	
	VerbosityLevel Level() const { return fLevel; }
private:
	VerbosityLevel fLevel;
};

#endif	// _CONSOLE_LISTENER_H
