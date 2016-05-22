/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CoreFileDebuggerInterface.h"

#include <algorithm>
#include <new>

#include <errno.h>

#include <AutoDeleter.h>

#include "ArchitectureX86.h"
#include "ArchitectureX8664.h"
#include "CoreFile.h"
#include "ElfSymbolLookup.h"
#include "ImageInfo.h"
#include "TeamInfo.h"
#include "ThreadInfo.h"
#include "Tracing.h"


CoreFileDebuggerInterface::CoreFileDebuggerInterface(CoreFile* coreFile)
	:
	fCoreFile(coreFile),
	fArchitecture(NULL)
{
}


CoreFileDebuggerInterface::~CoreFileDebuggerInterface()
{
	if (fArchitecture != NULL)
		fArchitecture->ReleaseReference();

	delete fCoreFile;
}


status_t
CoreFileDebuggerInterface::Init()
{
	// create the Architecture object
	uint16 machine = fCoreFile->GetElfFile().Machine();
	switch (machine) {
		case EM_386:
			fArchitecture = new(std::nothrow) ArchitectureX86(this);
			break;
		case EM_X86_64:
			fArchitecture = new(std::nothrow) ArchitectureX8664(this);
			break;
		default:
			WARNING("Unsupported core file machine (%u)\n", machine);
			return B_UNSUPPORTED;
	}

	if (fArchitecture == NULL)
		return B_NO_MEMORY;

	return fArchitecture->Init();
}


void
CoreFileDebuggerInterface::Close(bool killTeam)
{
}


bool
CoreFileDebuggerInterface::Connected() const
{
	return true;
}


bool
CoreFileDebuggerInterface::IsPostMortem() const
{
	return true;
}


team_id
CoreFileDebuggerInterface::TeamID() const
{
	return fCoreFile->GetTeamInfo().Id();
}


Architecture*
CoreFileDebuggerInterface::GetArchitecture() const
{
	return fArchitecture;
}


status_t
CoreFileDebuggerInterface::GetNextDebugEvent(DebugEvent*& _event)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::SetTeamDebuggingFlags(uint32 flags)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::ContinueThread(thread_id thread)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::StopThread(thread_id thread)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::SingleStepThread(thread_id thread)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::InstallBreakpoint(target_addr_t address)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::UninstallBreakpoint(target_addr_t address)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::InstallWatchpoint(target_addr_t address, uint32 type,
	int32 length)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::UninstallWatchpoint(target_addr_t address)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetSystemInfo(SystemInfo& info)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetTeamInfo(TeamInfo& info)
{
	const CoreFileTeamInfo& coreInfo = fCoreFile->GetTeamInfo();
	info.SetTo(coreInfo.Id(), coreInfo.Arguments());
	return B_OK;
}


status_t
CoreFileDebuggerInterface::GetThreadInfos(BObjectList<ThreadInfo>& infos)
{
	int32 count = fCoreFile->CountThreadInfos();
	for (int32 i = 0; i < count; i++) {
		const CoreFileThreadInfo* coreInfo = fCoreFile->ThreadInfoAt(i);
		ThreadInfo* info = new(std::nothrow) ThreadInfo;
		if (info == NULL || !infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}

		_GetThreadInfo(*coreInfo, *info);
	}

	return B_OK;
}


status_t
CoreFileDebuggerInterface::GetImageInfos(BObjectList<ImageInfo>& infos)
{
	int32 count = fCoreFile->CountImageInfos();
	for (int32 i = 0; i < count; i++) {
		const CoreFileImageInfo* coreInfo = fCoreFile->ImageInfoAt(i);
		ImageInfo* info = new(std::nothrow) ImageInfo;
		if (info == NULL || !infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}

		info->SetTo(TeamID(), coreInfo->Id(), coreInfo->Name(),
			(image_type)coreInfo->Type(), coreInfo->TextBase(),
			coreInfo->TextSize(), coreInfo->DataBase(), coreInfo->DataSize());
	}

	return B_OK;
}


