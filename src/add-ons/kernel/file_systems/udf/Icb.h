//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_ICB_H
#define _UDF_ICB_H

/*! \file Icb.h
*/

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include <fsproto.h>
}

#include "cpp.h"
#include "UdfDebug.h"

#include "DiskStructures.h"
#include "MemoryChunk.h"
#include "SinglyLinkedList.h"

namespace Udf {

class DirectoryIterator;
class Volume;

class Icb {
public:
	Icb(Volume *volume, udf_long_address address);
	status_t InitCheck();
	vnode_id Id() { return fId; }
	
	// categorization
	uint8 Type() { return IcbTag().file_type(); }
	bool IsFile() { return InitCheck() == B_OK && Type() == ICB_TYPE_REGULAR_FILE; }
	bool IsDirectory() { return InitCheck() == B_OK
	                     && (Type() == ICB_TYPE_DIRECTORY || Type() == ICB_TYPE_STREAM_DIRECTORY); }
	
	// directories
	status_t GetDirectoryIterator(DirectoryIterator **iterator);
	
private:
	Icb();	// unimplemented
	
	udf_icb_entry_tag& IcbTag() { return (reinterpret_cast<udf_icb_header*>(fIcbData.Data()))->icb_tag(); }


	
private:
	Volume *fVolume;
	MemoryChunk fIcbData;
	status_t fInitStatus;
	vnode_id fId;
	SinglyLinkedList<Strategy::SinglyLinkedList::Auto<DirectoryIterator*> > fIteratorList;
};

};	// namespace Udf

#endif	// _UDF_ICB_H
