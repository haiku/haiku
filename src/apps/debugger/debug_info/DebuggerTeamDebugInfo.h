/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_TEAM_DEBUG_INFO_H
#define DEBUGGER_TEAM_DEBUG_INFO_H

#include "SpecificTeamDebugInfo.h"


class Architecture;
class DebuggerInterface;
class ImageInfo;


class DebuggerTeamDebugInfo : public SpecificTeamDebugInfo {
public:
								DebuggerTeamDebugInfo(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture);
	virtual						~DebuggerTeamDebugInfo();

			status_t			Init();

	virtual	status_t			CreateImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									SpecificImageDebugInfo*& _imageDebugInfo);

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
};


#endif	// DEBUGGER_TEAM_DEBUG_INFO_H
