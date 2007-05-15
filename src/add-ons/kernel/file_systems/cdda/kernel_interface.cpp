/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "cdda.h"
#include "Lock.h"

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include <util/kernel_cpp.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


//#define TRACE_CDDA
#ifdef TRACE_CDDA
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


class Volume;
class Inode;
struct dir_cookie;

class Volume {
	public:
		Volume(mount_id id);
		~Volume();

		status_t		InitCheck();
		mount_id		ID() const { return fID; }
		Inode			&RootNode() const { return *fRootNode; }

		status_t		Mount(const char* device);
		int				Device() const { return fDevice; }
		vnode_id		GetNextNodeID() { return fNextID++; }

		const char		*Name() const { return fName; }
		status_t		SetName(const char *name);

		Semaphore		&Lock();

		Inode			*Find(vnode_id id);
		Inode			*Find(const char *name);

		Inode			*FirstEntry() const { return fFirstEntry; }
		off_t			NumBlocks() const { return fNumBlocks; }

	private:
		Inode			*_CreateNode(Inode *parent, const char *name,
							off_t start, off_t frames, int32 type);

		Semaphore		fLock;
		int				fDevice;
		mount_id		fID;
		Inode 			*fRootNode;
		vnode_id		fNextID;
		char			*fName;
		off_t			fNumBlocks;

		// root directory contents - we don't support other directories
		Inode			*fFirstEntry;
};


class Inode {
	public:
		Inode(Volume *volume, Inode *parent, const char *name, off_t start,
			off_t frames, int32 type);
		~Inode();

		status_t	InitCheck();
		vnode_id	ID() const { return fID; }

		const char	*Name() const { return fName; }
		status_t	SetName(const char* name);

		int32		Type() const
						{ return fType; }
		gid_t		GroupID() const
						{ return fGroupID; }
		uid_t		UserID() const
						{ return fUserID; }
		time_t		CreationTime() const
						{ return fCreationTime; }
		time_t		ModificationTime() const
						{ return fModificationTime; }
		off_t		StartFrame() const
						{ return fStartFrame; }
		off_t		FrameCount() const
						{ return fFrameCount; }
		off_t		Size() const
						{ return fFrameCount * kFrameSize /* + WAV header */; }

		Inode		*Next() const { return fNext; }
		void		SetNext(Inode *inode) { fNext = inode; }

	private:
		Inode		*fNext;
		vnode_id	fID;
		int32		fType;
		char		*fName;
		gid_t		fGroupID;
		uid_t		fUserID;
		time_t		fCreationTime;
		time_t		fModificationTime;
		off_t		fStartFrame;
		off_t		fFrameCount;
};


struct dir_cookie {
	Inode	*current;
	int		state;	// iteration state
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

struct file_cookie {
	int		open_mode;
};


Volume::Volume(mount_id id)
	:
	fLock("cdda"),
	fDevice(-1),
	fID(id),
	fRootNode(NULL),
	fNextID(1),
	fName(NULL),
	fNumBlocks(0),
	fFirstEntry(NULL)
{
	// create the root vnode
	fRootNode = _CreateNode(NULL, "", 0, 0, S_IFDIR | 0777);
}


Volume::~Volume()
{
	close(fDevice);

	// put_vnode on the root to release the ref to it
	if (fRootNode)
		put_vnode(ID(), fRootNode->ID());

	Inode *inode, *next;

	for (inode = fFirstEntry; inode != NULL; inode = next) {
		next = inode->Next();
		delete inode;
	}

	free(fName);
}


status_t 
Volume::InitCheck()
{
	if (fLock.InitCheck() < B_OK
		|| fRootNode == NULL)
		return B_ERROR;

	return B_OK;
}


status_t
Volume::Mount(const char* device)
{
	fDevice = open(device, O_RDONLY);
	if (fDevice < 0)
		return errno;

	scsi_toc_toc *toc = (scsi_toc_toc *)malloc(1024);
	if (toc == NULL)
		return B_NO_MEMORY;

	status_t status = read_table_of_contents(fDevice, toc, 1024);
	if (status < B_OK) {
		free(toc);
		return status;
	}

	cdtext text;
	if (read_cdtext(fDevice, text) < B_OK)
		dprintf("CDDA: no CD-Text found.\n");

	int32 trackCount = toc->last_track + 1 - toc->first_track;
	char title[256];

	for (int32 i = 0; i < trackCount; i++) {
		scsi_cd_msf& next = toc->tracks[i + 1].start.time;
			// the last track is always lead-out
		scsi_cd_msf& start = toc->tracks[i].start.time;

		off_t startFrame = start.minute * kFramesPerMinute
			+ start.second * kFramesPerSecond + start.frame;
		off_t frames = next.minute * kFramesPerMinute
			+ next.second * kFramesPerSecond + next.frame
			- startFrame;

		if (text.titles[i] != NULL && text.titles[i]) {
			if (text.artists[i] != NULL && text.artists[i]) {
				snprintf(title, sizeof(title), "%02ld. %s - %s.wav", i + 1,
					text.artists[i], text.titles[i]);
			} else {
				snprintf(title, sizeof(title), "%02ld. %s.wav", i + 1,
					text.titles[i]);
			}
		} else
			snprintf(title, sizeof(title), "%02ld.wav", i + 1);

		_CreateNode(fRootNode, title, startFrame, frames, S_IFREG | 0444);
	}

	free(toc);

	// determine volume title

	if (text.artist != NULL && text.album != NULL)
		snprintf(title, sizeof(title), "%s - %s", text.artist, text.album);
	else if (text.artist != NULL || text.album != NULL) {
		snprintf(title, sizeof(title), "%s", text.artist != NULL
			? text.artist : text.album);
	} else
		strcpy(title, "Audio CD");

	fName = strdup(title);
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


Semaphore&
Volume::Lock()
{
	return fLock;
}


Inode *
Volume::_CreateNode(Inode *parent, const char *name, off_t start, off_t frames,
	int32 type)
{
	Inode *inode = new Inode(this, parent, name, start, frames, type);
	if (inode == NULL)
		return NULL;

	if (inode->InitCheck() != B_OK) {
		delete inode;
		return NULL;
	}

	if (S_ISREG(type)) {
		inode->SetNext(fFirstEntry);
		fFirstEntry = inode;
	}

	publish_vnode(ID(), inode->ID(), inode);
	return inode;
}


Inode *
Volume::Find(vnode_id id)
{
	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		if (inode->ID() == id)
			return inode;
	}

	return NULL;
}


Inode *
Volume::Find(const char *name)
{
	if (!strcmp(name, ".")
		|| !strcmp(name, ".."))
		return fRootNode;

	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		if (!strcmp(inode->Name(), name))
			return inode;
	}

