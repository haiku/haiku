/* Inode - inode access functions
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "Debug.h"
#include "cpp.h"
#include "Inode.h"
#include "BPlusTree.h"
#include "Index.h"

#include <string.h>


class InodeAllocator {
	public:
		InodeAllocator(Transaction *transaction);
		~InodeAllocator();

		status_t New(block_run *parentRun,mode_t mode,block_run &run,Inode **inode);
		void Keep();

	private:
		Transaction *fTransaction;
		block_run fRun;
		Inode *fInode;
};


InodeAllocator::InodeAllocator(Transaction *transaction)
	:
	fTransaction(transaction),
	fInode(NULL)
{
}


InodeAllocator::~InodeAllocator()
{
	delete fInode;
	
	if (fTransaction)
		fTransaction->GetVolume()->Free(fTransaction,fRun);
}


status_t 
InodeAllocator::New(block_run *parentRun, mode_t mode, block_run &run, Inode **inode)
{
	Volume *volume = fTransaction->GetVolume();

	status_t status = volume->AllocateForInode(fTransaction,parentRun,mode,fRun);
	if (status < B_OK) {
		// don't free the space in the destructor, because
		// the allocation failed
		fTransaction = NULL;
		RETURN_ERROR(status);
	}

	run = fRun;
	fInode = new Inode(volume,volume->ToVnode(run),true);
	if (fInode == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*inode = fInode;
	return B_OK;
}


void InodeAllocator::Keep()
{
	fTransaction = NULL;
	fInode = NULL;
}


//	#pragma mark -


Inode::Inode(Volume *volume,vnode_id id,bool empty,uint8 reenter)
	: CachedBlock(volume,volume->VnodeToBlock(id),empty),
	fTree(NULL),
	fLock("bfs inode")
{
	Node()->flags &= INODE_PERMANENT_FLAGS;

	// these two will help to maintain the indices
	fOldSize = Size();
	fOldLastModified = Node()->last_modified_time;
}


Inode::~Inode()
{
	delete fTree;
}


status_t 
Inode::InitCheck()
{
	if (!Node())
		RETURN_ERROR(B_IO_ERROR);

	// test inode magic and flags
	if (Node()->magic1 != INODE_MAGIC1
		|| !(Node()->flags & INODE_IN_USE)
		|| Node()->inode_num.length != 1
		// matches inode size?
		|| Node()->inode_size != fVolume->InodeSize()
		// parent resides on disk?
		|| Node()->parent.allocation_group > fVolume->AllocationGroups()
		|| Node()->parent.allocation_group < 0
		|| Node()->parent.start > (1L << fVolume->AllocationGroupShift())
		|| Node()->parent.length != 1
		// attributes, too?
		|| Node()->attributes.allocation_group > fVolume->AllocationGroups()
		|| Node()->attributes.allocation_group < 0
		|| Node()->attributes.start > (1L << fVolume->AllocationGroupShift())) {
		FATAL(("inode at block %Ld corrupt!\n",fBlockNumber));
		RETURN_ERROR(B_BAD_DATA);
	}
	
	// ToDo: Add some tests to check the integrity of the other stuff here,
	// especially for the data_stream!

	// it's more important to know that the inode is corrupt
	// so we check for the lock not until here
	return fLock.InitCheck();
}


status_t 
Inode::CheckPermissions(int accessMode) const
{
	uid_t user = geteuid();
	gid_t group = getegid();

	// you never have write access to a read-only volume
	if (accessMode & W_OK && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// root users always have full access (but they can't execute anything)
	if (user == 0 && !((accessMode & X_OK) && (Mode() & S_IXUSR) == 0))
		return B_OK;

	// shift mode bits, to check directly against accessMode
	mode_t mode = Mode();
	if (user == Node()->uid)
		mode >>= 6;
	else if (group == Node()->gid)
		mode >>= 3;

	if (accessMode & ~(mode & S_IRWXO))
		return B_NOT_ALLOWED;

	return B_OK;
}


//	#pragma mark -


void 
Inode::AddIterator(AttributeIterator *iterator)
{
	if (fSmallDataLock.Lock() < B_OK)
		return;

	fIterators.Add(iterator);

	fSmallDataLock.Unlock();
}


void 
Inode::RemoveIterator(AttributeIterator *iterator)
{
	if (fSmallDataLock.Lock() < B_OK)
		return;

	fIterators.Remove(iterator);

	fSmallDataLock.Unlock();
}


/**	Tries to free up "bytes" space in the small_data section by moving
 *	attributes to real files. Used for system attributes like the name.
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t
Inode::MakeSpaceForSmallData(Transaction *transaction,const char *name,int32 bytes)
{
	while (bytes > 0) {
		small_data *item = Node()->small_data_start,*max = NULL;
		int32 index = 0,maxIndex = 0;
		for (;!item->IsLast(Node());item = item->Next(),index++) {
			// should not remove those
			if (*item->Name() == FILE_NAME_NAME || !strcmp(name,item->Name()))
				continue;

			if (max == NULL || max->Size() < item->Size()) {
				maxIndex = index;
				max = item;
			}

			// remove the first one large enough to free the needed amount of bytes
			if (bytes < item->Size())
				break;
		}

		if (item->IsLast(Node()) || item->Size() < bytes)
			return B_ERROR;

		bytes -= max->Size();

		// Move the attribute to a real attribute file
		// Luckily, this doesn't cause any index updates

		Inode *attribute;
		status_t status = CreateAttribute(transaction,item->Name(),item->type,&attribute);
		if (status < B_OK)
			RETURN_ERROR(status);

		size_t length = item->data_size;
		status = attribute->WriteAt(transaction,0,item->Data(),&length);

		ReleaseAttribute(attribute);

		if (status < B_OK) {
			Vnode vnode(fVolume,Attributes());
			Inode *attributes;
			if (vnode.Get(&attributes) < B_OK
				|| attributes->Remove(transaction,name) < B_OK) {
				FATAL(("Could not remove newly created attribute!\n"));
			}

			RETURN_ERROR(status);
		}

		RemoveSmallData(max,maxIndex);
	}
	return B_OK;
}


/**	Private function which removes the given attribute from the small_data
 *	section.
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t 
Inode::RemoveSmallData(small_data *item,int32 index)
{
	small_data *next = item->Next();
	if (!next->IsLast(Node())) {
		// find the last attribute
		small_data *last = next;
		while (!last->IsLast(Node()))
			last = last->Next();

		int32 size = (uint8 *)last - (uint8 *)next;
		if (size < 0 || size > (uint8 *)Node() + fVolume->BlockSize() - (uint8 *)next)
			return B_BAD_DATA;

		memmove(item,next,size);

		// Move the "last" one to its new location and
		// correctly terminate the small_data section
		last = (small_data *)((uint8 *)last - ((uint8 *)next - (uint8 *)item));
		memset(last,0,(uint8 *)Node() + fVolume->BlockSize() - (uint8 *)last);
	} else
		memset(item,0,item->Size());

	// update all current iterators
	AttributeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Update(index,-1);

	return B_OK;
}


/**	Removes the given attribute from the small_data section.
 *	Note that you need to write back the inode yourself after having called
 *	that method.
 */

