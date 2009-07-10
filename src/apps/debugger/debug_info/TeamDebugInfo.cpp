/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TeamDebugInfo.h"

#include <stdio.h>

#include <new>

#include <AutoDeleter.h>

#include "DebuggerTeamDebugInfo.h"
#include "DwarfTeamDebugInfo.h"
#include "Function.h"
#include "ImageDebugInfo.h"
#include "LocatableFile.h"
#include "SpecificImageDebugInfo.h"
#include "StringUtils.h"


// #pragma mark - FunctionHashDefinition


struct TeamDebugInfo::FunctionHashDefinition {
	typedef const FunctionInstance*	KeyType;
	typedef	Function				ValueType;

	size_t HashKey(const FunctionInstance* key) const
	{
		// Instances without source file only equal themselves.
		if (key->SourceFile() == NULL)
			return (uint32)(addr_t)key;

		uint32 hash = StringUtils::HashValue(key->Name());
		hash = hash * 17 + (uint32)(addr_t)key->SourceFile();
		SourceLocation location = key->GetSourceLocation();
		hash = hash * 17 + location.Line();
		hash = hash * 17 + location.Column();

		return hash;
	}

	size_t Hash(const Function* value) const
	{
		return HashKey(value->FirstInstance());
	}

	bool Compare(const FunctionInstance* key, const Function* value) const
	{
		// source file must be the same
		if (key->SourceFile() != value->SourceFile())
			return false;

		// Instances without source file only equal themselves.
		if (key->SourceFile() == NULL)
			return key == value->FirstInstance();

		// Source location and function name must also match.
		return key->GetSourceLocation() == value->GetSourceLocation()
			&& key->Name() == value->Name();
	}

	HashTableLink<Function>* GetLink(Function* value) const
	{
		return value;
	}
};


// #pragma mark - SourceFileEntry


struct TeamDebugInfo::SourceFileEntry : public HashTableLink<SourceFileEntry> {
	SourceFileEntry(LocatableFile* sourceFile)
		:
		fSourceFile(sourceFile)
	{
		fSourceFile->AcquireReference();
	}

	~SourceFileEntry()
	{
		fSourceFile->ReleaseReference();
	}

	status_t Init()
	{
		return B_OK;
	}

	LocatableFile* SourceFile() const
	{
		return fSourceFile;
	}

	bool IsUnused() const
	{
		return fFunctions.IsEmpty();
	}

	status_t AddFunction(Function* function)
	{
		if (!fFunctions.BinaryInsert(function, &_CompareFunctions))
			return B_NO_MEMORY;

		return B_OK;
	}

	void RemoveFunction(Function* function)
	{
		int32 index = fFunctions.BinarySearchIndex(*function,
			&_CompareFunctions);
		if (index >= 0)
			fFunctions.RemoveItemAt(index);
	}

	Function* FunctionAtLocation(const SourceLocation& location) const
	{
		int32 index = fFunctions.BinarySearchIndexByKey(location,
			&_CompareLocationFunction);
		if (index >= 0)
			return fFunctions.ItemAt(index);

		// No exact match, so we return the previous function which might still
		// contain the location.
		index = -index - 1;

		if (index == 0)
			return NULL;

		return fFunctions.ItemAt(index - 1);
	}

private:
	typedef BObjectList<Function> FunctionList;

private:
	static int _CompareFunctions(const Function* a, const Function* b)
	{
		SourceLocation locationA = a->GetSourceLocation();
		SourceLocation locationB = b->GetSourceLocation();

		if (locationA < locationB)
			return -1;

		return locationA == locationB ? 0 : 1;
	}

	static int _CompareLocationFunction(const SourceLocation* location,
		const Function* function)
	{
		SourceLocation functionLocation = function->GetSourceLocation();

		if (*location < functionLocation)
			return -1;

		return *location == functionLocation ? 0 : 1;
	}

private:
	LocatableFile*	fSourceFile;
	FunctionList	fFunctions;
};


// #pragma mark - SourceFileHashDefinition


