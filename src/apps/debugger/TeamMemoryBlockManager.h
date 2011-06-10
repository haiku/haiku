/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_MEMORY_BLOCK_MANAGER_H
#define TEAM_MEMORY_BLOCK_MANAGER_H


#include <Locker.h>
#include <Referenceable.h>
#include <util/OpenHashTable.h>

#include "Types.h"

struct MemoryBlockHashDefinition;
class TeamMemoryBlock;


class TeamMemoryBlockManager
{
public:
								TeamMemoryBlockManager();
								~TeamMemoryBlockManager();

		status_t				Init();

		TeamMemoryBlock*		GetMemoryBlock(target_addr_t address);

private:
		struct Key;
		struct MemoryBlockEntry;
		struct MemoryBlockHashDefinition;
		typedef BOpenHashTable<MemoryBlockHashDefinition> MemoryBlockTable;

private:
		void					_Cleanup();
		void					_MarkDeadBlock(target_addr_t address);
		void					_RemoveBlock(target_addr_t address);

private:
		friend class TeamMemoryBlock;

private:
		BLocker					fLock;
		MemoryBlockTable*		fActiveBlocks;
		MemoryBlockTable*		fDeadBlocks;
};


#endif // TEAM_MEMORY_BLOCK_MANAGER_H
