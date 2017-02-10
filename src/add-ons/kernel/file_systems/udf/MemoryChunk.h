//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _UDF_MEMORY_CHUNK_H
#define _UDF_MEMORY_CHUNK_H

#include <malloc.h>

#include <util/kernel_cpp.h>

/*! Simple class to encapsulate the boring details of allocating
	and deallocating a chunk of memory.
	
	The main use for this class is cleanly and simply allocating
	arbitrary chunks of data on the stack.
*/
class MemoryChunk {
public:
	MemoryChunk(uint32 blockSize)
		: fSize(blockSize)
		, fData(malloc(blockSize))
		, fOwnsData(true)
	{
	}
	
	MemoryChunk(uint32 blockSize, void *blockData)
		: fSize(blockSize)
		, fData(blockData)
		, fOwnsData(false)
	{
	}
		
	~MemoryChunk()
	{
		if (fOwnsData)
			free(Data());
	}
	
	uint32 Size() { return fSize; }	
	void* Data() { return fData; }
	status_t InitCheck() { return Data() ? B_OK : B_NO_MEMORY; }
	
private:
	MemoryChunk();
	MemoryChunk(const MemoryChunk&);
	MemoryChunk& operator=(const MemoryChunk&);

	uint32 fSize;
	void *fData;
	bool fOwnsData;
};

template <uint32 size>
class StaticMemoryChunk {
public:
	uint32 Size() { return size; }	
	void* Data() { return reinterpret_cast<void*>(fData); }
	status_t InitCheck() { return B_OK; }

private:
	uint8 fData[size];
};

#endif	// _UDF_MEMORY_CHUNK_H
