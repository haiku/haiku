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

			int32				CountFunctions() const;
			FunctionDebugInfo*	FunctionAt(int32 index) const;
			FunctionDebugInfo*	FunctionAtAddress(target_addr_t address) const;

private:
			typedef BObjectList<DebugInfo> DebugInfoList;
			typedef BObjectList<FunctionDebugInfo> FunctionList;

private:
	static	int					_CompareFunctions(const FunctionDebugInfo* a,
									const FunctionDebugInfo* b);
	static	int					_CompareAddressFunction(
									const target_addr_t* address,
									const FunctionDebugInfo* function);

private:
			ImageInfo			fImageInfo;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			DebugInfoList		fDebugInfos;
			FunctionList		fFunctions;
};


#endif	// IMAGE_DEBUG_INFO_H
