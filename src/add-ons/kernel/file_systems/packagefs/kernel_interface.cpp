/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "kernel_interface.h"

#include <dirent.h>

#include <algorithm>
#include <new>

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>
#include <io_requests.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "Directory.h"
#include "GlobalFactory.h"
#include "LeafNode.h"
#include "Volume.h"


using BPackageKit::BHPKG::BBufferDataReader;
using BPackageKit::BHPKG::BFDDataReader;


static const uint32 kOptimalIOSize = 64 * 1024;


// #pragma mark - helper functions


static bool
is_user_in_group(gid_t gid)
{
	gid_t groups[NGROUPS_MAX];
	int groupCount = getgroups(NGROUPS_MAX, groups);
	for (int i = 0; i < groupCount; i++) {
		if (gid == groups[i])
			return true;
	}

	return (gid == getegid());
}


static bool
set_dirent_name(struct dirent* buffer, size_t bufferSize, const char* name,
	size_t nameLen)
{
	size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
	if (length > bufferSize)
		return false;

	memcpy(buffer->d_name, name, nameLen);
	buffer->d_name[nameLen] = '\0';
	buffer->d_reclen = length;
	return true;
}


static status_t
check_access(Node* node, int mode)
{
	// write access requested?
	if (mode & W_OK)
		return B_READ_ONLY_DEVICE;

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
			| S_IROTH | S_IWOTH;
	} else if (uid == node->UserID()) {
		// user is node owner
		permissions = userPermissions;
	} else if (is_user_in_group(node->GroupID())) {
		// user is in owning group
		permissions = groupPermissions;
	} else {
		// user is one of the others
		permissions = otherPermissions;
	}

	return (mode & ~permissions) == 0 ? B_OK : B_NOT_ALLOWED;
}


//	#pragma mark - Volume


static status_t
packagefs_mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* parameters, ino_t* _rootID)
{
	FUNCTION("fsVolume: %p, device: \"%s\", flags: %#lx, parameters: \"%s\"\n",
		fsVolume, device, flags, parameters);

	// create a Volume object
	Volume* volume = new(std::nothrow) Volume(fsVolume);
	if (volume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);

	// Initialize the fs_volume now already, so it is mostly usable in during
	// mounting.
	fsVolume->private_volume = volumeDeleter.Detach();
	fsVolume->ops = &gPackageFSVolumeOps;

	status_t error = volume->Mount(parameters);
	if (error != B_OK)
		return error;

	// set return values
	*_rootID = volume->RootDirectory()->ID();

	return B_OK;
}


static status_t
packagefs_unmount(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p\n", volume);

	volume->Unmount();
	delete volume;

	return B_OK;
}


static status_t
packagefs_read_fs_info(fs_volume* fsVolume, struct fs_info* info)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p, info: %p\n", volume, info);

	info->flags = B_FS_IS_READONLY;
	info->block_size = 4096;
	info->io_size = kOptimalIOSize;
	info->total_blocks = info->free_blocks = 1;
	strlcpy(info->volume_name, volume->RootDirectory()->Name(),
		sizeof(info->volume_name));
	return B_OK;
}


// #pragma mark - VNodes


static status_t
packagefs_lookup(fs_volume* fsVolume, fs_vnode* fsDir, const char* entryName,
	ino_t* _vnid)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* dir = (Node*)fsDir->private_node;

	FUNCTION("volume: %p, dir: %p (%lld), entry: \"%s\"\n", volume, dir,
		dir->ID(), entryName);

	if (!S_ISDIR(dir->Mode()))
		return B_NOT_A_DIRECTORY;

	// resolve "."
	if (strcmp(entryName, ".") == 0) {
		Node* node;
		*_vnid = dir->ID();
		return volume->GetVNode(*_vnid, node);
	}

	// resolve ".."
	if (strcmp(entryName, "..") == 0) {
		Node* node;
		*_vnid = dir->Parent()->ID();
		return volume->GetVNode(*_vnid, node);
	}

	// resolve normal entries -- look up the node
	NodeReadLocker dirLocker(dir);
	Node* node = dynamic_cast<Directory*>(dir)->FindChild(entryName);
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;
	BReference<Node> nodeReference(node);
	dirLocker.Unlock();

	// get the vnode reference
	*_vnid = node->ID();
	RETURN_ERROR(volume->GetVNode(*_vnid, node));
}


