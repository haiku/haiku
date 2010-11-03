/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <new>

#include <fs_interface.h>
#include <io_requests.h>
#include <NodeMonitor.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include <debug.h>
#include <util/AutoLock.h>

#include "checksumfs.h"
#include "checksumfs_private.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "File.h"
#include "Notifications.h"
#include "SuperBlock.h"
#include "SymLink.h"
#include "Transaction.h"
#include "Volume.h"


static const char* const kCheckSumFSModuleName	= "file_systems/checksumfs"
	B_CURRENT_FS_API_VERSION;
static const char* const kCheckSumFSShortName	= "checksumfs";

static const bigtime_t kModifiedInterimUpdateInterval = 500000;
	// wait at least 0.5s between interim modified updates


// #pragma mark -


struct FileCookie {
	mutex		lock;
	int			openMode;
	bigtime_t	lastModifiedUpdate;
	bool		modifiedNeedsUpdate;
	bool		sizeChangedSinceUpdate;
	bool		modifiedNeedsFinalUpdate;
	bool		finalSizeChanged;


	FileCookie(int openMode)
		:
		openMode(openMode),
		lastModifiedUpdate(0),
		modifiedNeedsUpdate(false),
		sizeChangedSinceUpdate(false),
		modifiedNeedsFinalUpdate(false),
		finalSizeChanged(false)
	{
		mutex_init(&lock, "checksumfs file cookie");
	}

	~FileCookie()
	{
		mutex_destroy(&lock);
	}

	status_t UpdateModifiedIfNecessary(Node* node, bool finalUpdate)
	{
		MutexLocker locker(lock);

		return _UpdateModifiedIfNecessary(node, finalUpdate);
	}

	status_t FileModified(Node* node, bool sizeChanged)
	{
		MutexLocker locker(lock);

		modifiedNeedsUpdate = true;
		modifiedNeedsFinalUpdate = true;
		sizeChangedSinceUpdate |= sizeChanged;
		finalSizeChanged |= sizeChanged;

		return _UpdateModifiedIfNecessary(node, false);
	}

private:
	status_t _UpdateModifiedIfNecessary(Node* node, bool finalUpdate)
	{
		uint32 statFlags = B_STAT_MODIFICATION_TIME | B_STAT_CHANGE_TIME;

		if (finalUpdate) {
			if (!modifiedNeedsFinalUpdate)
				return B_OK;
			if (finalSizeChanged)
				statFlags |= B_STAT_SIZE;
		} else {
			if (!modifiedNeedsUpdate)
				return B_OK;

			if (system_time()
					< lastModifiedUpdate + kModifiedInterimUpdateInterval) {
				// not enough time passed -- postpone update
   				return B_OK;
			}

			statFlags |= B_STAT_INTERIM_UPDATE
				| (sizeChangedSinceUpdate ? B_STAT_SIZE : 0);
		}

		// do the update -- start a transaction, lock the node, and update
		Transaction transaction(node->GetVolume());
		status_t error = transaction.StartAndAddNode(node);
		if (error != B_OK)
			return error;

		node->Touched(NODE_MODIFIED);

		error = transaction.Commit(StatChangedNotification(node, statFlags));
		if (error != B_OK)
			return error;

		modifiedNeedsUpdate = false;
		lastModifiedUpdate = system_time();

		return B_OK;
	}
};


struct DirCookie {
	DirCookie(Directory* directory)
		:
		fDirectory(directory)
	{
		Rewind();
	}

	Directory* GetDirectory() const
	{
		return fDirectory;
	}

	void SetTo(Directory* directory, bool skipArtificialEntries)
	{
		fDirectory = directory;
		Rewind(skipArtificialEntries);
	}

	status_t ReadNextEntry(struct dirent* buffer, size_t size,
		uint32& _countRead)
	{
		const char* name;
		size_t nameLength;
		uint64 blockIndex;

		int nextIterationState = OTHERS;
		switch (fIterationState) {
			case DOT:
				name = ".";
				nameLength = 1;
				blockIndex = fDirectory->BlockIndex();
				nextIterationState = DOT_DOT;
				break;
			case DOT_DOT:
				name = "..";
				nameLength = 2;
				blockIndex = fDirectory->ParentDirectory();
				break;
			default:
			{
				status_t error = fDirectory->LookupNextEntry(fEntryName,
					fEntryName, nameLength, blockIndex);
				if (error != B_OK) {
					if (error != B_ENTRY_NOT_FOUND)
						return error;

					_countRead = 0;
					return B_OK;
				}

				name = fEntryName;
				break;
			}
		}

		size_t entrySize = sizeof(dirent) + nameLength;
		if (entrySize > size)
			return B_BUFFER_OVERFLOW;

		buffer->d_dev = fDirectory->GetVolume()->ID();
		buffer->d_ino = blockIndex;
		buffer->d_reclen = entrySize;
		strcpy(buffer->d_name, name);

		fIterationState = nextIterationState;

		_countRead = 1;
		return B_OK;
	}

	void Rewind(bool skipArtificialEntries = false)
	{
		fIterationState = skipArtificialEntries ? OTHERS : DOT;
		fEntryName[0] = '\0';
	}

private:
	enum {
		DOT,
		DOT_DOT,
		OTHERS
	};

	Directory*	fDirectory;
	int			fIterationState;
	char		fEntryName[kCheckSumFSNameLength + 1];
};


struct AttrDirCookie {
	AttrDirCookie(Node* node)
		:
		fNode(node),
		fAttributeDirectory(NULL),
		fDirCookie(NULL)
	{
	}

	~AttrDirCookie()
	{
		if (fAttributeDirectory != NULL)
			fNode->GetVolume()->PutNode(fAttributeDirectory);
	}

	status_t ReadNextEntry(struct dirent* buffer, size_t size,
		uint32& _countRead)
	{
		status_t error = _UpdateAttributeDirectory();
		if (error != B_OK)
			RETURN_ERROR(error);

		if (fAttributeDirectory == NULL) {
			_countRead = 0;
			return B_OK;
		}

		return fDirCookie.ReadNextEntry(buffer, size, _countRead);
	}

	void Rewind()
	{
		fDirCookie.Rewind(true);
	}

private:
	status_t _UpdateAttributeDirectory()
	{
		uint64 blockIndex = fNode->AttributeDirectory();
		if (blockIndex == 0) {
			// no (empty) attribute directory
			if (fAttributeDirectory != NULL) {
				fNode->GetVolume()->PutNode(fAttributeDirectory);
				fAttributeDirectory = NULL;
			}

			return B_OK;
		}

		if (fAttributeDirectory != NULL) {
			if (blockIndex == fAttributeDirectory->BlockIndex())
				return B_OK;

			// The attribute directory has changed in the meantime -- get rid
			// of the old one.
			fNode->GetVolume()->PutNode(fAttributeDirectory);
			fAttributeDirectory = NULL;
		}

		// get the attribute directory node
		Node* node;
		status_t error = fNode->GetVolume()->GetNode(blockIndex, node);
		if (error != B_OK)
			RETURN_ERROR(error);

		fAttributeDirectory = dynamic_cast<Directory*>(node);
		if (fAttributeDirectory == NULL) {
			fNode->GetVolume()->PutNode(node);
			ERROR("checksumfs: attribute directory (%" B_PRIu64 ") of node %"
				B_PRIu64 " is not a directory!\n", blockIndex,
				fNode->BlockIndex());
			RETURN_ERROR(B_BAD_DATA);
		}

		fDirCookie.SetTo(fAttributeDirectory, true);

		return B_OK;
	}

private:
	Node*		fNode;
	Directory*	fAttributeDirectory;
	DirCookie	fDirCookie;
};