status_t
Inode::RemoveSmallData(Transaction *transaction,const char *name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	SimpleLocker locker(fSmallDataLock);

	// search for the small_data item

	small_data *item = Node()->small_data_start;
	int32 index = 0;
	while (!item->IsLast(Node()) && strcmp(item->Name(),name)) {
		item = item->Next();
		index++;
	}

	if (item->IsLast(Node()))
		return B_ENTRY_NOT_FOUND;

	return RemoveSmallData(item,index);
}


/**	Try to place the given attribute in the small_data section - if the
 *	new attribute is too big to fit in that section, it returns B_DEVICE_FULL.
 *	In that case, the attribute should be written to a real attribute file;
 *	if the attribute was already part of the small_data section, but the new
 *	one wouldn't fit, the old one is automatically removed from the small_data
 *	section.
 *	Note that you need to write back the inode yourself after having called that
 *	method - it's a bad API decision that it needs a transaction but enforces you
 *	to write back the inode all by yourself, but it's just more efficient in most
 *	cases...
 */

status_t
Inode::AddSmallData(Transaction *transaction,const char *name,uint32 type,const uint8 *data,size_t length,bool force)
{
	if (name == NULL || data == NULL || type == 0)
		return B_BAD_VALUE;

	// reject any requests that can't fit into the small_data section
	uint32 nameLength = strlen(name);
	uint32 spaceNeeded = sizeof(small_data) + nameLength + 3 + length + 1;
	if (spaceNeeded > fVolume->InodeSize() - sizeof(bfs_inode))
		return B_DEVICE_FULL;

	SimpleLocker locker(fSmallDataLock);

	small_data *item = Node()->small_data_start;
	int32 index = 0;
	while (!item->IsLast(Node()) && strcmp(item->Name(),name)) {
		item = item->Next();
		index++;
	}

	// is the attribute already in the small_data section?
	// then just replace the data part of that one
	if (!item->IsLast(Node())) {
		// find last attribute
		small_data *last = item;
		while (!last->IsLast(Node()))
			last = last->Next();

		// try to change the attributes value
		if (item->data_size > length
			|| force
			|| ((uint8 *)last + length - item->data_size) <= ((uint8 *)Node() + fVolume->InodeSize())) {
			// make room for the new attribute if needed (and we are forced to do so)
			if (force
				&& ((uint8 *)last + length - item->data_size) > ((uint8 *)Node() + fVolume->InodeSize())) {
				// We also take the free space at the end of the small_data section
				// into account, and request only what's really needed
				uint32 needed = length - item->data_size -
						(uint32)((uint8 *)Node() + fVolume->InodeSize() - (uint8 *)last);

				if (MakeSpaceForSmallData(transaction,name,needed) < B_OK)
					return B_ERROR;
				
				// reset our pointers
				item = Node()->small_data_start;
				index = 0;
				while (!item->IsLast(Node()) && strcmp(item->Name(),name)) {
					item = item->Next();
					index++;
				}

				last = item;
				while (!last->IsLast(Node()))
					last = last->Next();
			}

			// move the attributes after the current one
			small_data *next = item->Next();
			if (!next->IsLast(Node()))
				memmove((uint8 *)item + spaceNeeded,next,(uint8 *)last - (uint8 *)next);

			// Move the "last" one to its new location and
			// correctly terminate the small_data section
			last = (small_data *)((uint8 *)last - ((uint8 *)next - ((uint8 *)item + spaceNeeded)));
			if ((uint8 *)last < (uint8 *)Node() + fVolume->BlockSize())
				memset(last,0,(uint8 *)Node() + fVolume->BlockSize() - (uint8 *)last);

			item->type = type;
			item->data_size = length;
			memcpy(item->Data(),data,length);
			item->Data()[length] = '\0';

			return B_OK;
		}

		// Could not replace the old attribute, so remove it to let
		// let the calling function create an attribute file for it
		if (RemoveSmallData(item,index) < B_OK)
			return B_ERROR;

		return B_DEVICE_FULL;
	}

	// try to add the new attribute!

	if ((uint8 *)item + spaceNeeded > (uint8 *)Node() + fVolume->InodeSize()) {
		// there is not enough space for it!
		if (!force)
			return B_DEVICE_FULL;

		// make room for the new attribute
		if (MakeSpaceForSmallData(transaction,name,spaceNeeded) < B_OK)
			return B_ERROR;

		// get new last item!
		item = Node()->small_data_start;
		index = 0;
		while (!item->IsLast(Node())) {
			item = item->Next();
			index++;
		}
	}

	memset(item,0,spaceNeeded);
	item->type = type;
	item->name_size = nameLength;
	item->data_size = length;
	strcpy(item->Name(),name);
	memcpy(item->Data(),data,length);

	// correctly terminate the small_data section
	item = item->Next();
	if (!item->IsLast(Node()))
		memset(item,0,(uint8 *)Node() + fVolume->InodeSize() - (uint8 *)item);

	// update all current iterators
	AttributeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Update(index,1);

	return B_OK;
}


/**	Iterates through the small_data section of an inode.
 *	To start at the beginning of this section, you let smallData
 *	point to NULL, like:
 *		small_data *data = NULL;
 *		while (inode->GetNextSmallData(&data) { ... }
 *
 *	This function is reentrant and doesn't allocate any memory;
 *	you can safely stop calling it at any point (you don't need
 *	to iterate through the whole list).
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t
Inode::GetNextSmallData(small_data **smallData) const
{
	if (!Node())
		RETURN_ERROR(B_ERROR);

	small_data *data = *smallData;

	// begin from the start?
	if (data == NULL)
		data = Node()->small_data_start;
	else
		data = data->Next();

	// is already last item?
	if (data->IsLast(Node()))
		return B_ENTRY_NOT_FOUND;

	*smallData = data;

	return B_OK;
}


/**	Finds the attribute "name" in the small data section, and
 *	returns a pointer to it (or NULL if it doesn't exist).
 *	You need to hold the fSmallDataLock when you call this method
 */

small_data *
Inode::FindSmallData(const char *name) const
{
	small_data *smallData = NULL;
	while (GetNextSmallData(&smallData) == B_OK) {
		if (!strcmp(smallData->Name(),name))
			return smallData;
	}
	return NULL;
}


const char *
Inode::Name() const
{
	SimpleLocker locker(fSmallDataLock);

	small_data *smallData = NULL;
	while (GetNextSmallData(&smallData) == B_OK) {
		if (*smallData->Name() == FILE_NAME_NAME && smallData->name_size == FILE_NAME_NAME_LENGTH)
			return (const char *)smallData->Data();
	}
	return NULL;
}


/**	Changes or set the name of a file: in the inode small_data section only, it
 *	doesn't change it in the parent directory's b+tree.
 *	Note that you need to write back the inode yourself after having called
 *	that method. It suffers from the same API decision as AddSmallData() does
 *	(and for the same reason).
 */

status_t 
Inode::SetName(Transaction *transaction,const char *name)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	const char nameTag[2] = {FILE_NAME_NAME, 0};

	return AddSmallData(transaction,nameTag,FILE_NAME_TYPE,(uint8 *)name,strlen(name),true);
}