	return NULL;
}


status_t
Volume::SetName(const char *name)
{
	if (name == NULL || !name[0])
		return B_BAD_VALUE;

	name = strdup(name);
	if (name == NULL)
		return B_NO_MEMORY;

	free(fName);
	fName = (char *)name;
	return B_OK;
}


//	#pragma mark -


Inode::Inode(Volume *volume, Inode *parent, const char *name, off_t start,
		off_t frames, int32 type)
	:
	fNext(NULL)
{
	fName = strdup(name);
	if (fName == NULL)
		return;

	fID = volume->GetNextNodeID();
	fType = type;
	fStartFrame = start;
	fFrameCount = frames;

	fUserID = geteuid();
	fGroupID = parent ? parent->GroupID() : getegid();

	fCreationTime = fModificationTime = time(NULL);
}


Inode::~Inode()
{
	free(const_cast<char *>(fName));
}


status_t 
Inode::InitCheck()
{
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
Inode::SetName(const char* name)
{
	if (name == NULL || !name[0])
		return B_BAD_VALUE;

	name = strdup(name);
	if (name == NULL)
		return B_NO_MEMORY;

	free(fName);
	fName = (char *)name;
	return B_OK;
}


//	#pragma mark - module API


static status_t
cdda_mount(mount_id id, const char *device, uint32 flags, const char *args,
	fs_volume *_volume, vnode_id *_rootVnodeID)
{
	TRACE(("cdda_mount: entry\n"));

	Volume *volume = new Volume(id);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status = volume->InitCheck();
	if (status == B_OK)
		status = volume->Mount(device);

	if (status < B_OK) {
		delete volume;
		return status;
	}

	*_rootVnodeID = volume->RootNode().ID();
	*_volume = volume;

	return B_OK;
}


static status_t
cdda_unmount(fs_volume _volume)
{
	struct Volume *volume = (struct Volume *)_volume;

	TRACE(("cdda_unmount: entry fs = %p\n", _volume));
	delete volume;

	return 0;
}


static status_t
cdda_read_fs_stat(fs_volume _volume, struct fs_info *info)
{
	Volume *volume = (Volume *)_volume;
	Locker locker(volume->Lock());

	// File system flags.
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME
		| B_FS_IS_REMOVABLE;
	info->io_size = 65536;

	info->block_size = 2048;
	info->total_blocks = volume->NumBlocks();
	info->free_blocks = 0;

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "cdda", sizeof(info->fsh_name));

	return B_OK;
}


static status_t
cdda_write_fs_stat(fs_volume _volume, const struct fs_info *info, uint32 mask)
{
	Volume *volume = (Volume *)_volume;
	Locker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & FS_WRITE_FSINFO_NAME)
		status = volume->SetName(info->volume_name);

	return status;
}