static status_t
packagefs_get_vnode(fs_volume* fsVolume, ino_t vnid, fs_vnode* fsNode,
	int* _type, uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p, vnid: %lld\n", volume, vnid);

	VolumeReadLocker volumeLocker(volume);
	Node* node = volume->FindNode(vnid);
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;
	BReference<Node> nodeReference(node);
	volumeLocker.Unlock();

	NodeWriteLocker nodeLocker(node);
	status_t error = node->VFSInit(volume->ID());
	if (error != B_OK)
		RETURN_ERROR(error);
	nodeLocker.Unlock();

	fsNode->private_node = nodeReference.Detach();
	fsNode->ops = &gPackageFSVnodeOps;
	*_type = node->Mode() & S_IFMT;
	*_flags = 0;

	return B_OK;
}


static status_t
packagefs_put_vnode(fs_volume* fsVolume, fs_vnode* fsNode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p\n", volume, node);
	TOUCH(volume);

	NodeWriteLocker nodeLocker(node);
	node->VFSUninit();
	nodeLocker.Unlock();

	node->ReleaseReference();

	return B_OK;
}


// #pragma mark - Request I/O


static status_t
packagefs_io(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	io_request* request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p, request: %p\n", volume,
		node, node->ID(), cookie, request);
	TOUCH(volume);

	if (io_request_is_write(request))
		RETURN_ERROR(B_READ_ONLY_DEVICE);

	status_t error = node->Read(request);
	notify_io_request(request, error);
	return error;
}


// #pragma mark - Nodes


static status_t
packagefs_read_symlink(fs_volume* fsVolume, fs_vnode* fsNode, char* buffer,
	size_t* _bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());
	TOUCH(volume);

	NodeReadLocker nodeLocker(node);

	if (!S_ISLNK(node->Mode()))
		return B_BAD_VALUE;

	const char* linkPath = dynamic_cast<LeafNode*>(node)->SymlinkPath();
	if (linkPath == NULL) {
		*_bufferSize = 0;
		return B_OK;
	}

	size_t toCopy = std::min(strlen(linkPath), *_bufferSize);
	memcpy(buffer, linkPath, toCopy);
	*_bufferSize = toCopy;

	return B_OK;
}


static status_t
packagefs_access(fs_volume* fsVolume, fs_vnode* fsNode, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());
	TOUCH(volume);

	NodeReadLocker nodeLocker(node);
	return check_access(node, mode);
}


static status_t
packagefs_read_stat(fs_volume* fsVolume, fs_vnode* fsNode, struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());
	TOUCH(volume);

	NodeReadLocker nodeLocker(node);

	st->st_mode = node->Mode();
	st->st_nlink = 1;
	st->st_uid = node->UserID();
	st->st_gid = node->GroupID();
	st->st_size = node->FileSize();
	st->st_blksize = kOptimalIOSize;
	st->st_mtim = node->ModifiedTime();
	st->st_atim = st->st_mtim;
	st->st_ctim = st->st_mtim;
		// TODO: Perhaps manage a changed time (particularly for directories)?
	st->st_crtim = st->st_mtim;
	st->st_blocks = (st->st_size + 511) / 512;

	return B_OK;
}


// #pragma mark - Files


struct FileCookie {
	int	openMode;

	FileCookie(int openMode)
		:
		openMode(openMode)
	{
	}
};


static status_t
packagefs_open(fs_volume* fsVolume, fs_vnode* fsNode, int openMode,
	void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld), openMode %#x\n", volume, node,
		node->ID(), openMode);
	TOUCH(volume);

	NodeReadLocker nodeLocker(node);

	// check the open mode and permissions
	if (S_ISDIR(node->Mode()) && (openMode & O_RWMASK) != O_RDONLY)
		return B_IS_A_DIRECTORY;

	if ((openMode & O_RWMASK) != O_RDONLY)
		return B_NOT_ALLOWED;

	status_t error = check_access(node, R_OK);
	if (error != B_OK)
		return error;

	// allocate the cookie
	FileCookie* cookie = new(std::nothrow) FileCookie(openMode);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = cookie;

	return B_OK;
}


static status_t
packagefs_close(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_OK;
}


static status_t
packagefs_free_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	delete cookie;

	return B_OK;
}


static status_t
packagefs_read(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie,
	off_t offset, void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	FileCookie* cookie = (FileCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p, offset: %lld, "
		"buffer: %p, size: %lu\n", volume, node, node->ID(), cookie, offset,
		buffer, *bufferSize);
	TOUCH(volume);

	if ((cookie->openMode & O_RWMASK) != O_RDONLY)
		return EBADF;

	return node->Read(offset, buffer, bufferSize);
}