/**	Reads data from the specified attribute.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::ReadAttribute(const char *name,int32 type,off_t pos,uint8 *buffer,size_t *_length)
{
	if (pos < 0)
		pos = 0;

	// search in the small_data section (which has to be locked first)
	{
		SimpleLocker locker(fSmallDataLock);

		small_data *smallData = FindSmallData(name);
		if (smallData != NULL) {
			size_t length = *_length;
			if (pos >= smallData->data_size) {
				*_length = 0;
				return B_OK;
			}
			if (length + pos > smallData->data_size)
				length = smallData->data_size - pos;
	
			memcpy(buffer,smallData->Data() + pos,length);
			*_length = length;
			return B_OK;
		}
	}

	// search in the attribute directory
	Inode *attribute;
	status_t status = GetAttribute(name,&attribute);
	if (status == B_OK) {
		if (attribute->Lock().Lock() == B_OK) {
			status = attribute->ReadAt(pos,(uint8 *)buffer,_length);
			attribute->Lock().Unlock();
		} else
			status = B_ERROR;

		ReleaseAttribute(attribute);
	}

	RETURN_ERROR(status);
}


/**	Writes data to the specified attribute.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::WriteAttribute(Transaction *transaction,const char *name,int32 type,off_t pos,const uint8 *buffer,size_t *_length)
{
	// needed to maintain the index
	uint8 oldBuffer[BPLUSTREE_MAX_KEY_LENGTH],*oldData = NULL;
	size_t oldLength = 0;

	Index index(fVolume);
	bool hasIndex = index.SetTo(name) == B_OK;

	Inode *attribute = NULL;
	status_t status;
	if (GetAttribute(name,&attribute) < B_OK) {
		// save the old attribute data
		if (hasIndex) {
			fSmallDataLock.Lock();

			small_data *smallData = FindSmallData(name);
			if (smallData != NULL) {
				oldLength = smallData->data_size;
				if (oldLength > BPLUSTREE_MAX_KEY_LENGTH)
					oldLength = BPLUSTREE_MAX_KEY_LENGTH;
				memcpy(oldData = oldBuffer,smallData->Data(),oldLength);
			}
			fSmallDataLock.Unlock();
		}

		// if the attribute doesn't exist yet (as a file), try to put it in the
		// small_data section first - if that fails (due to insufficent space),
		// create a real attribute file
		status = AddSmallData(transaction,name,type,buffer,*_length);
		if (status == B_DEVICE_FULL) {
			status = CreateAttribute(transaction,name,type,&attribute);
			if (status < B_OK)
				RETURN_ERROR(status);
		} else if (status == B_OK)
			status = WriteBack(transaction);
	}

	if (attribute != NULL) {
		if (attribute->Lock().LockWrite() == B_OK) {
			// save the old attribute data (if this fails, oldLength will reflect it)
			if (hasIndex) {
				oldLength = BPLUSTREE_MAX_KEY_LENGTH;
				if (attribute->ReadAt(0,oldBuffer,&oldLength) == B_OK)
					oldData = oldBuffer;
			}
			status = attribute->WriteAt(transaction,pos,buffer,_length);
	
			attribute->Lock().UnlockWrite();
		} else
			status = B_ERROR;

		ReleaseAttribute(attribute);
	}

	if (status == B_OK) {
		// ToDo: find a better way for that "pos" thing...
		// Update index
		if (hasIndex && pos == 0) {
			// index only the first BPLUSTREE_MAX_KEY_LENGTH bytes
			uint16 length = *_length;
			if (length > BPLUSTREE_MAX_KEY_LENGTH)
				length = BPLUSTREE_MAX_KEY_LENGTH;

			index.Update(transaction,name,0,oldData,oldLength,buffer,length,this);
		}
	}
	return status;
}


/**	Removes the specified attribute from the inode.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::RemoveAttribute(Transaction *transaction,const char *name)
{
	Index index(fVolume);
	bool hasIndex = index.SetTo(name) == B_OK;

	// update index for attributes in the small_data section
	if (hasIndex) {
		fSmallDataLock.Lock();

		small_data *smallData = FindSmallData(name);
		if (smallData != NULL) {
			uint32 length = smallData->data_size;
			if (length > BPLUSTREE_MAX_KEY_LENGTH)
				length = BPLUSTREE_MAX_KEY_LENGTH;
			index.Update(transaction,name,0,smallData->Data(),length,NULL,0,this);
		}
		fSmallDataLock.Unlock();
	}

	status_t status = RemoveSmallData(transaction,name);
	if (status == B_OK) {
		status = WriteBack(transaction);
	} else if (status == B_ENTRY_NOT_FOUND && !Attributes().IsZero()) {
		// remove the attribute file if it exists
		Vnode vnode(fVolume,Attributes());
		Inode *attributes;
		if ((status = vnode.Get(&attributes)) < B_OK)
			return status;

		// update index
		Inode *attribute;
		if (hasIndex && GetAttribute(name,&attribute) == B_OK) {
			uint8 data[BPLUSTREE_MAX_KEY_LENGTH];
			size_t length = BPLUSTREE_MAX_KEY_LENGTH;
			if (attribute->ReadAt(0,data,&length) == B_OK)
				index.Update(transaction,name,0,data,length,NULL,0,this);

			ReleaseAttribute(attribute);
		}

		if ((status = attributes->Remove(transaction,name)) < B_OK)
			return status;

		if (attributes->IsEmpty()) {
			// remove attribute directory (don't fail if that can't be done)
			if (remove_vnode(fVolume->ID(),attributes->ID()) == B_OK) {
				// update the inode, so that no one will ever doubt it's deleted :-)
				attributes->Node()->flags |= INODE_DELETED;
				if (attributes->WriteBack(transaction) == B_OK) {
					Attributes().SetTo(0,0,0);
					WriteBack(transaction);
				} else
					unremove_vnode(fVolume->ID(),attributes->ID());
			}
		}
	}
	return status;
}


status_t
Inode::GetAttribute(const char *name,Inode **attribute)
{
	// does this inode even have attributes?
	if (Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Vnode vnode(fVolume,Attributes());
	Inode *attributes;
	if (vnode.Get(&attributes) < B_OK) {
		FATAL(("get_vnode() failed in Inode::GetAttribute(name = \"%s\")\n",name));
		return B_ERROR;
	}

	BPlusTree *tree;
	status_t status = attributes->GetTree(&tree);
	if (status == B_OK) {
		vnode_id id;
		if ((status = tree->Find((uint8 *)name,(uint16)strlen(name),&id)) == B_OK)
			return get_vnode(fVolume->ID(),id,(void **)attribute);
	}
	return status;
}


void
Inode::ReleaseAttribute(Inode *attribute)
{
	if (attribute == NULL)
		return;

	put_vnode(fVolume->ID(),attribute->ID());
}


status_t
Inode::CreateAttribute(Transaction *transaction,const char *name,uint32 type,Inode **attribute)
{
	// do we need to create the attribute directory first?
	if (Attributes().IsZero()) {
		status_t status = Inode::Create(transaction,this,NULL,S_ATTR_DIR | 0666,0,0,NULL);
		if (status < B_OK)
			RETURN_ERROR(status);
	}
	Vnode vnode(fVolume,Attributes());
	Inode *attributes;
	if (vnode.Get(&attributes) < B_OK)
		return B_ERROR;

	// Inode::Create() locks the inode if we provide the "id" parameter
	vnode_id id;
	return Inode::Create(transaction,attributes,name,S_ATTR | 0666,0,type,&id,attribute);
}


//	#pragma mark -


/**	Gives the caller direct access to the b+tree for a given directory.
 *	The tree is created on demand, but lasts until the inode is
 *	deleted.
 */

