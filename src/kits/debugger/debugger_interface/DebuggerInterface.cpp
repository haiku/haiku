/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DebuggerInterface.h"

#include <algorithm>

#include <AutoDeleter.h>

#include "ElfSymbolLookup.h"


// #pragma mark - SymbolTableLookupSource


struct DebuggerInterface::SymbolTableLookupSource : ElfSymbolLookupSource {
public:
	SymbolTableLookupSource(const void* symbolTable, size_t symbolTableSize,
		const char* stringTable, size_t stringTableSize)
		:
		fSymbolTable((const uint8*)symbolTable),
		fStringTable(stringTable),
		fSymbolTableSize(symbolTableSize),
		fStringTableEnd(symbolTableSize + stringTableSize)
	{
	}

	virtual ssize_t Read(uint64 address, void* buffer, size_t size)
	{
		ssize_t copied = 0;

		if (address > fStringTableEnd)
			return B_BAD_VALUE;

		if (address < fSymbolTableSize) {
			size_t toCopy = std::min(size, size_t(fSymbolTableSize - address));
			memcpy(buffer, fSymbolTable + address, toCopy);
			address -= toCopy;
			size -= toCopy;
			copied += toCopy;
		}

		if (address < fStringTableEnd) {
			size_t toCopy = std::min(size, size_t(fStringTableEnd - address));
			memcpy(buffer, fStringTable + address - fSymbolTableSize, toCopy);
			address -= toCopy;
			size -= toCopy;
			copied += toCopy;
		}

		return copied;
	}

private:
	const uint8*	fSymbolTable;
	const char*		fStringTable;
	size_t			fSymbolTableSize;
	size_t			fStringTableEnd;
};


// #pragma mark - DebuggerInterface


DebuggerInterface::~DebuggerInterface()
{
}


bool
DebuggerInterface::IsPostMortem() const
{
	// only true for core file interfaces
	return false;
}


status_t
DebuggerInterface::GetElfSymbols(const char* filePath, int64 textDelta,
	BObjectList<SymbolInfo>& infos)
{
	// open the ELF file
	ElfFile elfFile;
	status_t error = elfFile.Init(filePath);
	if (error != B_OK)
		return error;

	// create the symbol lookup
	ElfSymbolLookup* symbolLookup;
	error = elfFile.CreateSymbolLookup(textDelta, symbolLookup);
	if (error != B_OK)
		return error;

	ObjectDeleter<ElfSymbolLookup> symbolLookupDeleter(symbolLookup);

	// get the symbols
	return GetElfSymbols(symbolLookup, infos);
}


status_t
DebuggerInterface::GetElfSymbols(const void* symbolTable, uint32 symbolCount,
	uint32 symbolTableEntrySize, const char* stringTable,
	uint32 stringTableSize, bool is64Bit, bool swappedByteOrder,
	int64 textDelta, BObjectList<SymbolInfo>& infos)
{
	size_t symbolTableSize = symbolCount * symbolTableEntrySize;
	SymbolTableLookupSource* source = new(std::nothrow) SymbolTableLookupSource(
		symbolTable, symbolTableSize, stringTable, stringTableSize);
	if (source == NULL)
		return B_NO_MEMORY;
	BReference<SymbolTableLookupSource> sourceReference(source, true);

	ElfSymbolLookup* symbolLookup;
	status_t error = ElfSymbolLookup::Create(
		source, 0, 0, symbolTableSize, symbolCount, symbolTableEntrySize,
		textDelta, is64Bit, swappedByteOrder, false, symbolLookup);
	if (error != B_OK)
		return error;

	ObjectDeleter<ElfSymbolLookup> symbolLookupDeleter(symbolLookup);

	// get the symbols
	return GetElfSymbols(symbolLookup, infos);
}


status_t
DebuggerInterface::GetElfSymbols(ElfSymbolLookup* symbolLookup,
	BObjectList<SymbolInfo>& infos)
{
	SymbolInfo symbolInfo;
	uint32 index = 0;
	while (symbolLookup->NextSymbolInfo(index, symbolInfo) == B_OK) {
		SymbolInfo* info = new(std::nothrow) SymbolInfo(symbolInfo);
		if (info == NULL || !infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}