static status_t
cdda_sync(fs_volume fs)
{
	TRACE(("cdda_sync: entry\n"));

	return 0;
}


static status_t
cdda_lookup(fs_volume _volume, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	Volume *volume = (Volume *)_volume;
	status_t status;

	TRACE(("cdda_lookup: entry dir %p, name '%s'\n", _dir, name));

	Inode *directory = (Inode *)_dir;
	if (!S_ISDIR(directory->Type()))
		return B_NOT_A_DIRECTORY;

	Locker _(volume->Lock());

	Inode *inode = volume->Find(name);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	Inode *dummy;
	status = get_vnode(volume->ID(), inode->ID(), (fs_vnode *)&dummy);
	if (status < B_OK)
		return status;

	*_id = inode->ID();
	*_type = inode->Type();
	return B_OK;
}


static status_t
cdda_get_vnode_name(fs_volume _volume, fs_vnode _node, char *buffer, size_t bufferSize)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_get_vnode_name(): inode = %p\n", inode));

	Locker _(volume->Lock());
	strlcpy(buffer, inode->Name(), bufferSize);
	return B_OK;
}


static status_t
cdda_get_vnode(fs_volume _volume, vnode_id id, fs_vnode *_inode, bool reenter)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode;

	TRACE(("cdda_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	inode = volume->Find(id);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	*_inode = inode;
	return B_OK;
}


static status_t
cdda_put_vnode(fs_volume _volume, fs_vnode _node, bool reenter)
{
	return B_OK;
}


static status_t
cdda_open(fs_volume _volume, fs_vnode _node, int openMode, fs_cookie *_cookie)
{
	TRACE(("cdda_open(): node = %p, openMode = %d\n", _node, openMode));

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	TRACE(("  open cookie = %p\n", cookie));
	cookie->open_mode = openMode;

	*_cookie = (void *)cookie;

	return B_OK;
}


static status_t
cdda_close(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	return B_OK;
}


static status_t
cdda_free_cookie(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	file_cookie *cookie = (file_cookie *)_cookie;

	TRACE(("cdda_freecookie: entry vnode %p, cookie %p\n", _node, _cookie));

	free(cookie);
	return B_OK;
}


static status_t
cdda_fsync(fs_volume _volume, fs_vnode _v)
{
	return B_OK;
}


static status_t
cdda_read(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t offset,
	void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_read(vnode = %p, offset %Ld, length = %lu, mode = %d)\n",
		_node, offset, *_length, cookie->open_mode));

	if (S_ISDIR(inode->Type()))
		return B_IS_A_DIRECTORY;
	if ((cookie->open_mode & O_RWMASK) != O_RDONLY)
		return B_NOT_ALLOWED;

	// TODO: read!
	*_length = 0;
	return B_OK;
}


static bool
cdda_can_page(fs_volume _volume, fs_vnode _v, fs_cookie cookie)
{
	return false;
}


static status_t
cdda_read_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return EPERM;
}


static status_t
cdda_write_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return EPERM;
}


static status_t
cdda_read_stat(fs_volume _volume, fs_vnode _node, struct stat *stat)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_read_stat: vnode %p (0x%Lx), stat %p\n", inode, inode->ID(), stat));

	stat->st_dev = volume->ID();
	stat->st_ino = inode->ID();

	stat->st_size = inode->Size();
	stat->st_mode = inode->Type();

	stat->st_nlink = 1;
	stat->st_blksize = 2048;

	stat->st_uid = inode->UserID();
	stat->st_gid = inode->GroupID();

	stat->st_atime = time(NULL);
	stat->st_mtime = stat->st_ctime = inode->ModificationTime();
	stat->st_crtime = inode->CreationTime();

	return B_OK;
}


status_t 
cdda_rename(fs_volume _volume, void *_oldDir, const char *oldName, void *_newDir,
	const char *newName)
{
	if (_volume == NULL || _oldDir == NULL || _newDir == NULL
		|| oldName == NULL || *oldName == '\0'
		|| newName == NULL || *newName == '\0'
		|| !strcmp(oldName, ".") || !strcmp(oldName, "..")
		|| !strcmp(newName, ".") || !strcmp(newName, "..")
		|| strchr(newName, '/') != NULL)
		return B_BAD_VALUE;

	// we only have a single directory which simplifies things a bit :-)

	Volume *volume = (Volume *)_volume;
	Locker _(volume->Lock());

	Inode *inode = volume->Find(oldName);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	if (volume->Find(newName) != NULL)
		return B_NAME_IN_USE;

	return inode->SetName(newName);
}


//	#pragma mark - directory functions


