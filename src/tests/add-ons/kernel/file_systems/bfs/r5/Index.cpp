/* Index - index access functions
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Debug.h"
#include "Index.h"
#include "Volume.h"
#include "Inode.h"
#include "BPlusTree.h"

#include <util/kernel_cpp.h>
#include <TypeConstants.h>

// B_MIME_STRING_TYPE is defined in storage/Mime.h, but we
// don't need the whole file here; the type can't change anyway
#ifndef _MIME_H
#	define B_MIME_STRING_TYPE 'MIMS'
#endif


Index::Index(Volume *volume)
	:
	fVolume(volume),
	fNode(NULL)
{
}


Index::~Index()
{
	if (fNode == NULL)
		return;

	put_vnode(fVolume->ID(), fNode->ID());
}


void
Index::Unset()
{
	if (fNode == NULL)
		return;

	put_vnode(fVolume->ID(), fNode->ID());
	fNode = NULL;
	fName = NULL;
}


/** Sets the index to specified one. Returns an error if the index could
 *	not be found or initialized.
 *	Note, Index::Update() may be called on the object even if this method
 *	failed previously. In this case, it will only update live queries for
 *	the updated attribute.
 */

status_t 
Index::SetTo(const char *name)
{
	// remove the old node, if the index is set for the second time
	Unset();
	
	fName = name;
		// only stores the pointer, so it assumes that it will stay constant
		// in further comparisons (currently only used in Index::Update())

	// Note, the name is saved even if the index couldn't be initialized!
	// This is used to optimize Index::Update() in case there is no index

	Inode *indices = fVolume->IndicesNode();
	if (indices == NULL)
		return B_ENTRY_NOT_FOUND;

	BPlusTree *tree;
	if (indices->GetTree(&tree) != B_OK)
		return B_BAD_VALUE;

	vnode_id id;
	status_t status = tree->Find((uint8 *)name, (uint16)strlen(name), &id);
	if (status != B_OK)
		return status;

	Vnode vnode(fVolume, id);
	if (vnode.Get(&fNode) != B_OK)
		return B_ENTRY_NOT_FOUND;

	if (fNode == NULL) {
		FATAL(("fatal error at Index::InitCheck(), get_vnode() returned NULL pointer\n"));
		return B_ERROR;
	}

	vnode.Keep();
	return B_OK;
}


/** Returns a standard type code for the stat() index type codes. Returns
 *	zero if the type is not known (can only happen if the mode field is
 *	corrupted somehow or not that of an index).
 */

uint32 
Index::Type()
{
	if (fNode == NULL)
		return 0;

	switch (fNode->Mode() & (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX |
							 S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX)) {
		case S_INT_INDEX:
			return B_INT32_TYPE;
		case S_UINT_INDEX:
			return B_UINT32_TYPE;
		case S_LONG_LONG_INDEX:
			return B_INT64_TYPE;
		case S_ULONG_LONG_INDEX:
			return B_UINT64_TYPE;
		case S_FLOAT_INDEX:
			return B_FLOAT_TYPE;
		case S_DOUBLE_INDEX:
			return B_DOUBLE_TYPE;
		case S_STR_INDEX:
			return B_STRING_TYPE;
	}
	FATAL(("index has unknown type!\n"));
	return 0;
}


size_t
Index::KeySize()
{
	if (fNode == NULL)
		return 0;
	
	int32 mode = fNode->Mode() & (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX |
								  S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX);

	if (mode == S_STR_INDEX)
		// string indices don't have a fixed key size
		return 0;

	switch (mode) {
		case S_INT_INDEX:
		case S_UINT_INDEX:
			return sizeof(int32);
		case S_LONG_LONG_INDEX:
		case S_ULONG_LONG_INDEX:
			return sizeof(int64);
		case S_FLOAT_INDEX:
			return sizeof(float);
		case S_DOUBLE_INDEX:
			return sizeof(double);
	}
	FATAL(("index has unknown type!\n"));
	return 0;
}


status_t
Index::Create(Transaction *transaction, const char *name, uint32 type)
{
	Unset();

	int32 mode = 0;
	switch (type) {
		case B_INT32_TYPE:
			mode = S_INT_INDEX;
			break;
		case B_UINT32_TYPE:
			mode = S_UINT_INDEX;
			break;
		case B_INT64_TYPE:
			mode = S_LONG_LONG_INDEX;
			break;
		case B_UINT64_TYPE:
			mode = S_ULONG_LONG_INDEX;
			break;
		case B_FLOAT_TYPE:
			mode = S_FLOAT_INDEX;
			break;
		case B_DOUBLE_TYPE:
			mode = S_DOUBLE_INDEX;
			break;
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
			// B_MIME_STRING_TYPE is the only supported non-standard type, but
			// will be handled like a B_STRING_TYPE internally
			mode = S_STR_INDEX;
			break;
		default:
			return B_BAD_TYPE;
	}


	// do we need to create the index directory first?
	if (fVolume->IndicesNode() == NULL) {
		status_t status = fVolume->CreateIndicesRoot(transaction);
		if (status < B_OK)
			RETURN_ERROR(status);
	}

	// Inode::Create() will keep the inode locked for us
	return Inode::Create(transaction, fVolume->IndicesNode(), name,
		S_INDEX_DIR | S_DIRECTORY | mode, 0, type, NULL, &fNode);
}


