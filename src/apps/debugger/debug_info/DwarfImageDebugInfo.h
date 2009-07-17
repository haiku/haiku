/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_IMAGE_DEBUG_INFO_H
#define DWARF_IMAGE_DEBUG_INFO_H


#include <Locker.h>

#include <util/OpenHashTable.h>

#include "ImageInfo.h"
#include "SpecificImageDebugInfo.h"
#include "Type.h"


class Architecture;
class CompilationUnit;
class DwarfFile;
class ElfSegment;
class FileManager;
class FileSourceCode;
class LocatableFile;
class SourceCode;
class TeamMemory;


class DwarfImageDebugInfo : public SpecificImageDebugInfo {
public:
								DwarfImageDebugInfo(const ImageInfo& imageInfo,
									Architecture* architecture,
									TeamMemory* teamMemory,
									FileManager* fileManager, DwarfFile* file);
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
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement);
	virtual	status_t			GetStatementAtSourceLocation(
									FunctionDebugInfo* function,
									const SourceLocation& sourceLocation,
									Statement*& _statement);

	virtual	status_t			GetSourceLanguage(FunctionDebugInfo* function,
									SourceLanguage*& _language);

	virtual	ssize_t				ReadCode(target_addr_t address, void* buffer,
									size_t size);

	virtual	status_t			AddSourceCodeInfo(LocatableFile* file,
									FileSourceCode* sourceCode);

private:
			struct UnwindTargetInterface;

private:
			status_t 			_AddSourceCodeInfo(CompilationUnit* unit,
									FileSourceCode* sourceCode,
									int32 fileIndex);
			int32				_GetSourceFileIndex(CompilationUnit* unit,
									LocatableFile* sourceFile) const;

private:
			BLocker				fLock;
			ImageInfo			fImageInfo;
			Architecture*		fArchitecture;
			TeamMemory*			fTeamMemory;
			FileManager*		fFileManager;
			DwarfFile*			fFile;
			ElfSegment*			fTextSegment;
			target_addr_t		fRelocationDelta;
};


#endif	// DWARF_IMAGE_DEBUG_INFO_H
