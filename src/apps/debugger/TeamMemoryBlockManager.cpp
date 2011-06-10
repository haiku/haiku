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
	}

	~MemoryBlockEntry()
	{
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

	fDeadBlocks = new(std::nothrow) DeadBlockTable();
	if (fDeadBlocks == NULL)
		return B_NO_MEMORY;

	activeDeleter.Detach();

	return B_OK;
}


TeamMemoryBlock*
TeamMemoryBlockManager::GetMemoryBlock(target_addr_t address)
{
	AutoLocker<BLocker> lock(fLock);

	address &= ~B_PAGE_SIZE - 1;
	MemoryBlockEntry* entry = fActiveBlocks->Lookup(address);
	if (entry != NULL) {
		if (entry->block->AcquireReference() != 0)
			return entry->block;

		// this block already had its last reference released,
		// move it to the dead list and create a new one instead.
		_MarkDeadBlock(address);
	}

	TeamMemoryBlockOwner* owner = new(std::nothrow) TeamMemoryBlockOwner(this);
	if (owner == NULL)
		return NULL;
	ObjectDeleter<TeamMemoryBlockOwner> ownerDeleter(owner);

	TeamMemoryBlock* block = new(std::nothrow) TeamMemoryBlock(address,
		owner);
	if (block == NULL)
		return NULL;
	ObjectDeleter<TeamMemoryBlock> blockDeleter(block);

	entry = new(std::nothrow) MemoryBlockEntry(block);
	if (entry == NULL)
		return NULL;

	ownerDeleter.Detach();
	blockDeleter.Detach();
	fActiveBlocks->Insert(entry);

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
	MemoryBlockEntry* entry = fActiveBlocks->Lookup(address);
	if (entry != NULL) {
		fActiveBlocks->Remove(entry);
		fDeadBlocks->Insert(entry->block);
		delete entry;
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

	DeadBlockTable::Iterator iterator = fDeadBlocks->GetIterator();
	while (iterator.HasNext()) {
		TeamMemoryBlock* block = iterator.Next();
		if (block->BaseAddress() == address) {
			fDeadBlocks->Remove(block);
			break;
		}
	}
}


TeamMemoryBlockOwner::TeamMemoryBlockOwner(TeamMemoryBlockManager* manager)
	:
	fBlockManager(manager)
{
}


TeamMemoryBlockOwner::~TeamMemoryBlockOwner()
{
}


void
TeamMemoryBlockOwner::RemoveBlock(TeamMemoryBlock* block)
{
	fBlockManager->_RemoveBlock(block->BaseAddress());
}