struct AttributeCookie {
	char*		name;
	Node*		attribute;
	FileCookie*	fileCookie;

	AttributeCookie(const char* name)
		:
		name(strdup(name)),
		attribute(NULL),
		fileCookie(NULL)
	{
	}

	~AttributeCookie()
	{
		if (attribute != NULL)
			attribute->GetVolume()->PutNode(attribute);
		delete fileCookie;
		free(name);
	}
};


// #pragma mark -


static void
set_timespec(timespec& time, uint64 nanos)
{
	time.tv_sec = nanos / 1000000000;
	time.tv_nsec = nanos % 1000000000;
}


static uint64
timespec_to_nsecs(const timespec& time)
{
	return (uint64)time.tv_sec * 1000000000 + time.tv_nsec;
}


struct PutNode {
	inline void operator()(Node* node)
	{
		if (node != NULL)
			node->GetVolume()->PutNode(node);
	}
};

typedef BPrivate::AutoDeleter<Node, PutNode> NodePutter;


static bool
is_user_in_group(gid_t gid)
{
	gid_t groups[NGROUPS_MAX];
	int groupCount = getgroups(NGROUPS_MAX, groups);
	for (int i = 0; i < groupCount; i++) {
		if (gid == groups[i])
			return true;
	}

	return gid == getegid();
}


static status_t
check_access(Node* node, uint32 accessFlags)
{
	// Note: we assume that the access flags are compatible with the permission
	// bits.
	STATIC_ASSERT(R_OK == S_IROTH && W_OK == S_IWOTH && X_OK == S_IXOTH);

	// get node permissions
	int userPermissions = (node->Mode() & S_IRWXU) >> 6;
	int groupPermissions = (node->Mode() & S_IRWXG) >> 3;
	int otherPermissions = node->Mode() & S_IRWXO;

	// get the permissions for this uid/gid
	int permissions = 0;
	uid_t uid = geteuid();

	if (uid == 0) {
		// user is root
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		permissions = userPermissions | groupPermissions | otherPermissions
			| R_OK | W_OK;
	} else if (uid == node->UID()) {
		// user is node owner
		permissions = userPermissions;
	} else if (is_user_in_group(node->GID())) {
		// user is in owning group
		permissions = groupPermissions;
	} else {
		// user is one of the others
		permissions = otherPermissions;
	}

	return (accessFlags & ~permissions) == 0 ? B_OK : B_NOT_ALLOWED;
}