struct TeamDebugInfo::SourceFileHashDefinition {
	typedef const LocatableFile*	KeyType;
	typedef	SourceFileEntry			ValueType;

	size_t HashKey(const LocatableFile* key) const
	{
		return (size_t)(addr_t)key;
	}

	size_t Hash(const SourceFileEntry* value) const
	{
		return HashKey(value->SourceFile());
	}

	bool Compare(const LocatableFile* key, const SourceFileEntry* value) const
	{
		return key == value->SourceFile();
	}

	HashTableLink<SourceFileEntry>* GetLink(SourceFileEntry* value) const
	{
		return value;
	}
};


// #pragma mark - TeamDebugInfo


TeamDebugInfo::TeamDebugInfo(DebuggerInterface* debuggerInterface,
	Architecture* architecture, FileManager* fileManager)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fFileManager(fileManager),
	fSpecificInfos(10, true),
	fFunctions(NULL),
	fSourceFiles(NULL)
{
}


TeamDebugInfo::~TeamDebugInfo()
{
	if (fSourceFiles != NULL) {
		SourceFileEntry* entry = fSourceFiles->Clear(true);
		while (entry != NULL) {
			SourceFileEntry* next = entry->fNext;
			delete entry;
			entry = next;
		}

		delete fSourceFiles;
	}

	if (fFunctions != NULL) {
		Function* function = fFunctions->Clear(true);
		while (function != NULL) {
			Function* next = function->fNext;
			function->ReleaseReference();
			function = next;
		}

		delete fFunctions;
	}
}


