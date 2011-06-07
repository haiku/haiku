// BlockReferenceManager.h

#ifndef BLOCK_REFERENCE_MANAGER_H
#define BLOCK_REFERENCE_MANAGER_H

#include <new>
#include "List.h"

class AllocationInfo;
class BlockReference;

class BlockReferenceManager {
public:
	BlockReferenceManager();
	~BlockReferenceManager();

	BlockReference *AllocateReference();
	void FreeReference(BlockReference *reference);

	// debugging only
	bool CheckReference(BlockReference *reference);
	void GetAllocationInfo(AllocationInfo &info);

private:
	status_t _AddTable();

private:
	class Table {
	public:
		Table() : fSize(0), fReferences(NULL) {}
		Table(int) : fSize(0), fReferences(NULL) {}
		~Table();

		status_t Init(int32 size);

		BlockReference *GetReferences()	{ return fReferences; }

		int32 GetSize() const { return fSize; }

	private:
		uint32			fSize;
		BlockReference	*fReferences;
	};

	List<Table>			fTables;
	BlockReference		*fFreeList;
};

#endif	// BLOCK_REFERENCE_MANAGER_H

