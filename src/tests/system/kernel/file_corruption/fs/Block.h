/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_H
#define BLOCK_H


#include <SupportDefs.h>


class Transaction;
class Volume;


class Block {
public:
	inline						Block();
	inline						~Block();

			void				TransferFrom(Block& other);

			bool				GetReadable(Volume* volume, uint64 blockIndex);
			bool				GetWritable(Volume* volume, uint64 blockIndex,
									Transaction& transaction);
			bool				GetZero(Volume* volume, uint64 blockIndex,
									Transaction& transaction);

			status_t			MakeWritable(Transaction& transaction);

			void				Put();

			void*				Data() const	{ return fData; }
			uint64				Index() const	{ return fIndex; }

private:
			bool				_Init(Volume* volume, uint64 blockIndex,
									const void* data, Transaction* transaction);

private:
			Volume*				fVolume;
			void*				fData;
			uint64				fIndex;
			Transaction*		fTransaction;
};


Block::Block()
	:
	fVolume(NULL),
	fData(NULL)
{
}


Block::~Block()
{
	Put();
}


#endif	// BLOCK_H
