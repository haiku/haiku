/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_IMAGE_DEBUG_INFO_H
#define DWARF_IMAGE_DEBUG_INFO_H

#include "ImageInfo.h"
#include "SpecificImageDebugInfo.h"


class Architecture;
class DwarfFile;
class ElfSegment;


class DwarfImageDebugInfo : public SpecificImageDebugInfo {
public:
								DwarfImageDebugInfo(const ImageInfo& imageInfo,
									Architecture* architecture,
									DwarfFile* file);
	virtual						~DwarfImageDebugInfo();

			status_t			Init();

			target_addr_t		RelocationDelta() const
									{ return fRelocationDelta; }

	virtual	status_t			GetFunctions(
									BObjectList<FunctionDebugInfo>& functions);
	virtual	status_t			CreateFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState);
	virtual	status_t			LoadSourceCode(FunctionDebugInfo* function,
									SourceCode*& _sourceCode);
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement);

private:
			ImageInfo			fImageInfo;
			Architecture*		fArchitecture;
			DwarfFile*			fFile;
			ElfSegment*			fTextSegment;
			target_addr_t		fRelocationDelta;
};


#endif	// DWARF_IMAGE_DEBUG_INFO_H