status_t
TeamDebugInfo::Init()
{
	// create function hash table
	fFunctions = new(std::nothrow) FunctionTable;
	if (fFunctions == NULL)
		return B_NO_MEMORY;

	status_t error = fFunctions->Init();
	if (error != B_OK)
		return error;

	// create source file hash table
	fSourceFiles = new(std::nothrow) SourceFileTable;
	if (fSourceFiles == NULL)
		return B_NO_MEMORY;

	error = fSourceFiles->Init();
	if (error != B_OK)
		return error;

	// Create specific infos for all types of debug info we support, in
	// descending order of expressiveness.

	// DWARF
	DwarfTeamDebugInfo* dwarfInfo = new(std::nothrow) DwarfTeamDebugInfo(
		fArchitecture, fFileManager);
	if (dwarfInfo == NULL || !fSpecificInfos.AddItem(dwarfInfo)) {
		delete dwarfInfo;
		return B_NO_MEMORY;
	}

	error = dwarfInfo->Init();
	if (error != B_OK)
		return error;

	// debugger based info
	DebuggerTeamDebugInfo* debuggerInfo
		= new(std::nothrow) DebuggerTeamDebugInfo(fDebuggerInterface,
			fArchitecture);
	if (debuggerInfo == NULL || !fSpecificInfos.AddItem(debuggerInfo)) {
		delete debuggerInfo;
		return B_NO_MEMORY;
	}

	error = debuggerInfo->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
TeamDebugInfo::LoadImageDebugInfo(const ImageInfo& imageInfo,
	LocatableFile* imageFile, ImageDebugInfo*& _imageDebugInfo)
{
	ImageDebugInfo* imageDebugInfo = new(std::nothrow) ImageDebugInfo(
		imageInfo);
	if (imageDebugInfo == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<ImageDebugInfo> imageDebugInfoDeleter(imageDebugInfo);

	for (int32 i = 0; SpecificTeamDebugInfo* specificTeamInfo
			= fSpecificInfos.ItemAt(i); i++) {
		SpecificImageDebugInfo* specificImageInfo;
		status_t error = specificTeamInfo->CreateImageDebugInfo(imageInfo,
			imageFile, specificImageInfo);
		if (error == B_OK) {
			if (!imageDebugInfo->AddSpecificInfo(specificImageInfo)) {
				delete specificImageInfo;
				return B_NO_MEMORY;
			}
		} else if (error == B_NO_MEMORY)
			return error;
				// fail only when out of memory
	}

	status_t error = imageDebugInfo->FinishInit();
	if (error != B_OK)
		return error;

	_imageDebugInfo = imageDebugInfoDeleter.Detach();
	return B_OK;
}


#include <stdio.h>
status_t
TeamDebugInfo::AddImageDebugInfo(ImageDebugInfo* imageDebugInfo)
{
printf("TeamDebugInfo::AddImageDebugInfo(%p)\n", imageDebugInfo);
	// Match all of the image debug info's functions instances with functions.
	for (int32 i = 0;
		FunctionInstance* instance = imageDebugInfo->FunctionAt(i); i++) {
		// lookup the function or create it, if it doesn't exist yet
		Function* function = fFunctions->Lookup(instance);
		if (function != NULL) {
// TODO: Also update possible user breakpoints in this function!
printf("  adding instance %p to existing function %p\n", instance, function);
			function->AddInstance(instance);
			instance->SetFunction(function);
		} else {
			function = new(std::nothrow) Function;
			if (function == NULL) {
				RemoveImageDebugInfo(imageDebugInfo);
				return B_NO_MEMORY;
			}
printf("  adding instance %p to new function %p\n", instance, function);
			function->AddInstance(instance);
			instance->SetFunction(function);

			status_t error = _AddFunction(function);
				// Insert after adding the instance. Otherwise the function
				// wouldn't be hashable/comparable.
			if (error != B_OK) {
				function->RemoveInstance(instance);
				instance->SetFunction(NULL);
				RemoveImageDebugInfo(imageDebugInfo);
				return error;
			}
		}
	}

	return B_OK;
}


void
TeamDebugInfo::RemoveImageDebugInfo(ImageDebugInfo* imageDebugInfo)
{
	// Remove the functions from all of the image debug info's functions
	// instances.
	for (int32 i = 0;
		FunctionInstance* instance = imageDebugInfo->FunctionAt(i); i++) {
		if (Function* function = instance->GetFunction()) {
// TODO: Also update possible user breakpoints in this function!
			if (function->FirstInstance() == function->LastInstance()) {
				// function unused -- remove it
				// Note, that we have to remove it from the hash before removing
				// the instance, since otherwise the function cannot be compared
				// anymore.
				_RemoveFunction(function);
				function->ReleaseReference();
					// The instance still has a reference.
			}

			function->RemoveInstance(instance);
			instance->SetFunction(NULL);
				// If this was the last instance, it will remove the last
				// reference to the function.
		}
	}
}


Function*
TeamDebugInfo::FunctionAtSourceLocation(LocatableFile* file,
	const SourceLocation& location)
{
	if (SourceFileEntry* entry = fSourceFiles->Lookup(file))
		return entry->FunctionAtLocation(location);
	return NULL;
}


status_t
TeamDebugInfo::_AddFunction(Function* function)
{
	// If the function refers to a source file, add it to the respective entry.
	if (LocatableFile* sourceFile = function->SourceFile()) {
		SourceFileEntry* entry = fSourceFiles->Lookup(sourceFile);
		if (entry == NULL) {
			// no entry for the source file yet -- create on
			entry = new(std::nothrow) SourceFileEntry(sourceFile);
			if (entry == NULL)
				return B_NO_MEMORY;

			status_t error = entry->Init();
			if (error != B_OK) {
				delete entry;
				return error;
			}

			fSourceFiles->Insert(entry);
		}

		// add the function
		status_t error = entry->AddFunction(function);
		if (error != B_OK) {
			if (entry->IsUnused()) {
				fSourceFiles->Remove(entry);
				delete entry;
			}
			return error;
		}
	}

	fFunctions->Insert(function);

	return B_OK;
}


void
TeamDebugInfo::_RemoveFunction(Function* function)
{
	fFunctions->Remove(function);

	// If the function refers to a source file, remove it from the respective
	// entry.
	if (LocatableFile* sourceFile = function->SourceFile()) {
		if (SourceFileEntry* entry = fSourceFiles->Lookup(sourceFile))
			entry->RemoveFunction(function);
	}
}