status_t
Inode::GetTree(BPlusTree **tree)
{
	if (fTree) {
		*tree = fTree;
		return B_OK;
	}

	if (IsDirectory()) {
		fTree = new BPlusTree(this);
		if (!fTree)
			RETURN_ERROR(B_NO_MEMORY);

		*tree = fTree;
		status_t status = fTree->InitCheck();
		if (status < B_OK) {
			delete fTree;
			fTree = NULL;
		}
		RETURN_ERROR(status);
	}
	RETURN_ERROR(B_BAD_VALUE);
}


bool 
Inode::IsEmpty()
{
	BPlusTree *tree;
	status_t status = GetTree(&tree);
	if (status < B_OK)
		return status;

	TreeIterator iterator(tree);

	// index and attribute directories are really empty when they are
	// empty - directories for standard files always contain ".", and
	// "..", so we need to ignore those two

	uint32 count = 0;
	char name[BPLUSTREE_MAX_KEY_LENGTH];
	uint16 length;
	vnode_id id;
	while (iterator.GetNextEntry(name,&length,B_FILE_NAME_LENGTH,&id) == B_OK) {
		if (Mode() & (S_ATTR_DIR | S_INDEX_DIR))
			return false;

		if (++count > 2 || strcmp(".",name) && strcmp("..",name))
			return false;
	}
	return true;
}


/** Finds the block_run where "pos" is located in the data_stream of
 *	the inode.
 *	If successful, "offset" will then be set to the file offset
 *	of the block_run returned; so "pos - offset" is for the block_run
 *	what "pos" is for the whole stream.
 */

status_t
Inode::FindBlockRun(off_t pos,block_run &run,off_t &offset)
{
	data_stream *data = &Node()->data;

	// Inode::ReadAt() does already does this
	//if (pos > data->size)
	//	return B_ENTRY_NOT_FOUND;

	// find matching block run

	if (data->max_direct_range > 0 && pos >= data->max_direct_range) {
		if (data->max_double_indirect_range > 0 && pos >= data->max_indirect_range) {
			// access to double indirect blocks

			CachedBlock cached(fVolume);

			off_t start = pos - data->max_indirect_range;
			int32 indirectSize = (16 << fVolume->BlockShift()) * (fVolume->BlockSize() / sizeof(block_run));
			int32 directSize = 4 << fVolume->BlockShift();
			int32 index = start / indirectSize;
			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);

			block_run *indirect = (block_run *)cached.SetTo(
					fVolume->ToBlock(data->double_indirect) + index / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			//printf("\tstart = %Ld, indirectSize = %ld, directSize = %ld, index = %ld\n",start,indirectSize,directSize,index);
			//printf("\tlook for indirect block at %ld,%d\n",indirect[index].allocation_group,indirect[index].start);

			int32 current = (start % indirectSize) / directSize;

			indirect = (block_run *)cached.SetTo(
					fVolume->ToBlock(indirect[index % runsPerBlock]) + current / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			run = indirect[current % runsPerBlock];
			offset = data->max_indirect_range + (index * indirectSize) + (current * directSize);
			//printf("\tfCurrent = %ld, fRunFileOffset = %Ld, fRunBlockEnd = %Ld, fRun = %ld,%d\n",fCurrent,fRunFileOffset,fRunBlockEnd,fRun.allocation_group,fRun.start);
		} else {
			// access to indirect blocks

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			off_t runBlockEnd = data->max_direct_range;

			CachedBlock cached(fVolume);
			off_t block = fVolume->ToBlock(data->indirect);

			for (int32 i = 0;i < data->indirect.length;i++) {
				block_run *indirect = (block_run *)cached.SetTo(block + i);
				if (indirect == NULL)
					RETURN_ERROR(B_IO_ERROR);

				int32 current = -1;
				while (++current < runsPerBlock) {
					if (indirect[current].IsZero())
						break;

					runBlockEnd += indirect[current].length << fVolume->BlockShift();
					if (runBlockEnd > pos) {
						run = indirect[current];
						offset = runBlockEnd - (run.length << fVolume->BlockShift());
						//printf("reading from indirect block: %ld,%d\n",fRun.allocation_group,fRun.start);
						//printf("### indirect-run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
						return fVolume->IsValidBlockRun(run);
					}
				}
			}
			RETURN_ERROR(B_ERROR);
		}
	} else {
		// access from direct blocks

		off_t runBlockEnd = 0LL;
		int32 current = -1;

		while (++current < NUM_DIRECT_BLOCKS) {
			if (data->direct[current].IsZero())
				break;

			runBlockEnd += data->direct[current].length << fVolume->BlockShift();
			if (runBlockEnd > pos) {
				run = data->direct[current];
				offset = runBlockEnd - (run.length << fVolume->BlockShift());
				//printf("### run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
				return fVolume->IsValidBlockRun(run);
			}
		}
		//PRINT(("FindBlockRun() failed in direct range: size = %Ld, pos = %Ld\n",data->size,pos));
		return B_ENTRY_NOT_FOUND;
	}
	return fVolume->IsValidBlockRun(run);
}


status_t
Inode::ReadAt(off_t pos, uint8 *buffer, size_t *_length)
{
	// set/check boundaries for pos/length

	if (pos < 0)
		pos = 0;
	else if (pos >= Node()->data.size) {
		*_length = 0;
		return B_NO_ERROR;
	}

	size_t length = *_length;

	if (pos + length > Node()->data.size)
		length = Node()->data.size - pos;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos,run,offset) < B_OK) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	uint32 bytesRead = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we read could not be aligned to the block_size boundary
	// (read partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start += (pos - offset) / blockSize;
		run.length -= (pos - offset) / blockSize;

		CachedBlock cached(fVolume,run);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			RETURN_ERROR(B_BAD_VALUE);
		}

		bytesRead = blockSize - (pos % blockSize);
		if (length < bytesRead)
			bytesRead = length;

		memcpy(buffer,block + (pos % blockSize),bytesRead);
		pos += bytesRead;

		length -= bytesRead;
		if (length == 0) {
			*_length = bytesRead;
			return B_OK;
		}

		if (FindBlockRun(pos,run,offset) < B_OK) {
			*_length = bytesRead;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	// the first block_run is already filled in at this point
	// read the following complete blocks using cached_read(),
	// the last partial block is read using the CachedBlock class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start += (pos - offset) >> blockShift;
		run.length -= (pos - offset) >> blockShift;

		if ((run.length << blockShift) > length) {
			if (length < blockSize) {
				CachedBlock cached(fVolume,run);
				if ((block = cached.Block()) == NULL) {
					*_length = bytesRead;
					RETURN_ERROR(B_BAD_VALUE);
				}
				memcpy(buffer + bytesRead,block,length);
				bytesRead += length;
				break;
			}
			run.length = length >> blockShift;
			partial = true;
		}

		if (cached_read(fVolume->Device(),fVolume->ToBlock(run),buffer + bytesRead,
						run.length,blockSize) != B_OK) {
			*_length = bytesRead;
			RETURN_ERROR(B_BAD_VALUE);
		}

		int32 bytes = run.length << blockShift;
		length -= bytes;
		bytesRead += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (partial) {
			// if the last block was read only partially, point block_run
			// to the remaining part
			run.start += run.length;
			run.length = 1;
			offset = pos;
		} else if (FindBlockRun(pos,run,offset) < B_OK) {
			*_length = bytesRead;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	*_length = bytesRead;
	return B_NO_ERROR;
}


status_t 
Inode::WriteAt(Transaction *transaction,off_t pos,const uint8 *buffer,size_t *_length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0)
		pos = 0;
	else if (pos + length > Node()->data.size) {
		off_t oldSize = Size();

		// the transaction doesn't have to be started already
		if ((Flags() & INODE_NO_TRANSACTION) == 0)
			transaction->Start(fVolume,BlockNumber());

		// let's grow the data stream to the size needed
		status_t status = SetFileSize(transaction,pos + length);
		if (status < B_OK) {
			*_length = 0;
			RETURN_ERROR(status);
		}
		// If the position of the write was beyond the file size, we
		// have to fill the gap between that position and the old file
		// size with zeros.
		FillGapWithZeros(oldSize,pos);
	}

	block_run run;
	off_t offset;
	if (FindBlockRun(pos,run,offset) < B_OK) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	bool logStream = (Flags() & INODE_LOGGED) == INODE_LOGGED;
	if (logStream)
		transaction->Start(fVolume,BlockNumber());

	uint32 bytesWritten = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we write could not be aligned to the block_size boundary
	// (write partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start += (pos - offset) / blockSize;
		run.length -= (pos - offset) / blockSize;

		CachedBlock cached(fVolume,run);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			RETURN_ERROR(B_BAD_VALUE);
		}

		bytesWritten = blockSize - (pos % blockSize);
		if (length < bytesWritten)
			bytesWritten = length;

		memcpy(block + (pos % blockSize),buffer,bytesWritten);

		// either log the stream or write it directly to disk
		if (logStream)
			cached.WriteBack(transaction);
		else
			fVolume->WriteBlocks(cached.BlockNumber(),block,1);

		pos += bytesWritten;
		
		length -= bytesWritten;
		if (length == 0) {
			*_length = bytesWritten;
			return B_OK;
		}

		if (FindBlockRun(pos,run,offset) < B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}
	
	// the first block_run is already filled in at this point
	// write the following complete blocks using Volume::WriteBlocks(),
	// the last partial block is written using the CachedBlock class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start += (pos - offset) >> blockShift;
		run.length -= (pos - offset) >> blockShift;

		if ((run.length << blockShift) > length) {
			if (length < blockSize) {
				CachedBlock cached(fVolume,run);
				if ((block = cached.Block()) == NULL) {
					*_length = bytesWritten;
					RETURN_ERROR(B_BAD_VALUE);
				}
				memcpy(block,buffer + bytesWritten,length);

				if (logStream)
					cached.WriteBack(transaction);
				else
					fVolume->WriteBlocks(cached.BlockNumber(),block,1);

				bytesWritten += length;
				break;
			}
			run.length = length >> blockShift;
			partial = true;
		}

		status_t status;
		if (logStream) {
			status = transaction->WriteBlocks(fVolume->ToBlock(run),
						buffer + bytesWritten,run.length);
		} else {
			status = fVolume->WriteBlocks(fVolume->ToBlock(run),
						buffer + bytesWritten,run.length);
		}
		if (status != B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}

		int32 bytes = run.length << blockShift;
		length -= bytes;
		bytesWritten += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (partial) {
			// if the last block was written only partially, point block_run
			// to the remaining part
			run.start += run.length;
			run.length = 1;
			offset = pos;
		} else if (FindBlockRun(pos,run,offset) < B_OK) {
			*_length = bytesWritten;
			RETURN_ERROR(B_BAD_VALUE);
		}
	}

	*_length = bytesWritten;

	return B_NO_ERROR;
}


