//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file EntryList.h
	BEntryList interface declaration.
*/

#ifndef __sk_entry_list_h__
#define __sk_entry_list_h__

#include <dirent.h>
#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// Forward declarations
class BEntry;
struct entry_ref;

//! Interface for iterating through a list of filesystem entries
/*! Defines a general interface for iterating through a list of entries (i.e.
	files in a folder
	
	@author <a href='mailto:tylerdauwalder@users.sf.net'>Tyler Dauwalder</a>
	@author Be Inc.
	@version 0.0.0
*/
class BEntryList {
public:
	BEntryList();
	virtual ~BEntryList();
	
	virtual status_t GetNextEntry(BEntry *entry, bool traverse = false) = 0;
	virtual status_t GetNextRef(entry_ref *ref) = 0;
	virtual int32 GetNextDirents(struct dirent *buf, size_t length,
								 int32 count = INT_MAX) = 0;
	virtual status_t Rewind() = 0;
	virtual int32 CountEntries() = 0;
	
private:
	virtual	void _ReservedEntryList1();
	virtual	void _ReservedEntryList2();
	virtual	void _ReservedEntryList3();
	virtual	void _ReservedEntryList4();
	virtual	void _ReservedEntryList5();
	virtual	void _ReservedEntryList6();
	virtual	void _ReservedEntryList7();
	virtual	void _ReservedEntryList8();
	
};

#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

#endif	// __sk_entry_list_h__
