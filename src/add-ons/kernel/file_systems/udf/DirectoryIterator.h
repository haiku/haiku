//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_DIRECTORY_ITERATOR_H
#define _UDF_DIRECTORY_ITERATOR_H

/*! \file DirectoryIterator.h
*/

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include <fsproto.h>
}

#include "cpp.h"
#include "UdfDebug.h"

namespace Udf {

class Icb;

class DirectoryIterator {
public:

	status_t GetNextEntry(vnode_id *id, char *name, uint32 *length);
	void Rewind();
	
	Icb* Parent() const { return fParent; }
	
private:
	friend class Icb;

	DirectoryIterator();				// unimplemented
	DirectoryIterator(Icb *parent);		// called by Icb::GetDirectoryIterator()
	void Invalidate();					// called by Icb::~Icb() 

	Icb *fParent;
	uint32 fCount;						// Used to implement fake iteration until file reading is implemented
};

};	// namespace Udf

#endif	// _UDF_DIRECTORY_ITERATOR_H