/**	Fills the gap between the old file size and the new file size
 *	with zeros.
 *	It's more or less a copy of Inode::WriteAt() but it can handle
 *	length differences of more than just 4 GB, and it never uses
 *	the log, even if the INODE_LOGGED flag is set.
 */

status_t
Inode::FillGapWithZeros(off_t pos,off_t newSize)
{
	//if (pos >= newSize)
		return B_OK;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos,run,offset) < B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	off_t length = newSize - pos;
	uint32 bytesWritten = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we write could not be aligned to the block_size boundary
	// (write partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start += (pos - offset) / blockSize;
		run.length -= (pos - offset) / blockSize;

		CachedBlock cached(fVolume,run);
		if ((block = cached.Block()) == NULL)
			RETURN_ERROR(B_BAD_VALUE);

		bytesWritten = blockSize - (pos % blockSize);
		if (length < bytesWritten)
			bytesWritten = length;

		memset(block + (pos % blockSize),0,bytesWritten);
		fVolume->WriteBlocks(cached.BlockNumber(),block,1);

		pos += bytesWritten;
		
		length -= bytesWritten;
		if (length == 0)
			return B_OK;

		if (FindBlockRun(pos,run,offset) < B_OK)
			RETURN_ERROR(B_BAD_VALUE);
	}

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start += (pos - offset) >> blockShift;
		run.length -= (pos - offset) >> blockShift;

		CachedBlock cached(fVolume);
		off_t blockNumber = fVolume->ToBlock(run);
		for (int32 i = 0;i < run.length;i++) {
			if ((block = cached.SetTo(blockNumber + i,true)) == NULL)
				RETURN_ERROR(B_IO_ERROR);

			if (fVolume->WriteBlocks(cached.BlockNumber(),block,1) < B_OK)
				RETURN_ERROR(B_IO_ERROR);
		}

		int32 bytes = run.length << blockShift;
		length -= bytes;
		bytesWritten += bytes;
		
		// since we don't respect a last partial block, length can be lower
		if (length <= 0)
			break;

		pos += bytes;

		if (FindBlockRun(pos,run,offset) < B_OK)
			RETURN_ERROR(B_BAD_VALUE);
	}
	return B_OK;
}


