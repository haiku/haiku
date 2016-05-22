/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ElfSymbolLookup.h"

#include <algorithm>

#include <image.h>


static const size_t kMaxSymbolNameLength = 64 * 1024;
static const size_t kMaxReadStringChunkSize = 1024;
static const size_t kCacheBufferSize = 4 * 1024;


struct CachedSymbolLookupSource : public ElfSymbolLookupSource {
	CachedSymbolLookupSource(ElfSymbolLookupSource* source)
		:
		fSource(source),
		fBufferSize(0)
	{
		for (int i = 0; i < 2; i++) {
			fBuffer[i] = 0;
			fAddress[i] = 0;
			fCachedSize[i] = 0;
			fHitEnd[i] = true;
		}

		fSource->AcquireReference();
	}

	~CachedSymbolLookupSource()
	{
		delete[] fBuffer[0];

		fSource->ReleaseReference();
	}

	status_t Init(size_t bufferSize)
	{
		fBuffer[0] = new(std::nothrow) uint8[bufferSize * 2];
		if (fBuffer[0] == NULL)
			return B_NO_MEMORY;

		fBuffer[1] = fBuffer[0] + bufferSize;
		fBufferSize = bufferSize;
		return B_OK;
	}

	virtual ssize_t Read(uint64 address, void* _buffer, size_t size)
	{
		uint8* buffer = (uint8*)_buffer;
		size_t totalRead = 0;

		while (size > 0) {
			ssize_t bytesRead = _ReadPartial(address, buffer, size);
			if (bytesRead < 0)
				return totalRead == 0 ? bytesRead : totalRead;
			if (bytesRead == 0)
				return totalRead == 0 ? B_IO_ERROR : totalRead;

			totalRead += bytesRead;
			buffer += bytesRead;
			size -= bytesRead;
		}

		return totalRead;
	}

private:
	ssize_t _ReadPartial(uint64 address, uint8* buffer, size_t size)
	{
		size_t bytesRead = _ReadCached(address, buffer, size);
		if (bytesRead > 0)
			return bytesRead;

		status_t error = _Cache(address, size);
		if (error != B_OK)
			return error;

		return (ssize_t)_ReadCached(address, buffer, size);
	}

	size_t _ReadCached(uint64 address, uint8* buffer, size_t size)
	{
		for (int i = 0; i < 2; i++) {
			if (address >= fAddress[i]
					&& address < fAddress[i] + fCachedSize[i]) {
				size_t toRead = std::min(size,
					size_t(fAddress[i] + fCachedSize[i] - address));
				memcpy(buffer, fBuffer[i] + (address - fAddress[i]), toRead);
				fHitEnd[i] = address + toRead == fAddress[i] + fCachedSize[i];
				return toRead;
			}
		}
		return 0;
	}

	status_t _Cache(uint64 address, size_t size)
	{
		int i = 0;
		if (!fHitEnd[i])
			i++;

		ssize_t bytesRead = fSource->Read(address, fBuffer[i], fBufferSize);
		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead == 0)
			return B_IO_ERROR;

		fAddress[i] = address;
		fCachedSize[i] = bytesRead;
		fHitEnd[i] = false;
		return B_OK;
	}

private:
	ElfSymbolLookupSource*	fSource;
	uint8*					fBuffer[2];
	uint64					fAddress[2];
	size_t					fCachedSize[2];
	bool					fHitEnd[2];
	size_t					fBufferSize;
};


// #pragma mark - ElfSymbolLookupImpl


template<typename ElfClass>
class ElfSymbolLookupImpl : public ElfSymbolLookup {
public:
	typedef typename ElfClass::Sym ElfSym;

	ElfSymbolLookupImpl(ElfSymbolLookupSource* source, uint64 symbolTable,
		uint64 symbolHash, uint64 stringTable, uint32 symbolCount,
		uint32 symbolTableEntrySize, uint64 textDelta, bool swappedByteOrder)
		:
		fSource(NULL),
		fSymbolTable(symbolTable),
		fSymbolHash(symbolHash),
		fStringTable(stringTable),
		fSymbolCount(symbolCount),
		fSymbolTableEntrySize(symbolTableEntrySize),
		fTextDelta(textDelta),
		fSwappedByteOrder(swappedByteOrder)
	{
		SetSource(source);
	}

	~ElfSymbolLookupImpl()
	{
		SetSource(NULL);
	}

	template<typename Value>
	Value Get(const Value& value) const
	{
		return ElfFile::StaticGet(value, fSwappedByteOrder);
	}

	void SetSource(ElfSymbolLookupSource* source)
	{
		if (source == fSource)
			return;

		if (fSource != NULL)
			fSource->ReleaseReference();

		fSource = source;

		if (fSource != NULL)
			fSource->AcquireReference();
	}