// #pragma mark - Directories


struct DirectoryCookie : DirectoryIterator {
	Directory*	directory;
	int32		state;
	bool		registered;

	DirectoryCookie(Directory* directory)
		:
		directory(directory),
		state(0),
		registered(false)
	{
		Rewind();
	}

	~DirectoryCookie()
	{
		if (registered)
			directory->RemoveDirectoryIterator(this);
	}

	void Rewind()
	{
		if (registered)
			directory->RemoveDirectoryIterator(this);
		registered = false;

		state = 0;
		node = directory;
	}

	Node* Current(const char*& _name) const
	{
		if (node == NULL)
			return NULL;

		if (state == 0)
			_name = ".";
		else if (state == 1)
			_name = "..";
		else
			_name = node->Name();

		return node;
	}

	Node* Next()
	{
		if (state == 0) {
			state = 1;
			node = directory->Parent();
			if (node == NULL)
				node = directory;
			return node;
		}

		if (state == 1) {
			node = directory->FirstChild();
			state = 2;
		} else {
			if (node != NULL)
				node = directory->NextChild(node);
		}

		if (node == NULL) {
			if (registered) {
				directory->RemoveDirectoryIterator(this);
				registered = false;
			}

			return NULL;
		}

		if (!registered) {
			directory->AddDirectoryIterator(this);
			registered = true;
		}

		return node;
	}
};


static status_t
packagefs_open_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());
	TOUCH(volume);

	if (!S_ISDIR(node->Mode()))
		return B_NOT_A_DIRECTORY;

	Directory* dir = dynamic_cast<Directory*>(node);

	status_t error = check_access(dir, R_OK);
	if (error != B_OK)
		return error;

	// create a cookie
	NodeWriteLocker dirLocker(dir);
	DirectoryCookie* cookie = new(std::nothrow) DirectoryCookie(dir);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = cookie;
	return B_OK;
}


static status_t
packagefs_close_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	return B_OK;
}


static status_t
packagefs_free_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	DirectoryCookie* cookie = (DirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	NodeWriteLocker dirLocker(node);
	delete cookie;

	return B_OK;
}


static status_t
packagefs_read_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	DirectoryCookie* cookie = (DirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	NodeWriteLocker dirLocker(cookie->directory);

	uint32 maxCount = *_count;
	uint32 count = 0;

	dirent* previousEntry = NULL;

	const char* name;
	while (Node* child = cookie->Current(name)) {
		// don't read more entries than requested
		if (count >= maxCount)
			break;

		// align the buffer for subsequent entries
		if (count > 0) {
			addr_t offset = (addr_t)buffer % 8;
			if (offset > 0) {
				offset = 8 - offset;
				if (bufferSize <= offset)
					break;

				previousEntry->d_reclen += offset;
				buffer = (dirent*)((addr_t)buffer + offset);
				bufferSize -= offset;
			}
		}

		// fill in the entry name -- checks whether the entry fits into the
		// buffer
		if (!set_dirent_name(buffer, bufferSize, name, strlen(name))) {
			if (count == 0)
				RETURN_ERROR(B_BUFFER_OVERFLOW);
			break;
		}

		// fill in the other data
		buffer->d_dev = volume->ID();
		buffer->d_ino = child->ID();

		count++;
		previousEntry = buffer;
		bufferSize -= buffer->d_reclen;
		buffer = (dirent*)((addr_t)buffer + buffer->d_reclen);

		cookie->Next();
	}

	*_count = count;
	return B_OK;
}


static status_t
packagefs_rewind_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	DirectoryCookie* cookie = (DirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	NodeWriteLocker dirLocker(node);
	cookie->Rewind();

	return B_OK;
}


// #pragma mark - Attribute Directories


struct AttributeDirectoryCookie {
	Node*					node;
	PackageNode*			packageNode;
	PackageNodeAttribute*	attribute;

	AttributeDirectoryCookie(Node* node)
		:
		node(node),
		packageNode(node->GetPackageNode()),
		attribute(NULL)
	{
		if (packageNode != NULL) {
			packageNode->AcquireReference();
			attribute = packageNode->Attributes().Head();
		}
	}

	~AttributeDirectoryCookie()
	{
		if (packageNode != NULL)
			packageNode->ReleaseReference();
	}

	PackageNodeAttribute* Current() const
	{
		return attribute;
	}

	void Next()
	{
		if (attribute == NULL)
			return;

		attribute = packageNode->Attributes().GetNext(attribute);
	}