status_t 
Inode::GrowStream(Transaction *transaction, off_t size)
{
	data_stream *data = &Node()->data;

	// is the data stream already large enough to hold the new size?
	// (can be the case with preallocated blocks)
	if (size < data->max_direct_range
		|| size < data->max_indirect_range
		|| size < data->max_double_indirect_range) {
		data->size = size;
		return B_OK;
	}

	// how many bytes are still needed? (unused ranges are always zero)
	off_t bytes;		
	if (data->size < data->max_double_indirect_range)
		bytes = size - data->max_double_indirect_range;
	else if (data->size < data->max_indirect_range)
		bytes = size - data->max_indirect_range;
	else if (data->size < data->max_direct_range)
		bytes = size - data->max_direct_range;
	else
		bytes = size - data->size;

	// do we have enough free blocks on the disk?
	off_t blocks = (bytes + fVolume->BlockSize() - 1) / fVolume->BlockSize();
	if (blocks > fVolume->FreeBlocks())
		return B_DEVICE_FULL;

	// should we preallocate some blocks (currently, always 64k)?
	off_t blocksNeeded = blocks;
	if (blocks < 65536 / fVolume->BlockSize() && fVolume->FreeBlocks() > 128)
		blocks = 65536 / fVolume->BlockSize();

	while (blocksNeeded > 0) {
		// the requested blocks do not need to be returned with a
		// single allocation, so we need to iterate until we have
		// enough blocks allocated
		block_run run;
		status_t status = fVolume->Allocate(transaction,this,blocks,run);
		if (status < B_OK)
			return status;

		// okay, we have the needed blocks, so just distribute them to the
		// different ranges of the stream (direct, indirect & double indirect)
		
		blocksNeeded -= run.length;
		// don't preallocate if the first allocation was already too small
		blocks = blocksNeeded;
		
		if (data->size <= data->max_direct_range) {
			// let's try to put them into the direct block range
			int32 free = 0;
			for (;free < NUM_DIRECT_BLOCKS;free++)
				if (data->direct[free].IsZero())
					break;

			if (free < NUM_DIRECT_BLOCKS) {
				// can we merge the last allocated run with the new one?
				int32 last = free - 1;
				if (free > 0
					&& data->direct[last].allocation_group == run.allocation_group
					&& data->direct[last].start + data->direct[last].length == run.start) {
					data->direct[last].length += run.length;
				} else {
					data->direct[free] = run;
				}
				data->max_direct_range += run.length * fVolume->BlockSize();
				data->size = blocksNeeded > 0 ? data->max_direct_range : size;
				continue;
			}
		}

		if (data->size <= data->max_indirect_range || !data->max_indirect_range) {
			CachedBlock cached(fVolume);
			block_run *runs = NULL;
			int32 free = 0;
			off_t block;

			// if there is no indirect block yet, create one
			if (data->indirect.IsZero()) {
				status = fVolume->Allocate(transaction,this,4,data->indirect,4);
				if (status < B_OK)
					return status;

				// make sure those blocks are empty
				block = fVolume->ToBlock(data->indirect);					
				for (int32 i = 1;i < data->indirect.length;i++) {
					block_run *runs = (block_run *)cached.SetTo(block + i,true);
					if (runs == NULL)
						return B_IO_ERROR;

					cached.WriteBack(transaction);
				}
				data->max_indirect_range = data->max_direct_range;
				// insert the block_run in the first block
				runs = (block_run *)cached.SetTo(block,true);
			} else {
				uint32 numberOfRuns = fVolume->BlockSize() / sizeof(block_run);
				block = fVolume->ToBlock(data->indirect);

				// search first empty entry
				int32 i = 0;
				for (;i < data->indirect.length;i++) {
					if ((runs = (block_run *)cached.SetTo(block + i)) == NULL)
						return B_IO_ERROR;

					for (free = 0;free < numberOfRuns;free++)
						if (runs[free].IsZero())
							break;

					if (free < numberOfRuns)
						break;
				}
				if (i == data->indirect.length)
					runs = NULL;
			}

			if (runs != NULL) {
				// try to insert the run to the last one - note that this doesn't
				// take block borders into account, so it could be further optimized
				int32 last = free - 1;
				if (free > 0
					&& runs[last].allocation_group == run.allocation_group
					&& runs[last].start + runs[last].length == run.start) {
					runs[last].length += run.length;
				} else {
					runs[free] = run;
				}
				data->max_indirect_range += run.length * fVolume->BlockSize();
				data->size = blocksNeeded > 0 ? data->max_indirect_range : size;

				cached.WriteBack(transaction);
				continue;
			}
		}

		// when we are here, we need to grow into the double indirect
		// range - but that's not yet implemented, so bail out!

		if (data->size <= data->max_double_indirect_range || !data->max_double_indirect_range) {
			FATAL(("growing in the double indirect range is not yet implemented!\n"));
			// ToDo: implement growing into the double indirect range, please!
		}

		RETURN_ERROR(EFBIG);
	}
	// update the size of the data stream
	data->size = size;

	return B_OK;
}


status_t
Inode::FreeStaticStreamArray(Transaction *transaction,int32 level,block_run run,off_t size,off_t offset,off_t &max)
{
	int32 indirectSize;
	if (level == 0)
		indirectSize = (16 << fVolume->BlockShift()) * (fVolume->BlockSize() / sizeof(block_run));
	else if (level == 1)
		indirectSize = 4 << fVolume->BlockShift();

	off_t start;
	if (size > offset)
		start = size - offset;
	else
		start = 0;

	int32 index = start / indirectSize;
	int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);

	CachedBlock cached(fVolume);
	off_t blockNumber = fVolume->ToBlock(run);

	// set the file offset to the current block run
	offset += (off_t)index * indirectSize;

	for (int32 i = index / runsPerBlock;i < run.length;i++) {
		block_run *array = (block_run *)cached.SetTo(blockNumber + i);
		if (array == NULL)
			RETURN_ERROR(B_ERROR);

		for (index = index % runsPerBlock;index < runsPerBlock;index++) {
			if (array[index].IsZero()) {
				// we also want to break out of the outer loop
				i = run.length;
				break;
			}

			status_t status = B_OK;
			if (level == 0)
				status = FreeStaticStreamArray(transaction,1,array[index],size,offset,max);
			else if (offset >= size)
				status = fVolume->Free(transaction,array[index]);
			else
				max = offset + indirectSize;

			if (status < B_OK)
				RETURN_ERROR(status);

			if (offset >= size)
				array[index].SetTo(0,0,0);

			offset += indirectSize;
		}
		index = 0;

		cached.WriteBack(transaction);
	}
	return B_OK;
}


/** Frees all block_runs in the array which come after the specified size.
 *	It also trims the last block_run that contain the size.
 *	"offset" and "max" are maintained until the last block_run that doesn't
 *	have to be freed - after this, the values won't be correct anymore, but
 *	will still assure correct function for all subsequent calls.
 */

status_t
Inode::FreeStreamArray(Transaction *transaction,block_run *array,uint32 arrayLength,off_t size,off_t &offset,off_t &max)
{
	off_t newOffset = offset;
	uint32 i = 0;
	for (;i < arrayLength;i++,offset = newOffset) {
		if (array[i].IsZero())
			break;

		newOffset += (off_t)array[i].length << fVolume->BlockShift();
		if (newOffset <= size)
			continue;

		block_run run = array[i];

		// determine the block_run to be freed
		if (newOffset > size && offset < size) {
			// free partial block_run (and update the original block_run)
			run.start = array[i].start + ((size - offset) >> fVolume->BlockShift()) + 1;
			array[i].length = run.start - array[i].start;
			run.length -= array[i].length;

			if (run.length == 0)
				continue;

			// update maximum range
			max = offset + ((off_t)array[i].length << fVolume->BlockShift());
		} else {
			// free the whole block_run
			array[i].SetTo(0,0,0);
			
			if (max > offset)
				max = offset;
		}

		if (fVolume->Free(transaction,run) < B_OK)
			return B_IO_ERROR;
	}
	return B_OK;
}


status_t 
Inode::ShrinkStream(Transaction *transaction, off_t size)
{
	data_stream *data = &Node()->data;

	if (data->max_double_indirect_range > size) {
		FreeStaticStreamArray(transaction,0,data->double_indirect,size,data->max_indirect_range,data->max_double_indirect_range);
		
		if (size <= data->max_indirect_range) {
			fVolume->Free(transaction,data->double_indirect);
			data->double_indirect.SetTo(0,0,0);
			data->max_double_indirect_range = 0;
		}
	}
	if (data->max_indirect_range > size) {
		CachedBlock cached(fVolume);
		off_t block = fVolume->ToBlock(data->indirect);
		off_t offset = data->max_direct_range;

		for (int32 i = 0;i < data->indirect.length;i++) {
			block_run *array = (block_run *)cached.SetTo(block + i);
			if (array == NULL)
				break;

			if (FreeStreamArray(transaction,array,fVolume->BlockSize() / sizeof(block_run),size,offset,data->max_indirect_range) == B_OK)
				cached.WriteBack(transaction);
		}
		if (data->max_direct_range == data->max_indirect_range) {
			fVolume->Free(transaction,data->indirect);
			data->indirect.SetTo(0,0,0);
			data->max_indirect_range = 0;
		}
	}
	if (data->max_direct_range > size) {
		off_t offset = 0;
		FreeStreamArray(transaction,data->direct,NUM_DIRECT_BLOCKS,size,offset,data->max_direct_range);
	}

	data->size = size;
	return B_OK;
}


