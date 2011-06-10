/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamMemoryBlockManager.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "TeamMemoryBlock.h"


struct TeamMemoryBlockManager::Key {
	target_addr_t address;

	Key(target_addr_t address)
		:
		address(address)
	{
	}

	uint32 HashValue() const
	{
		return (uint32)address;
	}

	bool operator==(const Key& other) const
	{
		return address == other.address;
	}
};


struct TeamMemoryBlockManager::MemoryBlockEntry : Key {
	TeamMemoryBlock*	block;
	MemoryBlockEntry*	next;

	MemoryBlockEntry(TeamMemoryBlock* block)
		:
		Key(block->BaseAddress()),
		block(block)
	{
		block->AcquireReference();
	}

	~MemoryBlockEntry()
	{
		block->ReleaseReference();
	}
};


struct TeamMemoryBlockManager::MemoryBlockHashDefinition {
	typedef Key					KeyType;
	typedef	MemoryBlockEntry	ValueType;

	size_t HashKey(const Key& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const MemoryBlockEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const Key& key, const MemoryBlockEntry* value) const
	{
		return key == *value;
	}

	MemoryBlockEntry*& GetLink(MemoryBlockEntry* value) const
	{
		return value->next;
	}
};


TeamMemoryBlockManager::TeamMemoryBlockManager()
	:
	fActiveBlocks(NULL),
	fDeadBlocks(NULL)
{
}


TeamMemoryBlockManager::~TeamMemoryBlockManager()
{
	_Cleanup();
}


status_t
TeamMemoryBlockManager::Init()
{
	status_t result = fLock.InitCheck();
	if (result != B_OK)
		return result;

	fActiveBlocks = new(std::nothrow) MemoryBlockTable();
	if (fActiveBlocks == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<MemoryBlockTable> activeDeleter(fActiveBlocks);
	result = fActiveBlocks->Init();
	if (result != B_OK)
		return result;

	fDeadBlocks = new(std::nothrow) MemoryBlockTable();
	if (fDeadBlocks == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<MemoryBlockTable> deadDeleter(fDeadBlocks);
	result = fDeadBlocks->Init();
	if (result != B_OK)
		return result;

	activeDeleter.Detach();
	deadDeleter.Detach();

	return B_OK;
}


TeamMemoryBlock*
TeamMemoryBlockManager::GetMemoryBlock(target_addr_t address)
{
	AutoLocker<BLocker> lock(fLock);

	address &= ~B_PAGE_SIZE - 1;
	MemoryBlockEntry* entry = fActiveBlocks->Lookup(address);
	if (entry == NULL) {
		TeamMemoryBlock* block = new(std::nothrow) TeamMemoryBlock(address,
			this);
		if (block == NULL)
			return NULL;

		entry = new(std::nothrow) MemoryBlockEntry(block);
		if (entry == NULL) {
			delete block;
			return NULL;
		}

		fActiveBlocks->Insert(entry);
	}

	int32 refCount = entry->block->AcquireReference();
	if (refCount == 0) {
		// this block already had its last reference released,
		// move it to the dead list and retrieve a new one instead.
		_MarkDeadBlock(address);
		return GetMemoryBlock(address);
	}

	return entry->block;
}


void
TeamMemoryBlockManager::_Cleanup()
{
	if (fActiveBlocks != NULL) {
		MemoryBlockEntry* entry = fActiveBlocks->Clear(true);

		while (entry != NULL) {
			MemoryBlockEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fActiveBlocks;
		fActiveBlocks = NULL;
	}
}


void
TeamMemoryBlockManager::_MarkDeadBlock(target_addr_t address)
{
	AutoLocker<BLocker> lock(fLock);
	MemoryBlockEntry* entry = fActiveBlocks->Lookup(address);
	if (entry != NULL) {
		fActiveBlocks->Remove(entry);
		fDeadBlocks->Insert(entry);
	}
}


void
TeamMemoryBlockManager::_RemoveBlock(target_addr_t address)
{
	AutoLocker<BLocker> lock(fLock);
	MemoryBlockEntry* entry = fActiveBlocks->Lookup(address);
	if (entry != NULL) {
		fActiveBlocks->Remove(entry);
		delete entry;
		return;
	}

	entry = fDeadBlocks->Lookup(address);
	if (entry != NULL) {
		fDeadBlocks->Remove(entry);
		delete entry;
	}
}