	void Rewind()
	{
		if (packageNode != NULL)
			attribute = packageNode->Attributes().Head();
	}
};


status_t
packagefs_open_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());
	TOUCH(volume);

	status_t error = check_access(node, R_OK);
	if (error != B_OK)
		return error;

	// create a cookie
	NodeReadLocker nodeLocker(node);
	AttributeDirectoryCookie* cookie
		= new(std::nothrow) AttributeDirectoryCookie(node);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = cookie;
	return B_OK;
}


status_t
packagefs_close_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	return B_OK;
}


status_t
packagefs_free_attr_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode,
	void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeDirectoryCookie* cookie = (AttributeDirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	delete cookie;

	return B_OK;
}


status_t
packagefs_read_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeDirectoryCookie* cookie = (AttributeDirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	uint32 maxCount = *_count;
	uint32 count = 0;

	dirent* previousEntry = NULL;

	while (PackageNodeAttribute* attribute = cookie->Current()) {
		// don't read more entries than requested
		if (count >= maxCount)
			break;

		// align the buffer for subsequent entries
		if (count > 0) {
			addr_t offset = (addr_t)buffer % 8;
			if (offset > 0) {
				offset = 8 - offset;
				if (bufferSize <= offset)
					break;

				previousEntry->d_reclen += offset;
				buffer = (dirent*)((addr_t)buffer + offset);
				bufferSize -= offset;
			}
		}

		// fill in the entry name -- checks whether the entry fits into the
		// buffer
		const char* name = attribute->Name();
		if (!set_dirent_name(buffer, bufferSize, name, strlen(name))) {
			if (count == 0)
				RETURN_ERROR(B_BUFFER_OVERFLOW);
			break;
		}

		// fill in the other data
		buffer->d_dev = volume->ID();
		buffer->d_ino = node->ID();

		count++;
		previousEntry = buffer;
		bufferSize -= buffer->d_reclen;
		buffer = (dirent*)((addr_t)buffer + buffer->d_reclen);

		cookie->Next();
	}

	*_count = count;
	return B_OK;
}


status_t
packagefs_rewind_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeDirectoryCookie* cookie = (AttributeDirectoryCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	cookie->Rewind();

	return B_OK;
}


// #pragma mark - Attribute Operations


struct AttributeCookie {
	PackageNode*			packageNode;
	Package*				package;
	PackageNodeAttribute*	attribute;
	int						openMode;

	AttributeCookie(PackageNode* packageNode, PackageNodeAttribute* attribute,
		int openMode)
		:
		packageNode(packageNode),
		package(packageNode->GetPackage()),
		attribute(attribute),
		openMode(openMode)
	{
		packageNode->AcquireReference();
		package->AcquireReference();
	}

	~AttributeCookie()
	{
		packageNode->ReleaseReference();
		package->ReleaseReference();
	}
};


status_t
packagefs_open_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	int openMode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld), name: \"%s\", openMode %#x\n",
		volume, node, node->ID(), name, openMode);
	TOUCH(volume);

	NodeReadLocker nodeLocker(node);

	// check the open mode and permissions
	if ((openMode & O_RWMASK) != O_RDONLY)
		return B_NOT_ALLOWED;

	status_t error = check_access(node, R_OK);
	if (error != B_OK)
		return error;

	// get the package node and the respectively named attribute
	PackageNode* packageNode = node->GetPackageNode();
	PackageNodeAttribute* attribute = packageNode != NULL
		? packageNode->FindAttribute(name) : NULL;
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	// allocate the cookie
	AttributeCookie* cookie = new(std::nothrow) AttributeCookie(packageNode,
		attribute, openMode);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = cookie;

	return B_OK;
}


status_t
packagefs_close_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	return B_OK;
}


status_t
packagefs_free_attr_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	delete cookie;

	return B_OK;
}