status_t 
Inode::SetFileSize(Transaction *transaction, off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;

	off_t oldSize = Node()->data.size;

	if (size == oldSize)
		return B_OK;

	// should the data stream grow or shrink?
	status_t status;
	if (size > oldSize) {
		status = GrowStream(transaction,size);
		if (status < B_OK) {
			// if the growing of the stream fails, the whole operation
			// fails, so we should shrink the stream to its former size
			ShrinkStream(transaction,oldSize);
		}
	}
	else
		status = ShrinkStream(transaction,size);

	if (status < B_OK)
		return status;

	return WriteBack(transaction);
}


status_t 
Inode::Append(Transaction *transaction,off_t bytes)
{
	return SetFileSize(transaction,Size() + bytes);
}


status_t 
Inode::Trim(Transaction *transaction)
{
	return ShrinkStream(transaction,Size());
}


status_t 
Inode::Sync()
{
	// We may also want to flush the attribute's data stream to
	// disk here... (do we?)

	data_stream *data = &Node()->data;
	status_t status;

	// flush direct range

	for (int32 i = 0;i < NUM_DIRECT_BLOCKS;i++) {
		if (data->direct[i].IsZero())
			return B_OK;
		
		status = flush_blocks(fVolume->Device(),fVolume->ToBlock(data->direct[i]),data->direct[i].length);
		if (status != B_OK)
			return status;
	}

	// flush indirect range
	
	if (data->max_indirect_range == 0)
		return B_OK;

	CachedBlock cached(fVolume);
	off_t block = fVolume->ToBlock(data->indirect);
	int32 count = fVolume->BlockSize() / sizeof(block_run);

	for (int32 j = 0;j < data->indirect.length;j++) {
		block_run *runs = (block_run *)cached.SetTo(block + j);
		if (runs == NULL)
			break;

		for (int32 i = 0;i < count;i++) {
			if (runs[i].IsZero())
				return B_OK;

			status = flush_blocks(fVolume->Device(),fVolume->ToBlock(runs[i]),runs[i].length);
			if (status != B_OK)
				return status;
		}
	}

	// flush double indirect range
	
	if (data->max_double_indirect_range == 0)
		return B_OK;

	off_t indirectBlock = fVolume->ToBlock(data->double_indirect);
	
	for (int32 l = 0;l < data->double_indirect.length;l++) {
		block_run *indirectRuns = (block_run *)cached.SetTo(indirectBlock + l);
		if (indirectRuns == NULL)
			return B_FILE_ERROR;

		CachedBlock directCached(fVolume);

		for (int32 k = 0;k < count;k++) {
			if (indirectRuns[k].IsZero())
				return B_OK;

			block = fVolume->ToBlock(indirectRuns[k]);			
			for (int32 j = 0;j < indirectRuns[k].length;j++) {
				block_run *runs = (block_run *)directCached.SetTo(block + j);
				if (runs == NULL)
					return B_FILE_ERROR;
				
				for (int32 i = 0;i < count;i++) {
					if (runs[i].IsZero())
						return B_OK;

					// ToDo: combine single block_runs to bigger ones when
					// they are adjacent
					status = flush_blocks(fVolume->Device(),fVolume->ToBlock(runs[i]),runs[i].length);
					if (status != B_OK)
						return status;
				}
			}
		}
	}
	return B_OK;
}