/**	Updates the specified index, the oldKey will be removed from, the newKey
 *	inserted into the tree.
 *	If the method returns B_BAD_INDEX, it means the index couldn't be found -
 *	the most common reason will be that the index doesn't exist.
 *	You may not want to let the whole transaction fail because of that.
 */

status_t
Index::Update(Transaction *transaction, const char *name, int32 type, const uint8 *oldKey,
	uint16 oldLength, const uint8 *newKey, uint16 newLength, Inode *inode)
{
	if (name == NULL
		|| oldKey == NULL && newKey == NULL
		|| oldKey != NULL && oldLength == 0
		|| newKey != NULL && newLength == 0)
		return B_BAD_VALUE;

	// B_MIME_STRING_TYPE is the only supported non-standard type
	if (type == B_MIME_STRING_TYPE)
		type = B_STRING_TYPE;

	// If the two keys are identical, don't do anything - only compare if the
	// type has been set, until we have a real type code, we can't do much
	// about the comparison here
	if (type != 0 && !compareKeys(type, oldKey, oldLength, newKey, newLength))
		return B_OK;

	// update all live queries about the change, if they have an index or not
	if (type != 0)
		fVolume->UpdateLiveQueries(inode, name, type, oldKey, oldLength, newKey, newLength);

	status_t status;
	if (((name != fName || strcmp(name, fName)) && (status = SetTo(name)) < B_OK)
		|| fNode == NULL)
		return B_BAD_INDEX;

	// now that we have the type, check again for equality
	if (type == 0 && !compareKeys(Type(), oldKey, oldLength, newKey, newLength))
		return B_OK;

	// same for the live query update
	if (type == 0)
		fVolume->UpdateLiveQueries(inode, name, Type(), oldKey, oldLength, newKey, newLength);
		
	BPlusTree *tree;
	if ((status = Node()->GetTree(&tree)) < B_OK)
		return status;

	// remove the old key from the tree

	if (oldKey != NULL) {
		status = tree->Remove(transaction, (const uint8 *)oldKey, oldLength, inode->ID());
		if (status == B_ENTRY_NOT_FOUND) {
			// That's not nice, but should be no reason to let the whole thing fail
			FATAL(("Could not find value in index \"%s\"!\n", name));
		} else if (status < B_OK)
			return status;
	}

	// add the new key to the tree

	if (newKey != NULL)
		status = tree->Insert(transaction, (const uint8 *)newKey, newLength, inode->ID());

	return status;
}


status_t 
Index::InsertName(Transaction *transaction, const char *name, Inode *inode)
{
	return UpdateName(transaction, NULL, name, inode);
}


status_t 
Index::RemoveName(Transaction *transaction, const char *name, Inode *inode)
{
	return UpdateName(transaction, name, NULL, inode);
}


status_t 
Index::UpdateName(Transaction *transaction, const char *oldName, const char *newName, Inode *inode)
{
	uint16 oldLength = oldName ? strlen(oldName) : 0;
	uint16 newLength = newName ? strlen(newName) : 0;
	return Update(transaction, "name", B_STRING_TYPE, (uint8 *)oldName, oldLength,
		(uint8 *)newName, newLength, inode);
}


status_t 
Index::InsertSize(Transaction *transaction, Inode *inode)
{
	off_t size = inode->Size();
	return Update(transaction, "size", B_INT64_TYPE, NULL, 0, (uint8 *)&size, sizeof(int64), inode);
}


status_t 
Index::RemoveSize(Transaction *transaction, Inode *inode)
{
	// Inode::OldSize() is the size that's in the index
	off_t size = inode->OldSize();
	return Update(transaction, "size", B_INT64_TYPE, (uint8 *)&size, sizeof(int64), NULL, 0, inode);
}


status_t
Index::UpdateSize(Transaction *transaction, Inode *inode)
{
	off_t oldSize = inode->OldSize();
	off_t newSize = inode->Size();

	status_t status = Update(transaction, "size", B_INT64_TYPE, (uint8 *)&oldSize,
		sizeof(int64), (uint8 *)&newSize, sizeof(int64), inode);
	if (status == B_OK)
		inode->UpdateOldSize();

	return status;
}


status_t 
Index::InsertLastModified(Transaction *transaction, Inode *inode)
{
	off_t modified = inode->LastModified();
	return Update(transaction, "last_modified", B_INT64_TYPE, NULL, 0,
		(uint8 *)&modified, sizeof(int64), inode);
}


status_t 
Index::RemoveLastModified(Transaction *transaction, Inode *inode)
{
	// Inode::OldLastModified() is the value which is in the index
	off_t modified = inode->OldLastModified();
	return Update(transaction, "last_modified", B_INT64_TYPE, (uint8 *)&modified,
		sizeof(int64), NULL, 0, inode);
}


status_t 
Index::UpdateLastModified(Transaction *transaction, Inode *inode, off_t modified)
{
	off_t oldModified = inode->OldLastModified();
	if (modified == -1)
		modified = (bigtime_t)time(NULL) << INODE_TIME_SHIFT;
	modified |= fVolume->GetUniqueID() & INODE_TIME_MASK;

	status_t status = Update(transaction, "last_modified", B_INT64_TYPE, (uint8 *)&oldModified,
		sizeof(int64), (uint8 *)&modified, sizeof(int64), inode);

	inode->Node()->last_modified_time = modified;
	if (status == B_OK)
		inode->UpdateOldLastModified();

	return status;
}