static status_t
remove_entry(fs_volume* fsVolume, fs_vnode* parent, const char* name,
	bool removeDirectory)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Directory* directory
		= dynamic_cast<Directory*>((Node*)parent->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// Since we need to lock both nodes (the directory and the entry's), this
	// is a bit cumbersome. We first look up the entry while having the
	// directory read-locked, then drop the read lock, write-lock both nodes
	// and check whether anything has changed.
	Transaction transaction(volume);
	Node* childNode;
	NodePutter childNodePutter;

	while (true) {
		// look up the entry
		NodeReadLocker directoryLocker(directory);

		uint64 blockIndex;
		status_t error = directory->LookupEntry(name, blockIndex);
		if (error != B_OK)
			RETURN_ERROR(error);

		directoryLocker.Unlock();

		// get the entry's node
		error = volume->GetNode(blockIndex, childNode);
		if (error != B_OK)
			RETURN_ERROR(error);
		childNodePutter.SetTo(childNode);

		// start the transaction
		error = transaction.Start();
		if (error != B_OK)
			RETURN_ERROR(error);

		// write-lock the nodes
		error = transaction.AddNodes(directory, childNode);
		if (error != B_OK)
			RETURN_ERROR(error);

		// check the situation again
		error = directory->LookupEntry(name, blockIndex);
		if (error != B_OK)
			RETURN_ERROR(error);
		if (blockIndex != childNode->BlockIndex()) {
			transaction.Abort();
			continue;
		}

		break;
	}

	// check permissions
	status_t error = check_access(directory, W_OK);
	if (error != B_OK)
		return error;

	// check whether the child node type agrees with our caller
	if (removeDirectory) {
		if (!S_ISDIR(childNode->Mode()))
			RETURN_ERROR(B_NOT_A_DIRECTORY);

		// directory must be empty
		if (childNode->Size() > 0)
			RETURN_ERROR(B_DIRECTORY_NOT_EMPTY);
	} else if (S_ISDIR(childNode->Mode()))
		RETURN_ERROR(B_IS_A_DIRECTORY);

	// remove the entry
	error = directory->RemoveEntry(name, transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update stat data
	childNode->SetHardLinks(childNode->HardLinks() - 1);

	directory->Touched(NODE_MODIFIED);

	// remove the child node, if no longer referenced
	if (childNode->HardLinks() == 0) {
		error = volume->RemoveNode(childNode);
		if (error != B_OK)
			return error;
	}

	// commit the transaction
	return transaction.Commit(EntryRemovedNotification(directory, name,
		childNode));
}


/*!	Opens the node according to the given open mode (if the permissions allow
	that) and creates a file cookie.
*/
static status_t
open_file(Volume* volume, Node* node, int openMode, Transaction& transaction,
	bool commitTransaction, FileCookie*& _cookie)
{
	// translate the open mode to required permissions
	uint32 accessFlags = 0;
	switch (openMode & O_RWMASK) {
		case O_RDONLY:
			accessFlags = R_OK;
			break;
		case O_WRONLY:
			accessFlags = W_OK;
			break;
		case O_RDWR:
			accessFlags = R_OK | W_OK;
			break;
	}

	// We need to at least read-lock the node. If O_TRUNC is specified, we even
	// need a write lock and a transaction.
	NodeReadLocker nodeReadLocker;

	if ((openMode & O_TRUNC) != 0) {
		accessFlags |= W_OK;

		status_t error = transaction.IsActive()
			? transaction.AddNode(node) : transaction.StartAndAddNode(node);
		if (error != B_OK)
			RETURN_ERROR(error);
	} else if (!transaction.IsNodeLocked(node))
		nodeReadLocker.SetTo(node, false);

	// check permissions
	if ((accessFlags & W_OK) != 0) {
		if (volume->IsReadOnly())
			return B_READ_ONLY_DEVICE;
		if (S_ISDIR(node->Mode()))
			return B_IS_A_DIRECTORY;
	}

	if ((openMode & O_DIRECTORY) != 0 && !S_ISDIR(node->Mode()))
		return B_NOT_A_DIRECTORY;

	status_t error = check_access(node, accessFlags);
	if (error != B_OK)
		return error;

	// TODO: Support O_NOCACHE.

	FileCookie* cookie = new(std::nothrow) FileCookie(openMode);
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	// truncate the file, if requested
	if ((openMode & O_TRUNC) != 0 && node->Size() > 0) {
		error = node->Resize(0, false, transaction);
		if (error != B_OK)
			return error;

		node->Touched(NODE_MODIFIED);

		if (commitTransaction) {
			uint32 statFlags = B_STAT_MODIFICATION_TIME | B_STAT_CHANGE_TIME
				| B_STAT_SIZE;
			error = transaction.Commit(StatChangedNotification(node,
				statFlags));
			if (error != B_OK)
				return error;
		}
	}

	_cookie = cookieDeleter.Detach();
	return B_OK;
}


static status_t
create_file(Volume* volume, Directory* directory, const char* name,
	int openMode, int permissions, Transaction& transaction,
	bool commitTransaction, FileCookie*& _cookie, Node*& _node, bool& _created)
{
	Node* childNode = NULL;
	NodePutter childNodePutter;

	// Start the transaction and add the directory. We only need a read lock
	// for the lookup, but later we'll need a write lock, if we have to create
	// the file. So this is simpler.
	status_t error = B_OK;
	bool directoryLocked = false;
	if (transaction.IsActive()) {
		directoryLocked = transaction.IsNodeLocked(directory);
		if (!directoryLocked)
			error = transaction.AddNode(directory);
	} else
		error = transaction.StartAndAddNode(directory);
	if (error != B_OK)
		RETURN_ERROR(error);

	// look up the entry
	uint64 blockIndex;
	error = directory->LookupEntry(name, blockIndex);
	if (error == B_OK) {
		// the entry already exists
		if ((openMode & O_EXCL) != 0)
			return B_FILE_EXISTS;

		// get the entry's node
		error = volume->GetNode(blockIndex, childNode);
		if (error != B_OK)
			RETURN_ERROR(error);
		childNodePutter.SetTo(childNode);

		// We can (must even) unlock the directory now. The file won't go
		// anywhere, since a transaction is already running.
		if (!directoryLocked)
			transaction.RemoveNode(directory);

		error = open_file(volume, childNode, openMode, transaction,
			commitTransaction, _cookie);
		if (error != B_OK)
			RETURN_ERROR(error);

		childNodePutter.Detach();
		_node = childNode;
		_created = false;
		return B_OK;
	}

	if (error != B_ENTRY_NOT_FOUND)
		RETURN_ERROR(error);

	// The entry doesn't exist yet. We have to create a new file.

	// check the directory write permission
	error = check_access(directory, W_OK);
	if (error != B_OK)
		return error;

	// don't create an entry in an unlinked directory
	if (directory->HardLinks() == 0)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// create a file
	File* newFile;
	error = volume->CreateFile(permissions, transaction, newFile);
	if (error != B_OK)
		return error;

	// insert the new file
	error = directory->InsertEntry(name, newFile->BlockIndex(), transaction);
	if (error != B_OK)
		return error;

	// open the file
	FileCookie* cookie;
	error = open_file(volume, newFile, openMode & ~O_TRUNC, transaction,
		false, cookie);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	// update stat data
	newFile->SetHardLinks(1);
	newFile->SetParentDirectory(directory->BlockIndex());

	directory->Touched(NODE_MODIFIED);

	// announce the new vnode (needed for creating the file cache), but don't
	// publish it yet
	error = volume->NewNode(newFile);
	if (error != B_OK)
		RETURN_ERROR(error);

	// There's a vnode now -- the File object will be deleted when that is
	// removed.
	transaction.UpdateNodeFlags(newFile, TRANSACTION_REMOVE_NODE_ON_ERROR);

	// create the file cache
	error = newFile->InitForVFS();
	if (error != B_OK)
		return error;

	// node is fully initialized -- publish the vnode
	error = volume->PublishNode(newFile, 0);
	if (error != B_OK) {
		// publish_vnode() deletes the vnode on error, but it doesn't call the
		// remove_vnode() hook. So we need to make sure the object is deleted.
		transaction.UpdateNodeFlags(newFile, TRANSACTION_DELETE_NODE);
		RETURN_ERROR(error);
	}

	// commit the transaction
	if (commitTransaction) {
		error = transaction.Commit(EntryCreatedNotification(directory, name,
			newFile));
		if (error != B_OK) {
			volume->PutNode(newFile);
			RETURN_ERROR(error);
		}
	}

	_cookie = cookieDeleter.Detach();
	_node = newFile;
	_created = true;

	return B_OK;
}


/*!	Gets the node's attribute directory.
	If a transaction is given and the attribute directory doesn't exist, a new
	one is created and associate with the node.
	On success the caller gets a reference to the attribute directory and is
	responsible for putting it. If a transaction was given, the attribute
	directory must be put after committing/aborting the transaction.
*/
static status_t
get_attribute_directory(Node* node, Transaction* transaction,
	Directory*& _attributeDirectory)
{
	uint64 blockIndex = node->AttributeDirectory();
	Directory* attributeDirectory;

	if (blockIndex != 0) {
		// get the attribute directory node
		Node* attrDirNode;
		status_t error = node->GetVolume()->GetNode(blockIndex, attrDirNode);
		if (error != B_OK)
			RETURN_ERROR(error);

		attributeDirectory = dynamic_cast<Directory*>(attrDirNode);
		if (attributeDirectory == NULL) {
			node->GetVolume()->PutNode(node);
			ERROR("checksumfs: attribute directory (%" B_PRIu64 ") of node %"
				B_PRIu64 " is not a directory!\n", blockIndex,
				node->BlockIndex());
			RETURN_ERROR(B_BAD_DATA);
		}
	} else {
		// no (i.e. empty) attribute directory yet
		if (transaction == NULL)
			return B_ENTRY_NOT_FOUND;

		// create a new one
		status_t error = node->GetVolume()->CreateDirectory(
			S_IRWXU | S_IRWXG | S_IRWXO, *transaction, attributeDirectory);
		if (error != B_OK)
			RETURN_ERROR(error);

		attributeDirectory->SetMode(attributeDirectory->Mode() | S_ATTR_DIR);
		attributeDirectory->SetParentDirectory(node->BlockIndex());
		attributeDirectory->SetHardLinks(1);
		node->SetAttributeDirectory(attributeDirectory->BlockIndex());

		// publish it
		error = node->GetVolume()->PublishNode(attributeDirectory, 0);
		if (error != B_OK)
			RETURN_ERROR(error);

		// We have published the attribute directory, so don't delete it when
		// committing or aborting the transaction. Instead, on error remove it.
		transaction->UpdateNodeFlags(attributeDirectory,
			TRANSACTION_REMOVE_NODE_ON_ERROR);
	}

	_attributeDirectory = attributeDirectory;
	return B_OK;
}


// #pragma mark - FS operations


static float
checksumfs_identify_partition(int fd, partition_data* partition,
	void** _cookie)
{
	if ((uint64)partition->size < kCheckSumFSMinSize)
		return -1;

	SuperBlock* superBlock = new(std::nothrow) SuperBlock;
	if (superBlock == NULL)
		return -1;
	ObjectDeleter<SuperBlock> superBlockDeleter(superBlock);

	if (pread(fd, superBlock, sizeof(*superBlock), kCheckSumFSSuperBlockOffset)
			!= sizeof(*superBlock)) {
		return -1;
	}

	if (!superBlock->Check((uint64)partition->size / B_PAGE_SIZE))
		return -1;

	*_cookie = superBlockDeleter.Detach();
	return 0.8f;
}


static status_t
checksumfs_scan_partition(int fd, partition_data* partition, void* cookie)
{
	SuperBlock* superBlock = (SuperBlock*)cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = superBlock->TotalBlocks() * B_PAGE_SIZE;
	partition->block_size = B_PAGE_SIZE;
	partition->content_name = strdup(superBlock->Name());
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
checksumfs_free_identify_partition_cookie(partition_data* partition,
	void* cookie)
{
	SuperBlock* superBlock = (SuperBlock*)cookie;
	delete superBlock;
}


static status_t
checksumfs_mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* args, ino_t* _rootVnodeID)
{
	Volume* volume = new(std::nothrow) Volume(flags);
	if (volume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);

	status_t error = volume->Init(device);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = volume->Mount(fsVolume);
	if (error != B_OK)
		RETURN_ERROR(error);

	fsVolume->private_volume = volumeDeleter.Detach();
	fsVolume->ops = &gCheckSumFSVolumeOps;
	*_rootVnodeID = volume->RootDirectory()->BlockIndex();

	return B_OK;
}


static status_t
checksumfs_set_content_name(int fd, partition_id partition, const char* name,
	disk_job_id job)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


static status_t
checksumfs_initialize(int fd, partition_id partition, const char* name,
	const char* parameters, off_t partitionSize, disk_job_id job)
{
	if (name == NULL || strlen(name) >= kCheckSumFSNameLength)
		return B_BAD_VALUE;

	// TODO: Forcing a non-empty name here. Superfluous when the userland disk
	// system add-on has a parameter editor for it.
	if (*name == '\0')
		name = "Unnamed";

	update_disk_device_job_progress(job, 0);

	Volume volume(0);

	status_t error = volume.Init(fd, partitionSize / B_PAGE_SIZE);
	if (error != B_OK)
		return error;

	error = volume.Initialize(name);
	if (error != B_OK)
		return error;

	// rescan partition
	error = scan_partition(partition);
	if (error != B_OK)
		return error;

	update_disk_device_job_progress(job, 1);

	return B_OK;
}


// #pragma mark - volume operations


static status_t
checksumfs_unmount(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	volume->Unmount();
	return B_OK;
}


static status_t
checksumfs_read_fs_info(fs_volume* fsVolume, struct fs_info* info)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	volume->GetInfo(*info);
	return B_OK;
}


