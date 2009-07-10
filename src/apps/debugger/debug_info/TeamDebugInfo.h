/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUG_INFO_H
#define TEAM_DEBUG_INFO_H


#include <ObjectList.h>
#include <Referenceable.h>
#include <util/OpenHashTable.h>

#include "ImageInfo.h"


class Architecture;
class DebuggerInterface;
class FileManager;
class Function;
class FunctionInstance;
class ImageDebugInfo;
class ImageInfo;
class LocatableFile;
class SourceLocation;
class SpecificTeamDebugInfo;


class TeamDebugInfo : public Referenceable {
public:
								TeamDebugInfo(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture,
									FileManager* fileManager);
								~TeamDebugInfo();

			status_t			Init();

			status_t			LoadImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									ImageDebugInfo*& _imageDebugInfo);

			// team is locked
			status_t			AddImageDebugInfo(
									ImageDebugInfo* imageDebugInfo);
			void				RemoveImageDebugInfo(
									ImageDebugInfo* imageDebugInfo);

			Function*			FunctionAtSourceLocation(LocatableFile* file,
									const SourceLocation& location);

private:
			struct FunctionHashDefinition;
			struct SourceFileEntry;
			struct SourceFileHashDefinition;

			typedef BObjectList<SpecificTeamDebugInfo> SpecificInfoList;
			typedef OpenHashTable<FunctionHashDefinition> FunctionTable;
			typedef OpenHashTable<SourceFileHashDefinition> SourceFileTable;

private:
			status_t			_AddFunction(Function* function);
			void				_RemoveFunction(Function* function);

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			FileManager*		fFileManager;
			SpecificInfoList	fSpecificInfos;
			FunctionTable*		fFunctions;
			SourceFileTable*	fSourceFiles;
};


#endif	// TEAM_DEBUG_INFO_H
