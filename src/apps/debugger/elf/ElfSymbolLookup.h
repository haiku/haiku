/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_SYMBOL_LOOKUP_H
#define ELF_SYMBOL_LOOKUP_H


#include <Referenceable.h>

#include "ElfFile.h"
#include "SymbolInfo.h"


class ElfSymbolLookupSource : public BReferenceable {
public:
	virtual	ssize_t				Read(uint64 address, void* buffer,
									size_t size) = 0;
};


class ElfSymbolLookup {
public:
	static	const uint32		kGetSymbolCountFromHash = ~(uint32)0;

public:
	virtual						~ElfSymbolLookup();

	static	status_t			Create(ElfSymbolLookupSource* source,
									uint64 symbolTable, uint64 symbolHash,
									uint64 stringTable, uint32 symbolCount,
									uint32 symbolTableEntrySize,
									uint64 textDelta, bool is64Bit,
									bool swappedByteOrder, bool cacheSource,
									ElfSymbolLookup*& _lookup);

	virtual	status_t			Init(bool cacheSource) = 0;
	virtual	status_t			NextSymbolInfo(uint32& index,
									SymbolInfo& _info) = 0;
	virtual	status_t			GetSymbolInfo(const char* name,
									uint32 symbolType, SymbolInfo& _info) = 0;
};


#endif	// ELF_SYMBOL_LOOKUP_H
