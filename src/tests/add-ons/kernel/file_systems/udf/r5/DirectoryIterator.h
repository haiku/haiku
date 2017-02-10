//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_DIRECTORY_ITERATOR_H
#define _UDF_DIRECTORY_ITERATOR_H

/*! \file DirectoryIterator.h
*/

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif
#ifdef COMPILE_FOR_R5
extern "C" {
#endif
	#include "fsproto.h"
#ifdef COMPILE_FOR_R5
}
#endif

#include "kernel_cpp.h"
#include "UdfDebug.h"

namespace Udf {

class Icb;

class DirectoryIterator {
public:

	status_t GetNextEntry(char *name, uint32 *length, vnode_id *id);
	void Rewind();
	
	Icb* Parent() { return fParent; }
	const Icb* Parent() const { return fParent; }
	
private:
	friend class Icb;

	DirectoryIterator();				// unimplemented
	DirectoryIterator(Icb *parent);		// called by Icb::GetDirectoryIterator()
	void Invalidate();					// called by Icb::~Icb() 

	Icb *fParent;
	off_t fPosition;
	bool fAtBeginning;
};

};	// namespace Udf

#endif	// _UDF_DIRECTORY_ITERATOR_H