static status_t
read_package_data(const BPackageData& data, BDataReader* dataReader, off_t offset,
	void* buffer, size_t* bufferSize)
{
	// create a BPackageDataReader
	BPackageDataReader* reader;
	status_t error = GlobalFactory::Default()->CreatePackageDataReader(
		dataReader, data, reader);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<BPackageDataReader> readerDeleter(reader);

	// check the offset
	if (offset < 0 || (uint64)offset > data.UncompressedSize())
		return B_BAD_VALUE;

	// clamp the size
	size_t toRead = std::min((uint64)*bufferSize,
		data.UncompressedSize() - offset);

	// read
	if (toRead > 0) {
		status_t error = reader->ReadData(offset, buffer, toRead);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	*bufferSize = toRead;
	return B_OK;
}


status_t
packagefs_read_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* _cookie,
	off_t offset, void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	const BPackageData& data = cookie->attribute->Data();
	if (data.IsEncodedInline()) {
		// inline data
		BBufferDataReader dataReader(data.InlineData(), data.CompressedSize());
		return read_package_data(data, &dataReader, offset, buffer, bufferSize);
	}

	// data not inline -- open the package
	int fd = cookie->package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(cookie->package);

	BFDDataReader dataReader(fd);
	return read_package_data(data, &dataReader, offset, buffer, bufferSize);
}


status_t
packagefs_read_attr_stat(fs_volume* fsVolume, fs_vnode* fsNode,
	void* _cookie, struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	FUNCTION("volume: %p, node: %p (%lld), cookie: %p\n", volume, node,
		node->ID(), cookie);
	TOUCH(volume);
	TOUCH(node);

	st->st_size = cookie->attribute->Data().UncompressedSize();
	st->st_type = cookie->attribute->Type();

	return B_OK;
}


// #pragma mark - Module Interface


static status_t
packagefs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			init_debugging();
			PRINT("package_std_ops(): B_MODULE_INIT\n");

			status_t error = GlobalFactory::CreateDefault();
			if (error != B_OK) {
				ERROR("Failed to init GlobalFactory\n");
				exit_debugging();
				return error;
			}

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			PRINT("package_std_ops(): B_MODULE_UNINIT\n");
			GlobalFactory::DeleteDefault();
			exit_debugging();
			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


static file_system_module_info sPackageFSModuleInfo = {
	{
		"file_systems/packagefs" B_CURRENT_FS_API_VERSION,
		0,
		packagefs_std_ops,
	},

	"packagefs",				// short_name
	"Package File System",		// pretty_name
	0,							// DDM flags


	// scanning
	NULL,	// identify_partition,
	NULL,	// scan_partition,
	NULL,	// free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&packagefs_mount
};


fs_volume_ops gPackageFSVolumeOps = {
	&packagefs_unmount,
	&packagefs_read_fs_info,
	NULL,	// write_fs_info,
	NULL,	// sync,

	&packagefs_get_vnode

	// TODO: index operations
	// TODO: query operations
	// TODO: FS layer operations
};


fs_vnode_ops gPackageFSVnodeOps = {
	// vnode operations
	&packagefs_lookup,
	NULL,	// get_vnode_name,
	&packagefs_put_vnode,
	&packagefs_put_vnode,	// remove_vnode -- same as put_vnode

	// VM file access
	NULL,	// can_page,
	NULL,	// read_pages,
	NULL,	// write_pages,

	&packagefs_io,
	NULL,	// cancel_io()

	NULL,	// get_file_map,

	NULL,	// ioctl,
	NULL,	// set_flags,
	NULL,	// select,
	NULL,	// deselect,
	NULL,	// fsync,

	&packagefs_read_symlink,
	NULL,	// create_symlink,

	NULL,	// link,
	NULL,	// unlink,
	NULL,	// rename,

	&packagefs_access,
	&packagefs_read_stat,
	NULL,	// write_stat,
	NULL,	// preallocate,

	// file operations
	NULL,	// create,
	&packagefs_open,
	&packagefs_close,
	&packagefs_free_cookie,
	&packagefs_read,
	NULL,	// write,

	// directory operations
	NULL,	// create_dir,
	NULL,	// remove_dir,
	&packagefs_open_dir,
	&packagefs_close_dir,
	&packagefs_free_dir_cookie,
	&packagefs_read_dir,
	&packagefs_rewind_dir,

	// attribute directory operations
	&packagefs_open_attr_dir,
	&packagefs_close_attr_dir,
	&packagefs_free_attr_dir_cookie,
	&packagefs_read_attr_dir,
	&packagefs_rewind_attr_dir,

	// attribute operations
	NULL,	// create_attr,
	&packagefs_open_attr,
	&packagefs_close_attr,
	&packagefs_free_attr_cookie,
	&packagefs_read_attr,
	NULL,	// write_attr,

	&packagefs_read_attr_stat,
	NULL,	// write_attr_stat,
	NULL,	// rename_attr,
	NULL	// remove_attr,

	// TODO: FS layer operations
};


module_info *modules[] = {
	(module_info *)&sPackageFSModuleInfo,
	NULL,
};
