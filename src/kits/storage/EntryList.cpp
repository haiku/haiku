//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file EntryList.cpp
	BEntryList implementation.
*/

#include <EntryList.h>

// constructor
//!	Creates a BEntryList.
/*!	Does nothing at this time.
*/
BEntryList::BEntryList()
{
}

// destructor
//!	Frees all resources associated with this BEntryList.
/*!	Does nothing at this time.
*/
BEntryList::~BEntryList()
{
}

// GetNextEntry
/*!	\fn status_t BEntryList::GetNextEntry(BEntry *entry, bool traverse)
	\brief Returns the BEntryList's next entry as a BEntry.
	Places the next entry in the list in \a entry, traversing symlinks if
	\a traverse is \c true.
	\param entry a pointer to a BEntry to be initialized with the found entry
	\param traverse specifies whether to follow it, if the found entry
		   is a symbolic link.
	\note The iterator used by this method is the same one used by
		  GetNextRef(), GetNextDirents(), Rewind() and CountEntries().
	\return
	- \c B_OK if successful,
	- \c B_ENTRY_NOT_FOUND when at the end of the list,
	- another error code (depending on the implementation of the derived class)
	  if an error occured.
*/

// GetNextRef
/*!	\fn status_t BEntryList::GetNextRef(entry_ref *ref)
	\brief Returns the BEntryList's next entry as an entry_ref.
	Places an entry_ref to the next entry in the list into \a ref.
	\param ref a pointer to an entry_ref to be filled in with the data of the
		   found entry
	\note The iterator used by this method is the same one used by
		  GetNextEntry(), GetNextDirents(), Rewind() and CountEntries().
	\return
	- \c B_OK if successful,
	- \c B_ENTRY_NOT_FOUND when at the end of the list,
	- another error code (depending on the implementation of the derived class)
	  if an error occured.
*/

// GetNextDirents
/*!	\fn int32 BEntryList::GetNextDirents(struct dirent *buf, size_t length, int32 count)
	\brief Returns the BEntryList's next entries as dirent structures.
	Reads a number of entries into the array of dirent structures pointed to by
	\a buf. Reads as many but no more than \a count entries, as many entries as
	remain, or as many entries as will fit into the array at \a buf with given
	length \a length (in bytes), whichever is smallest.
	\param buf a pointer to a buffer to be filled with dirent structures of
		   the found entries
	\param length the maximal number of entries to be read.
	\note The iterator used by this method is the same one used by
		  GetNextEntry(), GetNextRef(), Rewind() and CountEntries().
	\return
	- The number of dirent structures stored in the buffer, 0 when there are
	  no more entries to be read.
	- an error code (depending on the implementation of the derived class)
	  if an error occured.
*/

// Rewind
/*!	\fn status_t BEntryList::Rewind()
	\brief Rewinds the list pointer to the beginning of the list.
	\return
	- \c B_OK if successful,
	- an error code (depending on the implementation of the derived class)
	  if an error occured.
*/

// CountEntries
/*!	\fn int32 BEntryList::CountEntries()
	\brief Returns the number of entries in the list
	\return
	- the number of entries in the list,
	- an error code (depending on the implementation of the derived class)
	  if an error occured.
*/


/*! Currently unused */
void BEntryList::_ReservedEntryList1() {}
void BEntryList::_ReservedEntryList2() {}
void BEntryList::_ReservedEntryList3() {}
void BEntryList::_ReservedEntryList4() {}
void BEntryList::_ReservedEntryList5() {}
void BEntryList::_ReservedEntryList6() {}
void BEntryList::_ReservedEntryList7() {}
void BEntryList::_ReservedEntryList8() {}