static status_t
checksumfs_write_fs_info(fs_volume* fsVolume, const struct fs_info* info,
	uint32 mask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	if ((mask & FS_WRITE_FSINFO_NAME) != 0) {
		status_t error = volume->SetName(info->volume_name);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


static status_t
checksumfs_sync(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	return block_cache_sync(volume->BlockCache());
}


static status_t
checksumfs_get_vnode(fs_volume* fsVolume, ino_t id, fs_vnode* vnode,
	int* _type, uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	Node* node;
	status_t error = volume->ReadNode(id, node);
	if (error != B_OK)
		return error;

	error = node->InitForVFS();
	if (error != B_OK) {
		delete node;
		return error;
	}

	vnode->private_node = node;
	vnode->ops = &gCheckSumFSVnodeOps;
	*_type = node->Mode();
	*_flags = 0;

	return B_OK;
}


// #pragma mark - vnode operations


static status_t
checksumfs_lookup(fs_volume* fsVolume, fs_vnode* fsDir, const char* name,
	ino_t* _id)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsDir->private_node;

	Directory* directory = dynamic_cast<Directory*>(node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	status_t error = check_access(directory, X_OK);
	if (error != B_OK)
		return error;

	NodeReadLocker nodeLocker(node);

	uint64 blockIndex;

	if (strcmp(name, ".") == 0) {
		blockIndex = directory->BlockIndex();
	} else if (strcmp(name, "..") == 0) {
		blockIndex = directory->ParentDirectory();
	} else {
		status_t error = directory->LookupEntry(name, blockIndex);
		if (error != B_OK)
			return error;
	}

	// get the node
	Node* childNode;
	error = volume->GetNode(blockIndex, childNode);
	if (error != B_OK)
		return error;

	*_id = blockIndex;
	return B_OK;
}


static status_t
checksumfs_put_vnode(fs_volume* fsVolume, fs_vnode* vnode, bool reenter)
{
	Node* node = (Node*)vnode->private_node;
	delete node;
	return B_OK;
}


static status_t
checksumfs_remove_vnode(fs_volume* fsVolume, fs_vnode* vnode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;
	return volume->DeleteNode(node);
}



// #pragma mark - asynchronous I/O


static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, file_io_vec* vecs, size_t* _count)
{
	File* file = (File*)cookie;

	RETURN_ERROR(file_map_translate(file->FileMap(), offset, size, vecs, _count,
		B_PAGE_SIZE));
}


static status_t
iterative_io_finished_hook(void* cookie, io_request* request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	File* file = (File*)cookie;
	file->ReadUnlock();
	return B_OK;
}


static status_t
checksumfs_io(fs_volume* fsVolume, fs_vnode* vnode, void* cookie,
	io_request* request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	File* file = dynamic_cast<File*>((Node*)vnode->private_node);
	if (file == NULL) {
        notify_io_request(request, B_READ_ONLY_DEVICE);
		RETURN_ERROR(B_BAD_VALUE);
	}

	if (io_request_is_write(request) && volume->IsReadOnly()) {
        notify_io_request(request, B_READ_ONLY_DEVICE);
		RETURN_ERROR(B_READ_ONLY_DEVICE);
	}

	// Read-lock the file -- we'll unlock it in the finished hook.
	if (io_request_is_vip(request)) {
		// We cannot wait for the node lock indefinitely. So try read-locking
		// with a timeout (0.1 s).
		if (!file->ReadLockWithTimeout(B_RELATIVE_TIMEOUT, 100000)) {
	        notify_io_request(request, B_BUSY);
			RETURN_ERROR(B_BUSY);
		}
	} else
		file->ReadLock();

	RETURN_ERROR(do_iterative_fd_io(volume->FD(), request,
		iterative_io_get_vecs_hook, iterative_io_finished_hook, file));
}


// #pragma mark - cache file access


static status_t
checksumfs_get_file_map(fs_volume* fsVolume, fs_vnode* vnode, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	if (offset < 0)
		RETURN_ERROR(B_BAD_VALUE);

	File* file = dynamic_cast<File*>((Node*)vnode->private_node);
	if (file == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	RETURN_ERROR(file->GetFileVecs(offset, size, vecs, *_count, *_count));
}


// #pragma mark - common operations


static status_t
checksumfs_set_flags(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie,
	int flags)
{
	FileCookie* cookie = (FileCookie*)_cookie;

	cookie->openMode = (cookie->openMode & ~O_APPEND) | (flags & O_APPEND);

	// TODO: Also support O_NOCACHE!

	return B_OK;
}


static status_t
checksumfs_fsync(fs_volume* fsVolume, fs_vnode* vnode)
{
	Node* node = (Node*)vnode->private_node;

	NodeReadLocker nodeLocker(node);

	return node->Sync();
}


static status_t
checksumfs_read_symlink(fs_volume* fsVolume, fs_vnode* vnode, char* buffer,
	size_t* _bufferSize)
{
	SymLink* symLink = dynamic_cast<SymLink*>((Node*)vnode->private_node);
	if (symLink == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status_t error = check_access(symLink, R_OK);
	if (error != B_OK)
		return error;

	return symLink->ReadSymLink(buffer, *_bufferSize, *_bufferSize);
}


static status_t
checksumfs_create_symlink(fs_volume* fsVolume, fs_vnode* parent,
	const char* name, const char* path, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Directory* directory
		= dynamic_cast<Directory*>((Node*)parent->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	status_t error = check_access(directory, W_OK);
	if (error != B_OK)
		return error;

	// start a transaction and add the directory (write locks it, too)
	Transaction transaction(volume);
	error = transaction.StartAndAddNode(directory);
	if (error != B_OK)
		return error;

	// don't create an entry in an unlinked directory
	if (directory->HardLinks() == 0)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// create a symlink node
	SymLink* newSymLink;
	error = volume->CreateSymLink(mode, transaction, newSymLink);
	if (error != B_OK)
		return error;

	// write it
	error = newSymLink->WriteSymLink(path, strlen(path), transaction);
	if (error != B_OK)
		return error;

	// insert the new symlink
	error = directory->InsertEntry(name, newSymLink->BlockIndex(), transaction);
	if (error != B_OK)
		return error;

	// update stat data
	newSymLink->SetHardLinks(1);

	directory->Touched(NODE_MODIFIED);

	// commit the transaction
	return transaction.Commit(EntryCreatedNotification(directory, name,
		newSymLink));
}


static status_t
checksumfs_link(fs_volume* fsVolume, fs_vnode* dir, const char* name,
	fs_vnode* vnode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;
	Directory* directory = dynamic_cast<Directory*>((Node*)dir->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// don't allow hardlinking directories
	if (S_ISDIR(node->Mode()))
		RETURN_ERROR(B_NOT_ALLOWED);

	// start a transaction and lock the nodes
	Transaction transaction(volume);
	status_t error = transaction.Start();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = transaction.AddNodes(directory, node);
	if (error != B_OK)
		RETURN_ERROR(error);

	// check permissions
	error = check_access(directory, W_OK);
	if (error != B_OK)
		RETURN_ERROR(error);

	// don't create an entry in an unlinked directory
	if (directory->HardLinks() == 0)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// insert the new entry
	error = directory->InsertEntry(name, node->BlockIndex(), transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update stat data
	node->SetHardLinks(node->HardLinks() + 1);
	directory->Touched(NODE_MODIFIED);

	// commit the transaction
	return transaction.Commit(EntryCreatedNotification(directory, name, node));
}


static status_t
checksumfs_unlink(fs_volume* fsVolume, fs_vnode* dir, const char* name)
{
	return remove_entry(fsVolume, dir, name, false);
}


static status_t
checksumfs_rename(fs_volume* fsVolume, fs_vnode* fromDir, const char* fromName,
	fs_vnode* toDir, const char* toName)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Directory* fromDirectory
		= dynamic_cast<Directory*>((Node*)fromDir->private_node);
	Directory* toDirectory
		= dynamic_cast<Directory*>((Node*)toDir->private_node);
	if (fromDirectory == NULL || toDirectory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// We need to write-lock all three nodes (both directories and the moved
	// node). To make that atomic, we have to lock the source directory, look up
	// the entry, unlock the directory, get the node, re-lock all, and look up
	// the entry again to check for changes.
	Transaction transaction(volume);
	Node* node;
	NodePutter nodePutter;

	while (true) {
		// look up the entry
		NodeReadLocker directoryLocker(fromDirectory);

		uint64 blockIndex;
		status_t error = fromDirectory->LookupEntry(fromName, blockIndex);
		if (error != B_OK)
			RETURN_ERROR(error);

		directoryLocker.Unlock();

		// get the entry's node
		error = volume->GetNode(blockIndex, node);
		if (error != B_OK)
			RETURN_ERROR(error);
		nodePutter.SetTo(node);

		// start the transaction
		error = transaction.Start();
		if (error != B_OK)
			RETURN_ERROR(error);

		// write-lock the nodes
		error = fromDirectory != toDirectory
			? transaction.AddNodes(fromDirectory, toDirectory, node)
			: transaction.AddNodes(fromDirectory, node);
		if (error != B_OK)
			RETURN_ERROR(error);

		// check the situation again
		error = fromDirectory->LookupEntry(fromName, blockIndex);
		if (error != B_OK)
			RETURN_ERROR(error);
		if (blockIndex != node->BlockIndex()) {
			transaction.Abort();
			continue;
		}

		break;
	}

	// check permissions
	status_t error = check_access(fromDirectory, W_OK);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = check_access(toDirectory, W_OK);
	if (error != B_OK)
		RETURN_ERROR(error);

	// don't create an entry in an unlinked directory
	if (toDirectory->HardLinks() == 0)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// Check whether this operation would move a directory into one of its
	// descendents. We iterate upwards, checking whether any ancestor of the
	// target directory is the moved directory (if it is a directory that is).
	if (fromDirectory != toDirectory && S_ISDIR(node->Mode())) {
		NodePutter ancestorPutter;
		Node* ancestor = toDirectory;

		while (ancestor != volume->RootDirectory()
			|| ancestor == fromDirectory) {
			if (ancestor == node)
				RETURN_ERROR(B_BAD_VALUE);

			error = volume->GetNode(ancestor->ParentDirectory(), ancestor);
			if (error != B_OK)
				RETURN_ERROR(error);
			ancestorPutter.SetTo(ancestor);
		}
	}

	// Everything looks good -- insert a new entry in the target directory and
	// remove the old entry from the source directory.
	error = toDirectory->InsertEntry(toName, node->BlockIndex(), transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fromDirectory->RemoveEntry(fromName, transaction);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update stat data
	node->SetParentDirectory(toDirectory->BlockIndex());
	fromDirectory->Touched(NODE_MODIFIED);
	toDirectory->Touched(NODE_MODIFIED);

	// commit the transaction
	return transaction.Commit(EntryMovedNotification(fromDirectory, fromName,
		toDirectory, toName, node));
}


static status_t
checksumfs_access(fs_volume* fsVolume, fs_vnode* vnode, int mode)
{
	Node* node = (Node*)vnode->private_node;

	NodeReadLocker nodeLocker(node);

	return check_access(node, mode);
}


static status_t
checksumfs_read_stat(fs_volume* fsVolume, fs_vnode* vnode, struct stat* st)
{
	Node* node = (Node*)vnode->private_node;

	NodeReadLocker nodeLocker(node);

    st->st_mode = node->Mode();
    st->st_nlink = node->HardLinks();
    st->st_uid = node->UID();
    st->st_gid = node->GID();
    st->st_size = node->Size();
    st->st_blksize = B_PAGE_SIZE * 16;	// random number
    set_timespec(st->st_mtim, node->ModificationTime());
    set_timespec(st->st_ctim, node->ChangeTime());
    set_timespec(st->st_crtim, node->CreationTime());
    set_timespec(st->st_atim, node->AccessedTime());
    st->st_type = 0;        /* attribute/index type */
    st->st_blocks = 1 + (st->st_size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// TODO: That does neither count management structures for the content
		// (for files) nor attributes.

	return B_OK;
}


static status_t
checksumfs_write_stat(fs_volume* fsVolume, fs_vnode* vnode,
	const struct stat* st, uint32 statMask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// start a transaction and add the node to it (write locks the node, too)
	Transaction transaction(volume);
	status_t error = transaction.StartAndAddNode(node);
	if (error != B_OK)
		return error;

	uid_t uid = geteuid();
	bool isOwnerOrRoot = uid == 0 || uid == node->UID();
	bool hasWriteAccess = check_access(node, W_OK) == B_OK;

	bool updateModified = false;
	bool updateChanged = false;

	if ((statMask & B_STAT_SIZE) != 0 && (uint64)st->st_size != node->Size()) {
		if (!hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		error = node->Resize(st->st_size, true, transaction);
		if (error != B_OK)
			RETURN_ERROR(error);

		updateModified = updateChanged = true;
	}

	if ((statMask & B_STAT_UID) != 0 && st->st_uid != node->UID()) {
		// only root can do that
		if (uid != 0)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetUID(st->st_uid);
		updateChanged = true;
	}

	if ((statMask & B_STAT_GID) != 0 && st->st_gid != node->GID()) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetGID(st->st_gid);
		updateChanged = true;
	}

	if ((statMask & B_STAT_MODE) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetMode((node->Mode() & ~(mode_t)S_IUMSK)
			| (st->st_mode & S_IUMSK));
		updateChanged = true;
	}

	if ((statMask & B_STAT_CREATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetCreationTime(timespec_to_nsecs(st->st_crtim));
		updateChanged = true;
	}

	if ((statMask & B_STAT_MODIFICATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetModificationTime(timespec_to_nsecs(st->st_mtim));
		updateModified = false;
		updateChanged = true;
	}

	if ((statMask & B_STAT_CHANGE_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		node->SetModificationTime(timespec_to_nsecs(st->st_mtim));
		updateModified = false;
		updateChanged = false;
	}

	// update access/change/modification time
	if (updateModified)
		node->Touched(NODE_MODIFIED);
	else if (updateChanged)
		node->Touched(NODE_STAT_CHANGED);
	else
		node->Touched(NODE_ACCESSED);

	// commit the transaction
	return transaction.Commit(StatChangedNotification(node, statMask));
}


// #pragma mark - file operations


static status_t
checksumfs_create(fs_volume* fsVolume, fs_vnode* parent, const char* name,
	int openMode, int permissions, void** _cookie, ino_t* _newVnodeID)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Directory* directory
		= dynamic_cast<Directory*>((Node*)parent->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	Transaction transaction(volume);
	FileCookie* cookie;
	Node* node;
	bool created;
	status_t error = create_file(volume, directory, name, openMode, permissions,
		transaction, true, cookie, node, created);
	if (error != B_OK)
		RETURN_ERROR(error);

	*_cookie = cookie;
	*_newVnodeID = node->BlockIndex();

	return B_OK;
}


static status_t
checksumfs_open(fs_volume* fsVolume, fs_vnode* vnode, int openMode,
	void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;

	// don't allow opening an attribute this way
	if ((node->Mode() & S_ATTR) != 0)
		RETURN_ERROR(B_BAD_VALUE);

	Transaction transaction(volume);
	FileCookie* cookie;
	status_t error = open_file(volume, node, openMode, transaction, true,
		cookie);
	if (error != B_OK)
		RETURN_ERROR(error);

	*_cookie = cookie;
	return B_OK;
}


static status_t
checksumfs_close(fs_volume* fsVolume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
checksumfs_free_cookie(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie)
{
	FileCookie* cookie = (FileCookie*)_cookie;
	Node* node = (Node*)vnode->private_node;

	cookie->UpdateModifiedIfNecessary(node, true);

	delete cookie;
	return B_OK;
}


static status_t
checksumfs_read(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	FileCookie* cookie = (FileCookie*)_cookie;
	Node* node = (Node*)vnode->private_node;

	switch (cookie->openMode & O_RWMASK) {
		case O_RDONLY:
		case O_RDWR:
			break;
		case O_WRONLY:
		default:
			RETURN_ERROR(EBADF);
	}

	return node->Read(pos, buffer, *_length, *_length);
}


static status_t
checksumfs_write(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie, off_t pos,
	const void* buffer, size_t* _length)
{
	FileCookie* cookie = (FileCookie*)_cookie;
	Node* node = (Node*)vnode->private_node;

	switch (cookie->openMode & O_RWMASK) {
		case O_WRONLY:
		case O_RDWR:
			break;
		case O_RDONLY:
		default:
			RETURN_ERROR(EBADF);
	}

	if (pos < 0)
		RETURN_ERROR(B_BAD_VALUE);

	if ((cookie->openMode & O_APPEND) != 0) {
		pos = -1;
			// special value handled by Write()
	}

	bool sizeChanged;
	status_t error = node->Write(pos, buffer, *_length, *_length, sizeChanged);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update the modification time and send out a notification from time to
	// time
	cookie->FileModified(node, sizeChanged);

	return B_OK;
}


// #pragma mark - directory operations


status_t
checksumfs_create_dir(fs_volume* fsVolume, fs_vnode* parent, const char* name,
	int perms)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Directory* directory
		= dynamic_cast<Directory*>((Node*)parent->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	status_t error = check_access(directory, W_OK);
	if (error != B_OK)
		return error;

	// start a transaction and attach the directory (write locks it, too)
	Transaction transaction(volume);
	error = transaction.StartAndAddNode(directory);
	if (error != B_OK)
		return error;

	// don't create an entry in an unlinked directory
	if (directory->HardLinks() == 0)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// create a directory node
	Directory* newDirectory;
	error = volume->CreateDirectory(perms, transaction, newDirectory);
	if (error != B_OK)
		return error;

	// insert the new directory
	error = directory->InsertEntry(name, newDirectory->BlockIndex(),
		transaction);
	if (error != B_OK)
		return error;

	// update stat data
	newDirectory->SetHardLinks(1);
	newDirectory->SetParentDirectory(directory->BlockIndex());

	directory->Touched(NODE_MODIFIED);

	// commit the transaction
	return transaction.Commit(EntryCreatedNotification(directory, name,
		newDirectory));
}


status_t
checksumfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	return remove_entry(volume, parent, name, true);
}


static status_t
checksumfs_open_dir(fs_volume* fsVolume, fs_vnode* vnode, void** _cookie)
{
	Directory* directory = dynamic_cast<Directory*>((Node*)vnode->private_node);
	if (directory == NULL)
		return B_NOT_A_DIRECTORY;

	NodeReadLocker nodeLocker(directory);

	// don't allow opening an attribute directory this way
	if ((directory->Mode() & S_ATTR_DIR) != 0)
		RETURN_ERROR(B_BAD_VALUE);

	status_t error = check_access(directory, R_OK);
	if (error != B_OK)
		return error;

	DirCookie* cookie = new(std::nothrow) DirCookie(directory);
	if (cookie == NULL)
		return B_NO_MEMORY;

	*_cookie = cookie;
	return B_OK;
}


static status_t
checksumfs_close_dir(fs_volume* fsVolume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
checksumfs_free_dir_cookie(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie)
{
	DirCookie* cookie = (DirCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
checksumfs_read_dir(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _num)
{
	if (*_num == 0)
		return B_OK;

	DirCookie* cookie = (DirCookie*)_cookie;

	NodeReadLocker nodeLocker(cookie->GetDirectory());

	return cookie->ReadNextEntry(buffer, bufferSize, *_num);
}


static status_t
checksumfs_rewind_dir(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie)
{
	DirCookie* cookie = (DirCookie*)_cookie;

	NodeReadLocker nodeLocker(cookie->GetDirectory());

	cookie->Rewind();
	return B_OK;
}


// #pragma mark - attribute directory operations


static status_t
checksumfs_open_attr_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	Node* node = (Node*)vnode->private_node;

	NodeReadLocker nodeLocker(node);

	status_t error = check_access(node, R_OK);
	if (error != B_OK)
		return error;

	AttrDirCookie* cookie = new(std::nothrow) AttrDirCookie(node);
	if (cookie == NULL)
		return B_NO_MEMORY;

	*_cookie = cookie;
	return B_OK;
}


static status_t
checksumfs_close_attr_dir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
checksumfs_free_attr_dir_cookie(fs_volume* volume, fs_vnode* vnode,
	void* _cookie)
{
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
checksumfs_read_attr_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _num)
{
	if (*_num == 0)
		return B_OK;

	Node* node = (Node*)vnode->private_node;
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	NodeReadLocker nodeLocker(node);

	return cookie->ReadNextEntry(buffer, bufferSize, *_num);
}


static status_t
checksumfs_rewind_attr_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	Node* node = (Node*)vnode->private_node;
	AttrDirCookie* cookie = (AttrDirCookie*)_cookie;

	NodeReadLocker nodeLocker(node);

	cookie->Rewind();
	return B_OK;
}


// #pragma mark - attribute operations


static status_t
checksumfs_create_attr(fs_volume* fsVolume, fs_vnode* vnode, const char* name,
	uint32 type, int openMode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;
	if (node == NULL)
		return B_NOT_A_DIRECTORY;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// create the attribute cookie
	AttributeCookie* cookie = new(std::nothrow) AttributeCookie(name);
	if (cookie == NULL || cookie->name == NULL) {
		delete cookie;
		return B_NO_MEMORY;
	}
	ObjectDeleter<AttributeCookie> cookieDeleter(cookie);

	// Start a transaction and lock the node.
	// Note: Other than for ordinary nodes the locking order when attributes
	// are involved is: node -> attribute directory -> attribute.
	Transaction transaction(volume);
	status_t error = transaction.StartAndAddNode(node);
	if (error != B_OK)
		RETURN_ERROR(error);

	// check permissions
	error = check_access(node, W_OK);
	if (error != B_OK)
		return error;

	// get the attribute directory (create, if necessary)
	Directory* attributeDirectory;
	error = get_attribute_directory(node, &transaction, attributeDirectory);
	if (error != B_OK)
		RETURN_ERROR(error);
	NodePutter attributeDirectoryPutter(attributeDirectory);

	// open/create the attribute
	bool created;
	error = create_file(volume, attributeDirectory, name, openMode,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, transaction,
		false, cookie->fileCookie, cookie->attribute, created);
	if (error != B_OK)
		RETURN_ERROR(error);

	if (created) {
		cookie->attribute->SetMode(cookie->attribute->Mode() | S_ATTR);
		cookie->attribute->SetAttributeType(type);
	}

	// commit the transaction
	if (transaction.IsActive()) {
		if (created || (openMode & O_TRUNC) != 0) {
			node->Touched(NODE_STAT_CHANGED);

			AttributeChangedNotification attributeNotification(node, name,
				created ? B_ATTR_CREATED : B_ATTR_CHANGED);
			StatChangedNotification statNotification(node, B_STAT_CHANGE_TIME);
			error = transaction.Commit(&attributeNotification,
				&statNotification);
		} else
			error = transaction.Commit();
	}

	*_cookie = cookieDeleter.Detach();
	return B_OK;
}


static status_t
checksumfs_open_attr(fs_volume* fsVolume, fs_vnode* vnode, const char* name,
	int openMode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;

	// create the attribute cookie
	AttributeCookie* cookie = new(std::nothrow) AttributeCookie(name);
	if (cookie == NULL || cookie->name == NULL) {
		delete cookie;
		return B_NO_MEMORY;
	}
	ObjectDeleter<AttributeCookie> cookieDeleter(cookie);

	// Get the node's attribute directory (don't create it, if it doesn't exist
	// yet). We only need to read-lock the node for that, but when O_TRUNC is
	// given, we already start the transaction and write-lock the node, so we
	// don't get a locking order inversion later. The locking order when
	// attributes are involved is: node -> attribute directory -> attribute.
	Transaction transaction(volume);
	NodeReadLocker readLocker;
	if ((openMode & O_TRUNC) != 0) {
		status_t error = transaction.StartAndAddNode(node);
		if (error != B_OK)
			RETURN_ERROR(error);
	} else
		readLocker.SetTo(node, false);

	Directory* attributeDirectory;
	status_t error = get_attribute_directory(node, NULL, attributeDirectory);
	if (error != B_OK)
		RETURN_ERROR(error);
	NodePutter attributeDirectoryPutter(attributeDirectory);

	// look up the attribute
	readLocker.SetTo(attributeDirectory, false);
	uint64 blockIndex;
	error = attributeDirectory->LookupEntry(name, blockIndex);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = volume->GetNode(blockIndex, cookie->attribute);
		// the vnode reference directly goes to the cookie in case of success
	if (error != B_OK)
		RETURN_ERROR(error);

	// open the attribute
	error = open_file(volume, cookie->attribute, openMode, transaction, false,
		cookie->fileCookie);
	if (error != B_OK)
		RETURN_ERROR(error);

	// commit the transaction
	if (transaction.IsActive()) {
		if ((openMode & O_TRUNC) != 0) {
			node->Touched(NODE_STAT_CHANGED);

			AttributeChangedNotification attributeNotification(node, name,
				B_ATTR_CHANGED);
			StatChangedNotification statNotification(node, B_STAT_CHANGE_TIME);
			error = transaction.Commit(&attributeNotification,
				&statNotification);
		} else
			error = transaction.Commit();
	}

	*_cookie = cookieDeleter.Detach();
	return B_OK;
}


static status_t
checksumfs_close_attr(fs_volume* fsVolume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
checksumfs_free_attr_cookie(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
checksumfs_read_attr(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie,
	off_t pos, void* buffer, size_t* _length)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	switch (cookie->fileCookie->openMode & O_RWMASK) {
		case O_RDONLY:
		case O_RDWR:
			break;
		case O_WRONLY:
		default:
			RETURN_ERROR(EBADF);
	}

	return cookie->attribute->Read(pos, buffer, *_length, *_length);
}


static status_t
checksumfs_write_attr(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie,
	off_t pos, const void* buffer, size_t* _length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	switch (cookie->fileCookie->openMode & O_RWMASK) {
		case O_WRONLY:
		case O_RDWR:
			break;
		case O_RDONLY:
		default:
			RETURN_ERROR(EBADF);
	}

	if (pos < 0)
		RETURN_ERROR(B_BAD_VALUE);

	if ((cookie->fileCookie->openMode & O_APPEND) != 0) {
		pos = -1;
			// special value handled by Write()
	}

	bool sizeChanged;
	status_t error = cookie->attribute->Write(pos, buffer, *_length, *_length,
		sizeChanged);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update the node changed time and send out a notifications (don't fail,
	// if any of this fails)
	Transaction transaction(volume);
	if (transaction.StartAndAddNode(node) != B_OK)
		return B_OK;
	if (transaction.AddNode(cookie->attribute) != B_OK)
		return B_OK;

	cookie->attribute->Touched(NODE_MODIFIED);

	// commit the transaction
	if (cookie->attribute->ParentDirectory() != 0) {
		node->Touched(NODE_STAT_CHANGED);

		AttributeChangedNotification attributeNotification(node, cookie->name,
			B_ATTR_CHANGED);
		StatChangedNotification statNotification(node, B_STAT_CHANGE_TIME);
		transaction.Commit(&attributeNotification, &statNotification);
	} else {
		// attribute has been removed -- no notifications needed
		transaction.Commit();
	}

	return B_OK;
}


static status_t
checksumfs_read_attr_stat(fs_volume* fsVolume, fs_vnode* vnode, void* _cookie,
	struct stat* st)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	// not many fields needed ATM
	st->st_size = cookie->attribute->Size();
    st->st_type = cookie->attribute->AttributeType();

	return B_OK;
}


static status_t
checksumfs_remove_attr(fs_volume* fsVolume, fs_vnode* vnode, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)vnode->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// start a transaction
	Transaction transaction(volume);
	status_t error = transaction.StartAndAddNode(node);
	if (error != B_OK)
		RETURN_ERROR(error);

	// check permissions
	error = check_access(node, W_OK);
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the attribute directory
	Directory* attributeDirectory;
	error = get_attribute_directory(node, NULL, attributeDirectory);
	if (error != B_OK)
		RETURN_ERROR(error);
	NodePutter attributeDirectoryPutter(attributeDirectory);

	error = transaction.AddNode(attributeDirectory);
	if (error != B_OK)
		RETURN_ERROR(error);

	// look up the entry
	uint64 blockIndex;
	error = attributeDirectory->LookupEntry(name, blockIndex);
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the attribute node
	Node* attribute;
	error = volume->GetNode(blockIndex, attribute);
	if (error != B_OK)
		RETURN_ERROR(error);
	NodePutter attributePutter(attribute);

	error = transaction.AddNode(attribute);
	if (error != B_OK) {
		volume->PutNode(attribute);
		RETURN_ERROR(error);
	}

	// remove the entry
	bool attrDirEmpty;
	error = attributeDirectory->RemoveEntry(name, transaction, &attrDirEmpty);
	if (error != B_OK)
		RETURN_ERROR(error);

	// remove the attribute node
	error = volume->RemoveNode(attribute);
	if (error != B_OK)
		return error;
	transaction.UpdateNodeFlags(attribute, TRANSACTION_UNREMOVE_NODE_ON_ERROR);

	// if the attribute directory is empty now, remove it too
	if (attrDirEmpty) {
		error = volume->RemoveNode(attributeDirectory);
		if (error != B_OK)
			return error;
		transaction.UpdateNodeFlags(attributeDirectory,
			TRANSACTION_UNREMOVE_NODE_ON_ERROR);

		node->SetAttributeDirectory(0);
	}

	// update stat data
	attribute->SetHardLinks(0);

	node->Touched(NODE_STAT_CHANGED);

	// commit the transaction
	AttributeChangedNotification attributeNotification(node, name,
		B_ATTR_REMOVED);
	StatChangedNotification statNotification(node, B_STAT_CHANGE_TIME);
	return transaction.Commit(&attributeNotification, &statNotification);
}


// #pragma mark - module


static status_t
checksumfs_std_ops(int32 operation, ...)
{
	switch (operation) {
		case B_MODULE_INIT:
			init_debugging();
			PRINT("checksumfs_std_ops(): B_MODULE_INIT\n");
			return B_OK;

		case B_MODULE_UNINIT:
			PRINT("checksumfs_std_ops(): B_MODULE_UNINIT\n");
			exit_debugging();
			return B_OK;

		default:
			return B_BAD_VALUE;
	}
}


static file_system_module_info sFSModule = {
	{
		kCheckSumFSModuleName,
		0,
		checksumfs_std_ops
	},
	kCheckSumFSShortName,
	CHECK_SUM_FS_PRETTY_NAME,
	// DDM flags
	B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_WRITING,

	/* scanning (the device is write locked) */
	checksumfs_identify_partition,
	checksumfs_scan_partition,
	checksumfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie

	/* general operations */
	checksumfs_mount,

	/* capability querying (the device is read locked) */
	NULL,	// get_supported_operations

	NULL,	// validate_resize
	NULL,	// validate_move
	NULL,	// validate_set_content_name
	NULL,	// validate_set_content_parameters
	NULL,	// validate_initialize

	/* shadow partition modification (device is write locked) */
	NULL,	// shadow_changed

	/* writing (the device is NOT locked) */
	NULL,	// defragment
	NULL,	// repair
	NULL,	// resize
	NULL,	// move
	checksumfs_set_content_name,
	NULL,	// set_content_parameters
	checksumfs_initialize
};


const module_info* modules[] = {
	(module_info*)&sFSModule,
	NULL
};


fs_volume_ops gCheckSumFSVolumeOps = {
	checksumfs_unmount,

	checksumfs_read_fs_info,
	checksumfs_write_fs_info,
	checksumfs_sync,

	checksumfs_get_vnode,

	/* index directory & index operations */
	NULL,	// open_index_dir
	NULL,	// close_index_dir
	NULL,	// free_index_dir_cookie
	NULL,	// read_index_dir
	NULL,	// rewind_index_dir

	NULL,	// create_index
	NULL,	// remove_index
	NULL,	// read_index_stat

	/* query operations */
	NULL,	// open_query
	NULL,	// close_query
	NULL,	// free_query_cookie
	NULL,	// read_query
	NULL,	// rewind_query

	/* support for FS layers */
	NULL,	// all_layers_mounted
	NULL,	// create_sub_vnode
	NULL,	// delete_sub_vnode
};


fs_vnode_ops gCheckSumFSVnodeOps = {
	/* vnode operations */
	checksumfs_lookup,
	NULL,	// get_vnode_name

	checksumfs_put_vnode,
	checksumfs_remove_vnode,

	/* VM file access */
	NULL,	// can_page
	NULL,	// read_pages
	NULL,	// write_pages

	/* asynchronous I/O */
	checksumfs_io,
	NULL,	// cancel_io

	/* cache file access */
	checksumfs_get_file_map,

	/* common operations */
	NULL,	// ioctl
	checksumfs_set_flags,
	NULL,	// select
	NULL,	// deselect
	checksumfs_fsync,

	checksumfs_read_symlink,
	checksumfs_create_symlink,

	checksumfs_link,
	checksumfs_unlink,
	checksumfs_rename,

	checksumfs_access,
	checksumfs_read_stat,
	checksumfs_write_stat,
	NULL,	// preallocate

	/* file operations */
	checksumfs_create,
	checksumfs_open,
	checksumfs_close,
	checksumfs_free_cookie,
	checksumfs_read,
	checksumfs_write,

	/* directory operations */
	checksumfs_create_dir,
	checksumfs_remove_dir,
	checksumfs_open_dir,
	checksumfs_close_dir,
	checksumfs_free_dir_cookie,
	checksumfs_read_dir,
	checksumfs_rewind_dir,

	/* attribute directory operations */
	checksumfs_open_attr_dir,
	checksumfs_close_attr_dir,
	checksumfs_free_attr_dir_cookie,
	checksumfs_read_attr_dir,
	checksumfs_rewind_attr_dir,

	/* attribute operations */
	checksumfs_create_attr,
	checksumfs_open_attr,
	checksumfs_close_attr,
	checksumfs_free_attr_cookie,
	checksumfs_read_attr,
	checksumfs_write_attr,

	checksumfs_read_attr_stat,
	NULL,	// write_attr_stat
	NULL,	// rename_attr
	checksumfs_remove_attr,

	/* support for node and FS layers */
	NULL,	// create_special_node
	NULL	// get_super_vnode
};