status_t
Inode::Remove(Transaction *transaction,const char *name,off_t *_id,bool isDirectory)
{
	BPlusTree *tree;
	if (GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	// does the file even exists?
	off_t id;
	if (tree->Find((uint8 *)name,(uint16)strlen(name),&id) < B_OK)
		return B_ENTRY_NOT_FOUND;

	if (_id)
		*_id = id;

	Vnode vnode(fVolume,id);
	Inode *inode;
	status_t status = vnode.Get(&inode);
	if (status < B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}

	// It's a bit stupid, but indices are regarded as directories
	// in BFS - so a test for a directory always succeeds, but you
	// should really be able to do whatever you want with your indices
	// without having to remove all files first :)
	if (!inode->IsIndex()) {
		// if it's not of the correct type, don't delete it!
		if (inode->IsDirectory() != isDirectory)
			return isDirectory ? B_NOT_A_DIRECTORY : B_IS_A_DIRECTORY;

		// only delete empty directories
		if (isDirectory && !inode->IsEmpty())
			return B_DIRECTORY_NOT_EMPTY;
	}

	// remove_vnode() allows the inode to be accessed until the last put_vnode()
	if (remove_vnode(fVolume->ID(),id) != B_OK)
		return B_ERROR;

	if (tree->Remove(transaction,(uint8 *)name,(uint16)strlen(name),id) < B_OK) {
		unremove_vnode(fVolume->ID(),id);
		RETURN_ERROR(B_ERROR);
	}

	// update the inode, so that no one will ever doubt it's deleted :-)
	inode->Node()->flags |= INODE_DELETED;

	// In balance to the Inode::Create() method, the main indices
	// are updated here (name, size, & last_modified)

	Index index(fVolume);
	if ((inode->Mode() & (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)) == 0) {
		index.RemoveName(transaction,name,inode);
			// If removing from the index fails, it is not regarded as a
			// fatal error and will not be reported back!
			// Deleted inodes won't be visible in queries anyway.
	}
	
	if ((inode->Mode() & (S_FILE | S_SYMLINK)) != 0) {
		index.RemoveSize(transaction,inode);
		index.RemoveLastModified(transaction,inode);
	}

	if (inode->WriteBack(transaction) < B_OK)
		return B_ERROR;

	return B_OK;
}


/**	Creates the inode with the specified parent directory, and automatically
 *	adds the created inode to that parent directory. If an attribute directory
 *	is created, it will also automatically added to the parent inode as such.
 *	However, the indices root node, and the regular root node won't be added
 *	to the super block.
 *	It will also create the initial B+tree for the inode if it's a directory
 *	of any kind.
 *	If the "id" variable is given to store the inode's ID, the inode stays
 *	locked - you have to call put_vnode() if you don't use it anymore.
 */

status_t 
Inode::Create(Transaction *transaction,Inode *parent, const char *name, int32 mode, int omode, uint32 type, off_t *_id, Inode **_inode)
{
	block_run parentRun = parent ? parent->BlockRun() : block_run::Run(0,0,0);
	Volume *volume = transaction->GetVolume();
	BPlusTree *tree = NULL;

	if (parent && (mode & S_ATTR_DIR) == 0 && parent->IsDirectory()) {
		// check if the file already exists in the directory
		if (parent->GetTree(&tree) != B_OK)
			RETURN_ERROR(B_BAD_VALUE);

		// does the file already exist?
		off_t offset;
		if (tree->Find((uint8 *)name,(uint16)strlen(name),&offset) == B_OK) {
			// return if the file should be a directory or opened in exclusive mode
			if (mode & S_DIRECTORY || omode & O_EXCL)
				return B_FILE_EXISTS;

			Vnode vnode(volume,offset);
			Inode *inode;
			status_t status = vnode.Get(&inode);
			if (status < B_OK) {
				REPORT_ERROR(status);
				return B_ENTRY_NOT_FOUND;
			}

			// if it's a directory, bail out!
			if (inode->IsDirectory())
				return B_IS_A_DIRECTORY;

			// if omode & O_TRUNC, truncate the existing file
			if (omode & O_TRUNC) {
				WriteLocked locked(inode->Lock());

				status_t status = inode->SetFileSize(transaction,0);
				if (status < B_OK)
					return status;
			}

			// only keep the vnode in memory if the vnode_id pointer is provided
			if (_id) {
				*_id = offset;
				vnode.Keep();
			}
			if (_inode)
				*_inode = inode;

			return B_OK;
		}
	} else if (parent && (mode & S_ATTR_DIR) == 0)
		return B_BAD_VALUE;

	// allocate space for the new inode
	InodeAllocator allocator(transaction);
	block_run run;
	Inode *inode;
	status_t status = allocator.New(&parentRun,mode,run,&inode);
	if (status < B_OK)
		return status;

	// initialize the on-disk bfs_inode structure 

	bfs_inode *node = inode->Node();

	node->magic1 = INODE_MAGIC1;
	node->inode_num = run;
	node->parent = parentRun;

	node->uid = geteuid();
	node->gid = parent ? parent->Node()->gid : getegid();
		// the group ID is inherited from the parent, if available
	node->mode = mode;
	node->flags = INODE_IN_USE;
	node->type = type;

	node->create_time = (bigtime_t)time(NULL) << INODE_TIME_SHIFT;
	node->last_modified_time = node->create_time | (volume->GetUniqueID() & INODE_TIME_MASK);
		// we use Volume::GetUniqueID() to avoid having too many duplicates in the
		// last_modified index

	node->inode_size = volume->InodeSize();

	// only add the name to regular files, directories, or symlinks
	// don't add it to attributes, or indices
	if (tree && (mode & (S_INDEX_DIR | S_ATTR_DIR | S_ATTR)) == 0
		&& inode->SetName(transaction,name) < B_OK)
		return B_ERROR;

	// initialize b+tree if it's a directory (and add "." & ".." if it's
	// a standard directory for files - not for attributes or indices)
	if (mode & (S_DIRECTORY | S_ATTR_DIR | S_INDEX_DIR)) {
		BPlusTree *tree = inode->fTree = new BPlusTree(transaction,inode);
		if (tree == NULL || tree->InitCheck() < B_OK)
			return B_ERROR;

		if ((mode & (S_INDEX_DIR | S_ATTR_DIR)) == 0) {
			if (tree->Insert(transaction,".",inode->BlockNumber()) < B_OK
				|| tree->Insert(transaction,"..",volume->ToBlock(inode->Parent())) < B_OK)
				return B_ERROR;
		}
	}

	// update the main indices (name, size & last_modified)
	Index index(volume);
	if ((mode & (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)) == 0) {
		status = index.InsertName(transaction,name,inode);
		if (status < B_OK && status != B_BAD_INDEX)
			return status;
	}

	inode->UpdateOldLastModified();

	// The "size" & "last_modified" indices don't contain directories
	if ((mode & (S_FILE | S_SYMLINK)) != 0) {
		// if adding to these indices fails, the inode creation will not be harmed
		index.InsertSize(transaction,inode);
		index.InsertLastModified(transaction,inode);
	}

	if ((status = inode->WriteBack(transaction)) < B_OK)
		return status;

	if (new_vnode(volume->ID(),inode->ID(),inode) != B_OK)
		return B_ERROR;

	// add a link to the inode from the parent, depending on its type
	if (tree && tree->Insert(transaction,name,volume->ToBlock(run)) < B_OK) {
		put_vnode(volume->ID(),inode->ID());
		RETURN_ERROR(B_ERROR);
	} else if (parent && mode & S_ATTR_DIR) {
		parent->Attributes() = run;
		parent->WriteBack(transaction);
	}

	allocator.Keep();

	if (_id != NULL)
		*_id = inode->ID();
	else
		put_vnode(volume->ID(),inode->ID());

	if (_inode != NULL)
		*_inode = inode;

	return B_OK;
}


//	#pragma mark -


AttributeIterator::AttributeIterator(Inode *inode)
	:
	fCurrentSmallData(0),
	fInode(inode),
	fAttributes(NULL),
	fIterator(NULL),
	fBuffer(NULL)
{
	inode->AddIterator(this);
}


AttributeIterator::~AttributeIterator()
{
	if (fAttributes)
		put_vnode(fAttributes->GetVolume()->ID(),fAttributes->ID());

	delete fIterator;
	fInode->RemoveIterator(this);
}


status_t 
AttributeIterator::Rewind()
{
	fCurrentSmallData = 0;

	if (fIterator != NULL)
		fIterator->Rewind();

	return B_OK;
}


status_t 
AttributeIterator::GetNext(char *name, size_t *_length, uint32 *_type, vnode_id *_id)
{
	// read attributes out of the small data section

	if (fCurrentSmallData >= 0) {
		small_data *item = fInode->Node()->small_data_start;

		fInode->SmallDataLock().Lock();

		int32 i = 0;
		for (;;item = item->Next()) {
			if (item->IsLast(fInode->Node()))
				break;

			if (item->name_size == FILE_NAME_NAME_LENGTH
				&& *item->Name() == FILE_NAME_NAME)
				continue;

			if (i++ == fCurrentSmallData)
				break;
		}

		if (!item->IsLast(fInode->Node())) {
			strncpy(name,item->Name(),B_FILE_NAME_LENGTH);
			*_type = item->type;
			*_length = item->name_size;
			*_id = (vnode_id)fCurrentSmallData;

			fCurrentSmallData = i;
		}
		else {
			// stop traversing the small_data section
			fCurrentSmallData = -1;
		}

		fInode->SmallDataLock().Unlock();

		if (fCurrentSmallData != -1)
			return B_OK;
	}

	// read attributes out of the attribute directory

	if (fInode->Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Volume *volume = fInode->GetVolume();

	// if you haven't yet access to the attributes directory, get it
	if (fAttributes == NULL) {
		if (get_vnode(volume->ID(),volume->ToVnode(fInode->Attributes()),(void **)&fAttributes) != 0
			|| fAttributes == NULL) {
			FATAL(("get_vnode() failed in AttributeIterator::GetNext(vnode_id = %Ld,name = \"%s\")\n",fInode->ID(),name));
			return B_ENTRY_NOT_FOUND;
		}

		BPlusTree *tree;
		if (fAttributes->GetTree(&tree) < B_OK
			|| (fIterator = new TreeIterator(tree)) == NULL) {
			FATAL(("could not get tree in AttributeIterator::GetNext(vnode_id = %Ld,name = \"%s\")\n",fInode->ID(),name));
			return B_ENTRY_NOT_FOUND;
		}
	}

	block_run run;
	uint16 length;
	vnode_id id;
	status_t status = fIterator->GetNextEntry(name,&length,B_FILE_NAME_LENGTH,&id);
	if (status < B_OK)
		return status;

	Vnode vnode(volume,id);
	Inode *attribute;
	if ((status = vnode.Get(&attribute)) == B_OK) {
		*_type = attribute->Node()->type;
		*_length = attribute->Node()->data.size;
		*_id = id;
	}

	return status;
}


void 
AttributeIterator::Update(uint16 index, int8 change)
{
	// fCurrentSmallData points already to the next item
	if (index < fCurrentSmallData)
		fCurrentSmallData += change;
}

