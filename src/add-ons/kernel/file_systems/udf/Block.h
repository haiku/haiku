//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _UDF_BLOCK_H
#define _UDF_BLOCK_H

#include <malloc.h>

#include "cpp.h"

namespace UDF {

class Block {
public:
	Block(uint32 size)
		: fSize(size)
		, fData(malloc(size))
	{
	}
		
	~Block() { free(fData); }	
	uint32 Size() { return fSize; }	
	void* Data() { return fData; }
	status_t InitCheck() { return fData ? B_OK : B_NO_MEMORY; }
	
private:
	Block();
	Block(const Block&);
	Block& operator=(const Block&);

	uint32 fSize;
	void *fData;
};

};	// namespace UDF

#endif	// _UDF_BLOCK_H
