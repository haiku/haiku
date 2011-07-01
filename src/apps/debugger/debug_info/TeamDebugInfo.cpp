/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TeamDebugInfo.h"

#include <stdio.h>

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "Architecture.h"
#include "DebuggerInterface.h"
#include "DebuggerTeamDebugInfo.h"
#include "DisassembledCode.h"
#include "DwarfTeamDebugInfo.h"
#include "FileManager.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "FunctionID.h"
#include "ImageDebugInfo.h"
#include "LocatableFile.h"
#include "SourceFile.h"
#include "SourceLanguage.h"
#include "SpecificImageDebugInfo.h"
#include "StringUtils.h"
#include "Type.h"
#include "TypeLookupConstraints.h"


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

	Function*& GetLink(Function* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - SourceFileEntry


struct TeamDebugInfo::SourceFileEntry {
	SourceFileEntry(LocatableFile* sourceFile)
		:
		fSourceFile(sourceFile),
		fSourceCode(NULL)
	{
		fSourceFile->AcquireReference();
	}

	~SourceFileEntry()
	{
		SetSourceCode(NULL);
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

	FileSourceCode* GetSourceCode() const
	{
		return fSourceCode;
	}

	void SetSourceCode(FileSourceCode* sourceCode)
	{
		if (sourceCode == fSourceCode)
			return;

		if (fSourceCode != NULL)
			fSourceCode->ReleaseReference();

		fSourceCode = sourceCode;

		if (fSourceCode != NULL)
			fSourceCode->AcquireReference();
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

	Function* FunctionAt(int32 index) const
	{
		return fFunctions.ItemAt(index);
	}

	Function* FunctionByName(const BString& name) const
	{
		// TODO: That's not exactly optimal.
		for (int32 i = 0; Function* function = fFunctions.ItemAt(i); i++) {
			if (name == function->Name())
				return function;
		}
		return NULL;
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
	LocatableFile*		fSourceFile;
	FileSourceCode*		fSourceCode;
	FunctionList		fFunctions;

public:
	SourceFileEntry*	fNext;
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

	SourceFileEntry*& GetLink(SourceFileEntry* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - TeamDebugInfo


TeamDebugInfo::TeamDebugInfo(DebuggerInterface* debuggerInterface,
	Architecture* architecture, FileManager* fileManager)
	:
	fLock("team debug info"),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fFileManager(fileManager),
	fSpecificInfos(10, true),
	fFunctions(NULL),
	fSourceFiles(NULL),
	fTypeCache(NULL)
{
}


TeamDebugInfo::~TeamDebugInfo()
{
	if (fTypeCache != NULL)
		fTypeCache->ReleaseReference();

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
	// check the lock
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	// create function hash table
	fFunctions = new(std::nothrow) FunctionTable;
	if (fFunctions == NULL)
		return B_NO_MEMORY;

	error = fFunctions->Init();
	if (error != B_OK)
		return error;

	// create source file hash table
	fSourceFiles = new(std::nothrow) SourceFileTable;
	if (fSourceFiles == NULL)
		return B_NO_MEMORY;

	error = fSourceFiles->Init();
	if (error != B_OK)
		return error;

	// create a type cache
	fTypeCache = new(std::nothrow) GlobalTypeCache;
	if (fTypeCache == NULL)
		return B_NO_MEMORY;

	error = fTypeCache->Init();
	if (error != B_OK)
		return error;

	// Create specific infos for all types of debug info we support, in
	// descending order of expressiveness.

	// DWARF
	DwarfTeamDebugInfo* dwarfInfo = new(std::nothrow) DwarfTeamDebugInfo(
		fArchitecture, fDebuggerInterface, fFileManager, this, fTypeCache);
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
TeamDebugInfo::LookupTypeByName(const BString& name,
	const TypeLookupConstraints& constraints, Type*& _type)
{
	return GetType(fTypeCache, name, constraints, _type);
}

status_t
TeamDebugInfo::GetType(GlobalTypeCache* cache, const BString& name,
	const TypeLookupConstraints& constraints, Type*& _type)
{
	// maybe the type is already cached
	AutoLocker<GlobalTypeCache> cacheLocker(cache);
	Type* type = cache->GetType(name);
	if (type != NULL) {
		type->AcquireReference();
		_type = type;
		return B_OK;
	}

	cacheLocker.Unlock();

	// Clone the image list and get references to the images, so we can iterate
	// through them without locking.
	AutoLocker<BLocker> locker(fLock);

	ImageList images;
	for (int32 i = 0; ImageDebugInfo* imageDebugInfo = fImages.ItemAt(i); i++) {
		if (images.AddItem(imageDebugInfo))
			imageDebugInfo->AcquireReference();
	}

	locker.Unlock();

	// get the type
	status_t error = B_ENTRY_NOT_FOUND;
	for (int32 i = 0; ImageDebugInfo* imageDebugInfo = images.ItemAt(i); i++) {
		error = imageDebugInfo->GetType(cache, name, constraints, type);
		if (error == B_OK) {
			_type = type;
			break;
		}
	}

	// release the references
	for (int32 i = 0; ImageDebugInfo* imageDebugInfo = images.ItemAt(i); i++)
		imageDebugInfo->ReleaseReference();

	return error;
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


status_t
TeamDebugInfo::LoadSourceCode(LocatableFile* file, FileSourceCode*& _sourceCode)
{
	AutoLocker<BLocker> locker(fLock);

	// If we don't know the source file, there's nothing we can do.
	SourceFileEntry* entry = fSourceFiles->Lookup(file);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	// the source might already be loaded
	FileSourceCode* sourceCode = entry->GetSourceCode();
	if (sourceCode != NULL) {
		sourceCode->AcquireReference();
		_sourceCode = sourceCode;
		return B_OK;
	}

	// get the source language from some function's image debug info
	Function* function = entry->FunctionAt(0);
	if (function == NULL)
		return B_ENTRY_NOT_FOUND;

	FunctionDebugInfo* functionDebugInfo
		= function->FirstInstance()->GetFunctionDebugInfo();
	SourceLanguage* language;
	status_t error = functionDebugInfo->GetSpecificImageDebugInfo()
		->GetSourceLanguage(functionDebugInfo, language);
	if (error != B_OK)
		return error;
	BReference<SourceLanguage> languageReference(language, true);

	// no source code yet
//	locker.Unlock();
	// TODO: It would be nice to unlock here, but we need to iterate through
	// the images below. We could clone the list, acquire references, and
	// unlock. Then we have to compare the list with the then current list when
	// we're done loading.

	// load the source file
	SourceFile* sourceFile;
	error = fFileManager->LoadSourceFile(file, sourceFile);
	if (error != B_OK)
		return error;

	// create the source code
	sourceCode = new(std::nothrow) FileSourceCode(file, sourceFile, language);
	sourceFile->ReleaseReference();
	if (sourceCode == NULL)
		return B_NO_MEMORY;
	BReference<FileSourceCode> sourceCodeReference(sourceCode, true);

	error = sourceCode->Init();
	if (error != B_OK)
		return error;

	// Iterate through all images that know the source file and ask them to add
	// information.
	bool anyInfo = false;
	for (int32 i = 0; ImageDebugInfo* imageDebugInfo = fImages.ItemAt(i); i++)
		anyInfo |= imageDebugInfo->AddSourceCodeInfo(file, sourceCode) == B_OK;

	if (!anyInfo)
		return B_ENTRY_NOT_FOUND;

	entry->SetSourceCode(sourceCode);

	_sourceCode = sourceCodeReference.Detach();
	return B_OK;
}


status_t
TeamDebugInfo::DisassembleFunction(FunctionInstance* functionInstance,
	DisassembledCode*& _sourceCode)
{
	// allocate a buffer for the function code
	static const target_size_t kMaxBufferSize = 64 * 1024;
	target_size_t bufferSize = std::min(functionInstance->Size(),
		kMaxBufferSize);
	void* buffer = malloc(bufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	// read the function code
	FunctionDebugInfo* functionDebugInfo
		= functionInstance->GetFunctionDebugInfo();
	ssize_t bytesRead = functionDebugInfo->GetSpecificImageDebugInfo()
		->ReadCode(functionInstance->Address(), buffer, bufferSize);
	if (bytesRead < 0)
		return bytesRead;

	return fArchitecture->DisassembleCode(functionDebugInfo, buffer, bytesRead,
		_sourceCode);
}


status_t
TeamDebugInfo::AddImageDebugInfo(ImageDebugInfo* imageDebugInfo)
{
	AutoLocker<BLocker> locker(fLock);
		// We have both locks now, so that for read-only access either lock
		// suffices.

	if (!fImages.AddItem(imageDebugInfo))
		return B_NO_MEMORY;

	// Match all of the image debug info's functions instances with functions.
	BObjectList<SourceFileEntry> sourceFileEntries;
	for (int32 i = 0;
		FunctionInstance* instance = imageDebugInfo->FunctionAt(i); i++) {
		// lookup the function or create it, if it doesn't exist yet
		Function* function = fFunctions->Lookup(instance);
		if (function != NULL) {
// TODO: Also update possible user breakpoints in this function!
			function->AddInstance(instance);
			instance->SetFunction(function);

			// The new image debug info might have additional information about
			// the source file of the function, so remember the source file
			// entry.
			if (LocatableFile* sourceFile = function->SourceFile()) {
				SourceFileEntry* entry = fSourceFiles->Lookup(sourceFile);
				if (entry != NULL && entry->GetSourceCode() != NULL)
					sourceFileEntries.AddItem(entry);
			}
		} else {
			function = new(std::nothrow) Function;
			if (function == NULL) {
				RemoveImageDebugInfo(imageDebugInfo);
				return B_NO_MEMORY;
			}
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

	// update the source files the image debug info knows about
	for (int32 i = 0; SourceFileEntry* entry = sourceFileEntries.ItemAt(i);
			i++) {
		FileSourceCode* sourceCode = entry->GetSourceCode();
		sourceCode->Lock();
		if (imageDebugInfo->AddSourceCodeInfo(entry->SourceFile(),
				sourceCode) == B_OK) {
			// TODO: Notify interesting parties! Iterate through all functions
			// for this source file?
		}
		sourceCode->Unlock();
	}

	return B_OK;
}


void
TeamDebugInfo::RemoveImageDebugInfo(ImageDebugInfo* imageDebugInfo)
{
	AutoLocker<BLocker> locker(fLock);
		// We have both locks now, so that for read-only access either lock
		// suffices.

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

	// remove cached types from that image
	fTypeCache->RemoveTypes(imageDebugInfo->GetImageInfo().ImageID());

	fImages.RemoveItem(imageDebugInfo);
}


ImageDebugInfo*
TeamDebugInfo::ImageDebugInfoByName(const char* name) const
{
	for (int32 i = 0; ImageDebugInfo* imageDebugInfo = fImages.ItemAt(i); i++) {
		if (imageDebugInfo->GetImageInfo().Name() == name)
			return imageDebugInfo;
	}

	return NULL;
}


Function*
TeamDebugInfo::FunctionAtSourceLocation(LocatableFile* file,
	const SourceLocation& location) const
{
	if (SourceFileEntry* entry = fSourceFiles->Lookup(file))
		return entry->FunctionAtLocation(location);
	return NULL;
}


Function*
TeamDebugInfo::FunctionByID(FunctionID* functionID) const
{
	if (SourceFunctionID* sourceFunctionID
			= dynamic_cast<SourceFunctionID*>(functionID)) {
		// get the source file
		LocatableFile* file = fFileManager->GetSourceFile(
			sourceFunctionID->SourceFilePath());
		if (file == NULL)
			return NULL;
		BReference<LocatableFile> fileReference(file, true);

		if (SourceFileEntry* entry = fSourceFiles->Lookup(file))
			return entry->FunctionByName(functionID->FunctionName());
		return NULL;
	}

	ImageFunctionID* imageFunctionID
		= dynamic_cast<ImageFunctionID*>(functionID);
	if (imageFunctionID == NULL)
		return NULL;

	ImageDebugInfo* imageDebugInfo
		= ImageDebugInfoByName(imageFunctionID->ImageName());
	if (imageDebugInfo == NULL)
		return NULL;

	FunctionInstance* functionInstance = imageDebugInfo->FunctionByName(
		functionID->FunctionName());
	return functionInstance != NULL ? functionInstance->GetFunction() : NULL;
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