status_t
CoreFileDebuggerInterface::GetAreaInfos(BObjectList<AreaInfo>& infos)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetSemaphoreInfos(BObjectList<SemaphoreInfo>& infos)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetSymbolInfos(team_id team, image_id image,
	BObjectList<SymbolInfo>& infos)
{
	// get the image info
	const CoreFileImageInfo* imageInfo = fCoreFile->ImageInfoForId(image);
	if (imageInfo == NULL)
		return B_BAD_IMAGE_ID;

	if (const CoreFileSymbolsInfo* symbolsInfo = imageInfo->SymbolsInfo()) {
		return GetElfSymbols(symbolsInfo->SymbolTable(),
			symbolsInfo->SymbolCount(), symbolsInfo->SymbolTableEntrySize(),
			symbolsInfo->StringTable(), symbolsInfo->StringTableSize(),
			fCoreFile->GetElfFile().Is64Bit(),
			fCoreFile->GetElfFile().IsByteOrderSwapped(),
			imageInfo->TextDelta(), infos);
	}

	// get the symbols from the ELF file, if possible
	status_t error = GetElfSymbols(imageInfo->Name(), imageInfo->TextDelta(),
		infos);
	if (error == B_OK)
		return error;

	// get the symbols from the core file, if possible
	ElfSymbolLookup* symbolLookup;
	error = fCoreFile->CreateSymbolLookup(imageInfo, symbolLookup);
	if (error != B_OK) {
		WARNING("Failed to create symbol lookup for image (%" B_PRId32
			"): %s\n", image, strerror(error));
		return error;
	}

	ObjectDeleter<ElfSymbolLookup> symbolLookupDeleter(symbolLookup);

	return GetElfSymbols(symbolLookup, infos);
}


status_t
CoreFileDebuggerInterface::GetSymbolInfo(team_id team, image_id image,
	const char* name, int32 symbolType, SymbolInfo& info)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetThreadInfo(thread_id thread, ThreadInfo& info)
{
	const CoreFileThreadInfo* coreInfo = fCoreFile->ThreadInfoForId(thread);
	if (coreInfo == NULL)
		return B_BAD_THREAD_ID;

	_GetThreadInfo(*coreInfo, info);
	return B_OK;
}


status_t
CoreFileDebuggerInterface::GetCpuState(thread_id thread, CpuState*& _state)
{
	const CoreFileThreadInfo* coreInfo = fCoreFile->ThreadInfoForId(thread);
	if (coreInfo == NULL)
		return B_BAD_THREAD_ID;

	return fArchitecture->CreateCpuState(coreInfo->GetCpuState(),
		coreInfo->CpuStateSize(), _state);
}


status_t
CoreFileDebuggerInterface::SetCpuState(thread_id thread, const CpuState* state)
{
	return B_UNSUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetCpuFeatures(uint32& flags)
{
	return fArchitecture->GetCpuFeatures(flags);
}


status_t
CoreFileDebuggerInterface::WriteCoreFile(const char* path)
{
	return B_NOT_SUPPORTED;
}


status_t
CoreFileDebuggerInterface::GetMemoryProperties(target_addr_t address,
	uint32& protection, uint32& locking)
{
	const CoreFileAreaInfo* info = fCoreFile->AreaInfoForAddress(address);
	if (info == NULL)
		return B_BAD_ADDRESS;

	protection = info->Protection() & ~(uint32)B_WRITE_AREA;
		// Filter out write protection, since we don't support writing memory.
	locking = info->Locking();
	return B_OK;
}


ssize_t
CoreFileDebuggerInterface::ReadMemory(target_addr_t address, void* _buffer,
	size_t size)
{
	if (size == 0)
		return B_OK;

	ssize_t totalRead = 0;
	uint8* buffer = (uint8*)_buffer;

	while (size > 0) {
		const CoreFileAreaInfo* info = fCoreFile->AreaInfoForAddress(address);
		if (info == NULL)
			return totalRead > 0 ? totalRead : B_BAD_ADDRESS;

		ElfSegment* segment = info->Segment();
		uint64 offset = address - segment->LoadAddress();
		if (offset >= segment->FileSize())
			return totalRead > 0 ? totalRead : B_BAD_ADDRESS;

		size_t toRead = (size_t)std::min((uint64)size,
			segment->FileSize() - offset);
		ssize_t bytesRead = pread(fCoreFile->GetElfFile().FD(), buffer, toRead,
			segment->FileOffset() + offset);
		if (bytesRead <= 0) {
			status_t error = bytesRead == 0 ? B_IO_ERROR : errno;
			return totalRead > 0 ? totalRead : error;
		}

		buffer += bytesRead;
		size -= bytesRead;
		totalRead += bytesRead;
	}

	return totalRead;
}


ssize_t
CoreFileDebuggerInterface::WriteMemory(target_addr_t address, void* buffer,
	size_t size)
{
	return B_UNSUPPORTED;
}


void
CoreFileDebuggerInterface::_GetThreadInfo(const CoreFileThreadInfo& coreInfo,
	ThreadInfo& info)
{
	info.SetTo(TeamID(), coreInfo.Id(), coreInfo.Name());
}
