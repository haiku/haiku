/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_INFO_H
#define IMAGE_DEBUG_INFO_H

#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>

#include "ArchitectureTypes.h"
#include "ImageInfo.h"


class Architecture;
class DebuggerInterface;
class DebugInfo;
class FunctionDebugInfo;


class ImageDebugInfo : public Referenceable {
public:
								ImageDebugInfo(const ImageInfo& imageInfo,
									DebuggerInterface* debuggerInterface,
									Architecture* architecture);
								~ImageDebugInfo();

			status_t			Init();

			FunctionDebugInfo*	FindFunction(target_addr_t address);
									// returns a reference

private:
			typedef BObjectList<DebugInfo> DebugInfoList;

private:
			ImageInfo			fImageInfo;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			DebugInfoList		fDebugInfos;
};


#endif	// IMAGE_DEBUG_INFO_H