static status_t
cdda_open_dir(fs_volume _volume, fs_vnode _node, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("cdda_open_dir(): vnode = %p\n", _node));

	Inode *inode = (Inode *)_node;
	if (!S_ISDIR(inode->Type()))
		return B_BAD_VALUE;

	if (inode != &volume->RootNode())
		panic("pipefs: found directory that's not the root!");

	dir_cookie *cookie = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->current = volume->FirstEntry();
	cookie->state = ITERATION_STATE_BEGIN;

	*_cookie = (void *)cookie;
	return B_OK;
}


static status_t
cdda_read_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;
	status_t status = 0;

	TRACE(("cdda_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _node, _cookie, dirent, bufferSize,_num));

	if (_node != &volume->RootNode())
		return B_BAD_VALUE;

	Locker _(volume->Lock());

	dir_cookie *cookie = (dir_cookie *)_cookie;
	Inode *childNode = NULL;
	const char *name = NULL;
	Inode *nextChildNode = NULL;
	int nextState = cookie->state;

	switch (cookie->state) {
		case ITERATION_STATE_DOT:
			childNode = inode;
			name = ".";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = inode; // parent of the root node is the root node
			name = "..";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		default:
			childNode = cookie->current;
			if (childNode) {
				name = childNode->Name();
				nextChildNode = childNode->Next();
			}
			break;
	}

	if (!childNode) {
		// we're at the end of the directory
		*_num = 0;
		return B_OK;
	}

	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = strlen(name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize)
		return ENOBUFS;

	status = user_strlcpy(dirent->d_name, name, bufferSize);
	if (status < B_OK)
		return status;

	cookie->current = nextChildNode;
	cookie->state = nextState;
	return B_OK;
}


static status_t
cdda_rewind_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	Volume *volume = (Volume *)_volume;

	dir_cookie *cookie = (dir_cookie *)_cookie;
	cookie->current = volume->FirstEntry();
	cookie->state = ITERATION_STATE_BEGIN;

	return B_OK;
}


static status_t
cdda_close_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	TRACE(("cdda_close: entry vnode %p, cookie %p\n", _node, _cookie));

	return 0;
}


static status_t
cdda_free_dir_cookie(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;

	TRACE(("cdda_freecookie: entry vnode %p, cookie %p\n", _vnode, cookie));

	free(cookie);
	return 0;
}


static status_t
cdda_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


static file_system_module_info sCDDAFileSystem = {
	{
		"file_systems/cdda" B_CURRENT_FS_API_VERSION,
		0,
		cdda_std_ops,
	},

	"CDDA File System",

	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&cdda_mount,
	&cdda_unmount,
	&cdda_read_fs_stat,
	&cdda_write_fs_stat,
	&cdda_sync,

	&cdda_lookup,
	&cdda_get_vnode_name,

	&cdda_get_vnode,
	&cdda_put_vnode,
	NULL,	// fs_remove_vnode()

	&cdda_can_page,
	&cdda_read_pages,
	&cdda_write_pages,

	NULL,	// get_file_map()

	// common
	NULL,	// fs_ioctl()
	NULL,	// fs_set_flags()
	NULL,	// fs_select()
	NULL,	// fs_deselect()
	&cdda_fsync,

	NULL,	// fs_read_link()
	NULL,	// fs_symlink()
	NULL,	// fs_link()
	NULL,	// fs_unlink()
	NULL,	// fs_rename()

	NULL,	// fs_access()
	&cdda_read_stat,
	NULL,	// fs_write_stat()

	// file
	NULL,	// fs_create()
	&cdda_open,
	&cdda_close,
	&cdda_free_cookie,
	&cdda_read,
	NULL,	// fs_write()

	// directory
	NULL,	// fs_create_dir()
	NULL,	// fs_remove_dir()
	&cdda_open_dir,
	&cdda_close_dir,
	&cdda_free_dir_cookie,
	&cdda_read_dir,
	&cdda_rewind_dir,

#if 0
	// attribute directory operations
	&cdda_open_attr_dir,
	&cdda_close_attr_dir,
	&cdda_free_attr_dir_cookie,
	&cdda_read_attr_dir,
	&cdda_rewind_attr_dir,

	// attribute operations
	&cdda_create_attr,
	&cdda_open_attr,
	&cdda_close_attr,
	&cdda_free_attr_cookie,
	&cdda_read_attr,
	&cdda_write_attr,

	&cdda_read_attr_stat,
	&cdda_write_attr_stat,
	&cdda_rename_attr,
	&cdda_remove_attr,
#endif

	// the other operations are not yet supported (indices, queries)
	NULL,
};

module_info *modules[] = {
	(module_info *)&sCDDAFileSystem,
	NULL,
};
