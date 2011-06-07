// BlockReferenceManager.cpp

#include "AllocationInfo.h"
#include "Block.h"
#include "BlockAllocator.h"		// only for BA_PANIC
#include "BlockReferenceManager.h"
#include "Debug.h"

static const int kBlockReferenceTableSize = 128;

// constructor
BlockReferenceManager::BlockReferenceManager()
	: fTables(10),
	  fFreeList(NULL)
{
}

// destructor
BlockReferenceManager::~BlockReferenceManager()
{
}

// AllocateReference
BlockReference *
BlockReferenceManager::AllocateReference()
{
	BlockReference *reference = NULL;
	if (!fFreeList)
		_AddTable();
	if (fFreeList) {
		reference = fFreeList;
		fFreeList = *(BlockReference**)fFreeList;
	}
	return reference;
}

// FreeReference
void
BlockReferenceManager::FreeReference(BlockReference *reference)
{
	if (reference) {
		*(BlockReference**)reference = fFreeList;
		fFreeList = reference;
	}
}

// CheckReference
bool
BlockReferenceManager::CheckReference(BlockReference *reference)
{
	if (reference) {
		uint32 address = (uint32)reference;
		int32 tableCount = fTables.CountItems();
		for (int32 i = 0; i < tableCount; i++) {
			Table *table = &fTables.ItemAt(i);
			uint32 first = (uint32)table->GetReferences();
			uint32 last = (uint32)(table->GetReferences() + table->GetSize());
			if (first <= address && address < last)
				return true;
		}
	}
	FATAL(("BlockReference %p does not exist!\n", reference));
	BA_PANIC("BlockReference doesn't exist.");
	return false;
}

// GetAllocationInfo
void
BlockReferenceManager::GetAllocationInfo(AllocationInfo &info)
{
	info.AddListAllocation(fTables.GetCapacity(), sizeof(Table));
	int32 count = fTables.CountItems();
	for (int32 i = 0; i < count; i++) {
		Table &table = fTables.ItemAt(i);
		info.AddOtherAllocation(table.GetSize() * sizeof(BlockReference));
	}
}

// _AddTable
status_t
BlockReferenceManager::_AddTable()
{
	status_t error = B_OK;
	// add a new table
	Table dummy;
	if (fTables.AddItem(dummy)) {
		int32 index = fTables.CountItems() - 1;
		Table &table = fTables.ItemAt(index);
		error = table.Init(kBlockReferenceTableSize);
		if (error == B_OK) {
			// add the references to the free list
			uint32 count = table.GetSize();
			BlockReference *references = table.GetReferences();
			for (uint32 i = 0; i < count; i++) {
				BlockReference *reference = references + i;
				*(BlockReference**)reference = fFreeList;
				fFreeList = reference;
			}
		} else
			fTables.RemoveItem(index);
	} else
		SET_ERROR(error, B_NO_MEMORY);
	return error;
}


// Table

// destructor
BlockReferenceManager::Table::~Table()
{
	if (fReferences)
		delete[] fReferences;
}

// Init
status_t
BlockReferenceManager::Table::Init(int32 size)
{
	status_t error = (size > 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fReferences = new(std::nothrow) BlockReference[size];
		if (fReferences)
			fSize = size;
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