	virtual status_t Init(bool cacheSource)
	{
		if (fSymbolTableEntrySize < sizeof(ElfSym))
			return B_BAD_DATA;

		// Create a cached source, if requested.
		if (cacheSource) {
			CachedSymbolLookupSource* cachedSource
				= new(std::nothrow) CachedSymbolLookupSource(fSource);
			if (cachedSource == NULL)
				return B_NO_MEMORY;

			SetSource(cachedSource);

			status_t error = cachedSource->Init(kCacheBufferSize);
			if (error != B_OK)
				return error;
		}

		if (fSymbolCount == kGetSymbolCountFromHash) {
			// Read the number of symbols in the symbol table from the hash
			// table entry 1.
			uint32 symbolCount;
			ssize_t bytesRead = fSource->Read(fSymbolHash + 4, &symbolCount, 4);
			if (bytesRead < 0)
				return bytesRead;
			if (bytesRead != 4)
				return B_IO_ERROR;

			fSymbolCount = Get(symbolCount);
		}

		return B_OK;
	}

	virtual status_t NextSymbolInfo(uint32& index, SymbolInfo& _info)
	{
		uint64 symbolAddress = fSymbolTable + index * fSymbolTableEntrySize;
		for (; index < fSymbolCount;
				index++, symbolAddress += fSymbolTableEntrySize) {
			// read the symbol structure
			ElfSym symbol;
			ssize_t bytesRead = fSource->Read(symbolAddress, &symbol,
				sizeof(symbol));
			if (bytesRead < 0)
				return bytesRead;
			if ((size_t)bytesRead != sizeof(symbol))
				return B_IO_ERROR;

			// check, if it is a function or a data object and defined
			// Note: Type() operates on a uint8, so byte order is irrelevant.
			if ((symbol.Type() != STT_FUNC && symbol.Type() != STT_OBJECT)
				|| symbol.st_value == 0) {
				continue;
			}

			// get the values
			target_addr_t address = Get(symbol.st_value) + fTextDelta;
			target_size_t size = Get(symbol.st_size);
			uint32 type = symbol.Type() == STT_FUNC
				? B_SYMBOL_TYPE_TEXT : B_SYMBOL_TYPE_DATA;

			// get the symbol name
			uint64 nameAddress = fStringTable + Get(symbol.st_name);
			BString name;
			status_t error = _ReadString(nameAddress, kMaxSymbolNameLength,
				name);
			if (error != B_OK)
				return error;

			_info.SetTo(address, size, type, name);
			index++;
			return B_OK;
		}

		return B_ENTRY_NOT_FOUND;
	}

	virtual status_t GetSymbolInfo(const char* name, uint32 symbolType,
		SymbolInfo& _info)
	{
		// TODO: Optimize this by using the hash table.
		uint32 index = 0;
		SymbolInfo info;
		while (NextSymbolInfo(index, info) == B_OK) {
			if (strcmp(name, info.Name()) == 0) {
				_info = info;
				return B_OK;
			}
		}

		return B_ENTRY_NOT_FOUND;
	}

private:
	status_t _ReadString(uint64 address, size_t size, BString& _string)
	{
		_string.Truncate(0);

		char buffer[kMaxReadStringChunkSize];
		while (size > 0) {
			size_t toRead = std::min(size, sizeof(buffer));
			ssize_t bytesRead = fSource->Read(address, buffer, toRead);
			if (bytesRead < 0)
				return bytesRead;
			if (bytesRead == 0)
				return B_IO_ERROR;

			size_t chunkSize = strnlen(buffer, bytesRead);
			int32 oldLength = _string.Length();
			_string.Append(buffer, chunkSize);
			if (_string.Length() <= oldLength)
				return B_NO_MEMORY;

			if (chunkSize < (size_t)bytesRead) {
				// we found a terminating null
				return B_OK;
			}

			address += bytesRead;
			size -= bytesRead;
		}

		return B_BAD_DATA;
	}

private:
	ElfSymbolLookupSource*	fSource;
	uint64					fSymbolTable;
	uint64					fSymbolHash;
	uint64					fStringTable;
	uint32					fSymbolCount;
	uint32					fSymbolTableEntrySize;
	uint64					fTextDelta;
	bool					fSwappedByteOrder;
};


// #pragma mark - ElfSymbolLookup


ElfSymbolLookup::~ElfSymbolLookup()
{
}


/*static*/ status_t
ElfSymbolLookup::Create(ElfSymbolLookupSource* source, uint64 symbolTable,
	uint64 symbolHash, uint64 stringTable, uint32 symbolCount,
	uint32 symbolTableEntrySize, uint64 textDelta, bool is64Bit,
	bool swappedByteOrder, bool cacheSource, ElfSymbolLookup*& _lookup)
{
	// create
	ElfSymbolLookup* lookup;
	if (is64Bit) {
		lookup = new(std::nothrow) ElfSymbolLookupImpl<ElfClass64>(source,
			symbolTable, symbolHash, stringTable, symbolCount,
			symbolTableEntrySize, textDelta, swappedByteOrder);
	} else {
		lookup = new(std::nothrow) ElfSymbolLookupImpl<ElfClass32>(source,
			symbolTable, symbolHash, stringTable, symbolCount,
			symbolTableEntrySize, textDelta, swappedByteOrder);
	}

	if (lookup == NULL)
		return B_NO_MEMORY;

	// init
	status_t error = lookup->Init(cacheSource);
	if (error == B_OK)
		_lookup = lookup;
	else
		delete lookup;

	return error;
}
