/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <string.h>
#include <sys/stat.h>

#include <KernelExport.h>
#include <Drivers.h>

#include <fsproto.h>
#include <lock.h>
#include <cache.h>
#include <time.h>

#include "iter.h"
#include "dosfs.h"
#include "dlist.h"
#include "fat.h"
#include "dir.h"
#include "file.h"
#include "attr.h"
#include "vcache.h"
#include "util.h"

#define DPRINTF(a,b) if (debug_file > (a)) dprintf b

#define MAX_FILE_SIZE 0xffffffffLL

#define FILECOOKIE_MAGIC 'looT'

typedef struct filecookie
{
	uint32		magic;
	uint32		mode;		// open mode

	/* simple cluster cache */
	struct {
		uint32		iteration;
		uint32		index;		/* index in the fat chain */
		uint32		cluster;
	} ccache;
} filecookie;

static CHECK_MAGIC(filecookie,struct filecookie, FILECOOKIE_MAGIC)

status_t write_vnode_entry(nspace *vol, vnode *node)
{
	uint32 i;
	struct diri diri;
	uint8 *buffer;

	// don't update entries of deleted files
	if (is_vnode_removed(vol->id, node->vnid) > 0) return 0;

	// XXX: should check if directory position is still valid even
	// though we do the is_vnode_removed check above

	if ((node->cluster != 0) && !IS_DATA_CLUSTER(node->cluster)) {
		dprintf("write_vnode_entry called on invalid cluster (%lx)\n", node->cluster);
		return EINVAL;
	}

	buffer = diri_init(vol, VNODE_PARENT_DIR_CLUSTER(node), node->eindex, &diri);
	if (buffer == NULL)
		return ENOENT;

	buffer[0x0b] = node->mode; // file attributes
	
	memset(buffer+0xc, 0, 0x16-0xc);
	i = time_t2dos(node->st_time);
	buffer[0x16] = i & 0xff;
	buffer[0x17] = (i >> 8) & 0xff;
	buffer[0x18] = (i >> 16) & 0xff;
	buffer[0x19] = (i >> 24) & 0xff;
	buffer[0x1a] = node->cluster & 0xff;	// starting cluster
	buffer[0x1b] = (node->cluster >> 8) & 0xff;
	if (vol->fat_bits == 32) {
		buffer[0x14] = (node->cluster >> 16) & 0xff;
		buffer[0x15] = (node->cluster >> 24) & 0xff;
	}
	if (node->mode & FAT_SUBDIR) {
		buffer[0x1c] = buffer[0x1d] = buffer[0x1e] = buffer[0x1f] = 0;
	} else {
		buffer[0x1c] = node->st_size & 0xff;	// file size
		buffer[0x1d] = (node->st_size >> 8) & 0xff;
		buffer[0x1e] = (node->st_size >> 16) & 0xff;
		buffer[0x1f] = (node->st_size >> 24) & 0xff;
	}

	diri_mark_dirty(&diri);
	diri_free(&diri);

	notify_listener(B_STAT_CHANGED, vol->id, 0, 0, node->vnid, NULL);

	return B_OK;
}

// called when fs is done with vnode
// after close, etc. free vnode resources here
int dosfs_write_vnode(void *_vol, void *_node, char reenter)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	TOUCH(reenter);

	if (check_nspace_magic(vol, "dosfs_write_vnode") ||
		check_vnode_magic(node, "dosfs_write_vnode")) {
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_write_vnode (vnode_id %Lx)\n", ((vnode *)_node)->vnid));
	
	if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && node->dirty) {
		LOCK_VOL(vol);
		_dosfs_sync(vol);
		UNLOCK_VOL(vol);
	}

	if (node != NULL) {
#if TRACK_FILENAME
		if (node->filename) free(node->filename);
#endif
		if (node->vnid != vol->root_vnode.vnid) {
			node->magic = ~VNODE_MAGIC; // munge magic number to be safe
			free(node);
		}
	}

	return 0;
}

int dosfs_rstat(void *_vol, void *_node, struct stat *st)
{
	nspace	*vol = (nspace*)_vol;
	vnode	*node = (vnode*)_node;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_rstat") ||
		check_vnode_magic(node, "dosfs_rstat")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(1, ("dosfs_rstat (vnode id %Lx)\n", node->vnid));

	st->st_dev = vol->id;
	st->st_ino = node->vnid;
	st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
	if (node->mode & FAT_SUBDIR) {
		st->st_mode &= ~S_IFREG;
		st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	}
	if ((vol->flags & B_FS_IS_READONLY) || (node->mode & FAT_READ_ONLY))
		st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);

	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_size = node->st_size;
	st->st_blksize = 0x10000; /* this value was chosen arbitrarily */
	st->st_atime = st->st_mtime = st->st_ctime = st->st_crtime = node->st_time;

	UNLOCK_VOL(vol);

	return B_NO_ERROR;
}

int dosfs_wstat(void *_vol, void *_node, struct stat *st, long mask)
{
	int err = B_OK;
	nspace	*vol = (nspace*)_vol;
	vnode	*node = (vnode*)_node;
	bool dirty = false;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_wstat") ||
		check_vnode_magic(node, "dosfs_wstat")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_wstat (vnode id %Lx)\n", node->vnid));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("can't wstat on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (node->disk_image == 2) {
		dprintf("can't wstat disk image\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	if (mask & WSTAT_MODE) {
		DPRINTF(0, ("setting file mode to %o\n", st->st_mode));
		if (st->st_mode & S_IWUSR)
			node->mode &= ~FAT_READ_ONLY;
		else
			node->mode |= FAT_READ_ONLY;
		dirty = true;
	}

	if (mask & WSTAT_SIZE) {
		DPRINTF(0, ("setting file size to %Lx\n", st->st_size));
		if (node->mode & FAT_SUBDIR) {
			dprintf("dosfs_wstat: can't set file size of directory!\n");
			err = EISDIR;
		} else if (st->st_size > MAX_FILE_SIZE) {
			dprintf("dosfs_wstat: desired file size exceeds fat limit\n");
			err = E2BIG;
		} else {
			uint32 clusters = (st->st_size + vol->bytes_per_sector*vol->sectors_per_cluster - 1) / vol->bytes_per_sector / vol->sectors_per_cluster;
			DPRINTF(0, ("setting fat chain length to %lx clusters\n", clusters));
			if ((err = set_fat_chain_length(vol, node, clusters)) == B_OK) {
				node->st_size = st->st_size;
				node->iteration++;
				dirty = true;
			}
		}
	}
	
	if (mask & WSTAT_MTIME) {
		DPRINTF(0, ("setting modification time\n"));
		node->st_time = st->st_mtime;
		dirty = true;
	}

	if (dirty) {
		write_vnode_entry(vol, node);
		notify_listener(B_STAT_CHANGED, vol->id, 0, 0, node->vnid, NULL);

		if (vol->fs_flags & FS_FLAGS_OP_SYNC) {
			// sync the filesystem
			_dosfs_sync(vol);
			node->dirty = false;
		}
	}

	if (err != B_OK) DPRINTF(0, ("dosfs_wstat (%s)\n", strerror(err)));

	UNLOCK_VOL(vol);
	
	return err;
}

int dosfs_open(void *_vol, void *_node, int omode, void **_cookie)
{
	int		result = EINVAL;
	nspace *vol = (nspace *)_vol;
	vnode* 	node = (vnode*)_node;
	filecookie *cookie;

	*_cookie = NULL;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_open") ||
		check_vnode_magic(node, "dosfs_open")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_open: vnode id %Lx, omode %x\n", node->vnid, omode));

	if (omode & O_CREAT) {
		dprintf("dosfs_open called with O_CREAT. call dosfs_create instead!\n");
		result = EINVAL;
		goto error;
	}

	if ((vol->flags & B_FS_IS_READONLY) || 
		(node->mode & FAT_READ_ONLY) ||
		(node->disk_image != 0) ||
		// allow opening directories for ioctl() calls
		// and to let BVolume to work
		(node->mode & FAT_SUBDIR)) {
		omode = (omode & ~O_RWMASK) | O_RDONLY;
	}

	if ((omode & O_TRUNC) && ((omode & O_RWMASK) == O_RDONLY)) {
		DPRINTF(0, ("can't open file for reading with O_TRUNC\n"));
		result = EPERM;
		goto error;
	}

	if (omode & O_TRUNC) {
		DPRINTF(0, ("dosfs_open called with O_TRUNC set\n"));
		if ((result = set_fat_chain_length(vol, node, 0)) != B_OK) {
			dprintf("dosfs_open: error truncating file\n");
			goto error;
		}
		node->mode = 0;
		node->st_size = 0;
		node->iteration++;
	}

	if ((cookie = calloc(sizeof(filecookie), 1)) == NULL) {
		result = ENOMEM;
		goto error;
	}

	cookie->magic = FILECOOKIE_MAGIC;
	cookie->mode = omode;
	cookie->ccache.iteration = node->iteration;
	cookie->ccache.index = 0;
	cookie->ccache.cluster = node->cluster;
	*_cookie = cookie;
	result = B_OK;

error:
	if (result != B_OK) DPRINTF(0, ("dosfs_open (%s)\n", strerror(result)));

	UNLOCK_VOL(vol);
	return result;
}

int dosfs_read(void *_vol, void *_node, void *_cookie, off_t pos,
			void *buf, size_t *len)
{
	nspace	*vol = (nspace *)_vol;
	vnode	*node = (vnode *)_node;
	filecookie *cookie = (filecookie *)_cookie;
	uint8 *buffer;
	struct csi iter;
	int result = B_OK;
	size_t bytes_read = 0;
	uint32 cluster1;
	off_t diff;

	LOCK_VOL(vol);

	if (check_nspace_magic((nspace *)vol, "dosfs_read") ||
		check_vnode_magic(node, "dosfs_read") ||
		check_filecookie_magic(cookie, "dosfs_read")) {
		*len = 0;
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if (node->mode & FAT_SUBDIR) {
		DPRINTF(0, ("dosfs_read called on subdirectory %Lx\n", node->vnid));
		*len = 0;
		UNLOCK_VOL(vol);
		return EISDIR;
	}

	DPRINTF(0, ("dosfs_read called %lx bytes at %Lx (vnode id %Lx)\n", *len, pos, node->vnid));

	if (pos < 0) pos = 0;

	if ((node->st_size == 0) || (*len == 0) || (pos >= node->st_size)) {
		bytes_read = 0;
		goto bi;
	}

	// truncate bytes to read to file size
	if (pos + *len >= node->st_size)
		*len = node->st_size - pos;

	if ((cookie->ccache.iteration == node->iteration) &&
		(pos >= cookie->ccache.index * vol->bytes_per_sector * vol->sectors_per_cluster)) {
		/* the cached fat value is both valid and helpful */
		ASSERT(IS_DATA_CLUSTER(cookie->ccache.cluster));
		ASSERT(cookie->ccache.cluster == get_nth_fat_entry(vol, node->cluster, cookie->ccache.index));
		cluster1 = cookie->ccache.cluster;
		diff = pos - cookie->ccache.index * vol->bytes_per_sector * vol->sectors_per_cluster;
	} else {
		/* the fat chain changed, so we have to start from the beginning */
		cluster1 = node->cluster;
		diff = pos;
	}
	diff /= vol->bytes_per_sector; /* convert to sectors */

	if ((result = init_csi(vol, cluster1, 0, &iter)) != B_OK) {
		dprintf("dosfs_read: invalid starting cluster (%lx)\n", cluster1);
		goto bi;
	}
	
	if (diff && ((result = iter_csi(&iter, diff)) != B_OK)) {
		dprintf("dosfs_read: end of file reached (init)\n");
		result = EIO;
		goto bi;
	}

	ASSERT(iter.cluster == get_nth_fat_entry(vol, node->cluster, pos / vol->bytes_per_sector / vol->sectors_per_cluster));

	if ((pos % vol->bytes_per_sector) != 0) {
		// read in partial first sector if necessary
		size_t amt;
		buffer = csi_get_block(&iter);
		if (buffer == NULL) {
			dprintf("dosfs_read: error reading cluster %lx, sector %lx\n", iter.cluster, iter.sector);
			result = EIO;
			goto bi;
		}
		amt = vol->bytes_per_sector - (pos % vol->bytes_per_sector);
		if (amt > *len) amt = *len;
		memcpy(buf, buffer + (pos % vol->bytes_per_sector), amt);
		csi_release_block(&iter);
		bytes_read += amt;

		if (bytes_read < *len)
			if ((result = iter_csi(&iter, 1)) != B_OK) {
				dprintf("dosfs_read: end of file reached\n");
				result = EIO;
				goto bi;
			}
	}

	// read middle sectors
	while (bytes_read + vol->bytes_per_sector <= *len) {
		result = csi_read_blocks(&iter, (uint8*)buf + bytes_read, *len - bytes_read);
		if (result < B_OK) {
			dprintf("dosfs_read: end of file reached\n");
			goto bi;
		}
		bytes_read += result;

		if (bytes_read < *len)
			if ((result = iter_csi(&iter, 1)) != B_OK) {
				dprintf("dosfs_read: end of file reached\n");
				result = EIO;
				goto bi;
			}
	}

	// read part of remaining sector if needed
	if (bytes_read < *len) {
		size_t amt;

		buffer = csi_get_block(&iter);
		if (buffer == NULL) {
			dprintf("dosfs_read: error reading cluster %lx, sector %lx\n", iter.cluster, iter.sector);
			result = EIO;
			goto bi;
		}
		amt = *len - bytes_read;
		memcpy((uint8*)buf + bytes_read, buffer, amt);
		csi_release_block(&iter);
		bytes_read += amt;
	}
	
	if (*len) {	
		cookie->ccache.iteration = node->iteration;
		cookie->ccache.index = (pos + *len - 1) / vol->bytes_per_sector / vol->sectors_per_cluster;
		cookie->ccache.cluster = iter.cluster;

		ASSERT(cookie->ccache.cluster == get_nth_fat_entry(vol, node->cluster, cookie->ccache.index));
	}

	result = B_OK;
bi:
	*len = bytes_read;

	if (result != B_OK) {
		DPRINTF(0, ("dosfs_read (%s)\n", strerror(result)));
	} else {
		DPRINTF(0, ("dosfs_read: read %lx bytes\n", *len));
	}
	UNLOCK_VOL(vol);
	
	return result;
}

int dosfs_write(void *_vol, void *_node, void *_cookie, off_t pos,
	const void *buf, size_t *len)
{
	nspace	*vol = (nspace *)_vol;
	vnode	*node = (vnode *)_node;
	filecookie *cookie = (filecookie *)_cookie;
	uint8 *buffer;
	struct csi iter;
	int result = B_OK;
	size_t bytes_written = 0;
	uint32 cluster1;
	off_t diff;

	LOCK_VOL(vol);

	if (check_nspace_magic((nspace *)vol, "dosfs_write") ||
		check_vnode_magic(node, "dosfs_write") ||
		check_filecookie_magic(cookie, "dosfs_write")) {
		*len = 0;
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if (node->mode & FAT_SUBDIR) {
		DPRINTF(0, ("dosfs_write called on subdirectory %Lx\n", node->vnid));
		*len = 0;
		UNLOCK_VOL(vol);
		return EISDIR;
		
	}

	DPRINTF(0, ("dosfs_write called %lx bytes at %Lx from buffer at %lx (vnode id %Lx)\n", *len, pos, (uint32)buf, node->vnid));

	if ((cookie->mode & O_RWMASK) == O_RDONLY) {
		dprintf("dosfs_write: called on file opened as read-only\n");
		result = EPERM;
		goto bi;
	}

	if (pos < 0) pos = 0;
	
	if (cookie->mode & O_APPEND) {
		pos = node->st_size;
	} 

	if (pos >= MAX_FILE_SIZE) {
		dprintf("dosfs_write: write position exceeds fat limits\n");
		result = E2BIG;
		goto bi;
	}

	if (pos + *len >= MAX_FILE_SIZE) {
		*len = (size_t)(MAX_FILE_SIZE - pos);
	}

	if (node->st_size && 
		(cookie->ccache.iteration == node->iteration) &&
		(pos >= cookie->ccache.index * vol->bytes_per_sector * vol->sectors_per_cluster)) {
		ASSERT(IS_DATA_CLUSTER(cookie->ccache.cluster));
		ASSERT(cookie->ccache.cluster == get_nth_fat_entry(vol, node->cluster, cookie->ccache.index));
		cluster1 = cookie->ccache.cluster;
		diff = pos - cookie->ccache.index * vol->bytes_per_sector * vol->sectors_per_cluster;
	} else {
		cluster1 = 0xffffffff;
		diff = 0;
	}

	// extend file size if needed
	if (pos + *len > node->st_size) {
		uint32 clusters = (pos + *len + vol->bytes_per_sector*vol->sectors_per_cluster - 1) / vol->bytes_per_sector / vol->sectors_per_cluster;
		if (node->st_size <= (clusters - 1) * vol->sectors_per_cluster * vol->bytes_per_sector) {
			if ((result = set_fat_chain_length(vol, node, clusters)) != B_OK) {
				goto bi;
			}
			node->iteration++;
		}
		node->st_size = pos + *len;
		/* needs to be written to disk asap so that later vnid calculations
		 * by get_next_dirent are correct
		 */
		write_vnode_entry(vol, node);

		DPRINTF(0, ("setting file size to %Lx (%lx clusters)\n", node->st_size, clusters));
		node->dirty = true;
	}

	if (cluster1 == 0xffffffff) {
		cluster1 = node->cluster;
		diff = pos;
	}
	diff /= vol->bytes_per_sector; /* convert to sectors */

	if ((result = init_csi(vol, cluster1, 0, &iter)) != B_OK) {
		dprintf("dosfs_write: invalid starting cluster (%lx)\n", cluster1);
		goto bi;
	}
	
	if (diff && ((result = iter_csi(&iter, diff)) != B_OK)) {
		dprintf("dosfs_write: end of file reached (init)\n");
		result = EIO;
		goto bi;
	}

	ASSERT(iter.cluster == get_nth_fat_entry(vol, node->cluster, pos / vol->bytes_per_sector / vol->sectors_per_cluster));

	if(*len > 0)
		node->dirty = true;

	// write partial first sector if necessary
	if ((pos % vol->bytes_per_sector) != 0) {
		size_t amt;
		buffer = csi_get_block(&iter);
		if (buffer == NULL) {
			dprintf("dosfs_write: error writing cluster %lx, sector %lx\n", iter.cluster, iter.sector);
			result = EIO;
			goto bi;
		}
		amt = vol->bytes_per_sector - (pos % vol->bytes_per_sector);
		if (amt > *len) amt = *len;
		memcpy(buffer + (pos % vol->bytes_per_sector), buf, amt);
		csi_mark_block_dirty(&iter);
		csi_release_block(&iter);
		bytes_written += amt;

		if (bytes_written < *len)
			if ((result = iter_csi(&iter, 1)) != B_OK) {
				dprintf("dosfs_write: end of file reached\n");
				result = EIO;
				goto bi;
			}
	}

	// write middle sectors
	while (bytes_written + vol->bytes_per_sector <= *len) {
		result = csi_write_blocks(&iter, (uint8*)buf + bytes_written, *len - bytes_written);
		if (result < B_OK) {
			dprintf("dosfs_write: end of file reached\n");
			goto bi;
		}
		bytes_written += result;

		if (bytes_written < *len)
			if ((result = iter_csi(&iter, 1)) != B_OK) {
				dprintf("dosfs_write: end of file reached\n");
				result = EIO;
				goto bi;
			}
	}

	// write part of remaining sector if needed
	if (bytes_written < *len) {
		size_t amt;

		buffer = csi_get_block(&iter);
		if (buffer == NULL) {
			dprintf("error writing cluster %lx, sector %lx\n", iter.cluster, iter.sector);
			result = EIO;
			goto bi;
		}
		amt = *len - bytes_written;
		memcpy(buffer, (uint8*)buf + bytes_written, amt);
		csi_mark_block_dirty(&iter);
		csi_release_block(&iter);
		bytes_written += amt;
	}

	if (*len) {	
		cookie->ccache.iteration = node->iteration;
		cookie->ccache.index = (pos + *len - 1) / vol->bytes_per_sector / vol->sectors_per_cluster;
		cookie->ccache.cluster = iter.cluster;

		ASSERT(cookie->ccache.cluster == get_nth_fat_entry(vol, node->cluster, cookie->ccache.index));
	}

	result = B_OK;

bi:
	*len = bytes_written;

	if (result != B_OK) {
		DPRINTF(0, ("dosfs_write (%s)\n", strerror(result)));
	} else {
		DPRINTF(0, ("dosfs_write: wrote %lx bytes\n", *len));
	}
	UNLOCK_VOL(vol);
	
	return result;
}

int dosfs_close(void *_vol, void *_node, void *_cookie)
{
	nspace	*vol = (nspace *)_vol;
	vnode	*node = (vnode *)_node;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_close") ||
		check_vnode_magic(node, "dosfs_close") ||
		check_filecookie_magic(_cookie, "dosfs_close")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_close (vnode id %Lx)\n", ((vnode *)_node)->vnid));

	if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && node->dirty) {
		_dosfs_sync(vol);
		node->dirty = false;
	}

	UNLOCK_VOL(vol);

	return 0;
}

int dosfs_free_cookie(void *_vol, void *_node, void *_cookie)
{
	nspace *vol = _vol;
	vnode *node = _node;
	filecookie *cookie = _cookie;
	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_free_cookie") ||
		check_vnode_magic(node, "dosfs_free_cookie") ||
		check_filecookie_magic(cookie, "dosfs_free_cookie")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_free_cookie (vnode id %Lx)\n", node->vnid));

	cookie->magic = ~FILECOOKIE_MAGIC;
	free(cookie);

	UNLOCK_VOL(vol);

	return 0;
}

int dosfs_create(void *_vol, void *_dir, const char *name, int omode,
	int perms, vnode_id *vnid, void **_cookie)
{
	nspace *vol = (nspace *)_vol;
	vnode *dir = (vnode *)_dir, *file;
	filecookie *cookie;
	status_t result = EINVAL;
	bool dups_exist;
	
	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_create") ||
		check_vnode_magic(dir, "dosfs_create")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	ASSERT(name != NULL);
	if (name == NULL) {
		dprintf("dosfs_create called with null name\n");
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_create called: %Lx/%s perms=%o omode=%o\n", dir->vnid, name, perms, omode));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("dosfs_create called on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (is_vnode_removed(vol->id, dir->vnid) > 0) {
		dprintf("dosfs_create() called in removed directory. disallowed.\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	if ((omode & O_RWMASK) == O_RDONLY) {
		dprintf("invalid permissions used in creating file\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	// create file cookie; do it here to make cleaning up easier
	if ((cookie = calloc(sizeof(filecookie), 1)) == NULL) {
		result = ENOMEM;
		goto bi;
	}

	result = findfile_case_duplicates(vol, dir, name, vnid, &file, &dups_exist);
	if (result == B_OK) {
		if (omode & O_EXCL) {
			dprintf("exclusive dosfs_create called on existing file %s\n", name);
			put_vnode(vol->id, file->vnid);
			result = EEXIST;
			goto bi;
		}
		
		if (file->mode & FAT_SUBDIR) {
			dprintf("can't dosfs_create over an existing subdirectory\n");
			put_vnode(vol->id, file->vnid);
			result = EPERM;
			goto bi;
		}

		if (file->disk_image) {
			dprintf("can't dosfs_create over a disk image\n");
			put_vnode(vol->id, file->vnid);
			result = EPERM;
			goto bi;
		}

		if (omode & O_TRUNC) {
			set_fat_chain_length(vol, file, 0);
			file->st_size = 0;
			file->iteration++;
		}
	} else if (result == ENOENT && dups_exist) { 
		// the file doesn't exist in the exact case, but another does in the
		// non-exact case. We wont create the new file.
		result = EEXIST;
		goto bi;
	} else if (result == ENOENT && !dups_exist) {
		// the file doesn't already exist in any case
		vnode dummy; /* used only to create directory entry */

		dummy.magic = VNODE_MAGIC;
		dummy.dir_vnid = dir->vnid;
		dummy.cluster = 0;
		dummy.end_cluster = 0;
		dummy.mode = 0;
		dummy.st_size = 0;
		time(&(dummy.st_time));

		if ((result = create_dir_entry(vol, dir, &dummy, name, &(dummy.sindex), &(dummy.eindex))) != B_OK) {
			dprintf("dosfs_create: error creating directory entry for %s (%s)\n", name, strerror(result));
			dummy.magic = ~VNODE_MAGIC;
			goto bi;
		}
		dummy.vnid = GENERATE_DIR_INDEX_VNID(dummy.dir_vnid, dummy.sindex);
		// XXX: dangerous construct
		if (find_vnid_in_vcache(vol, dummy.vnid) == B_OK) {
			dummy.vnid = generate_unique_vnid(vol);
			if ((result = add_to_vcache(vol, dummy.vnid, GENERATE_DIR_INDEX_VNID(dummy.dir_vnid, dummy.sindex))) < 0) {
				// XXX: should remove entry on failure
				if (vol->fs_flags & FS_FLAGS_OP_SYNC)
					_dosfs_sync(vol);
				goto bi;
			}
		}
		*vnid = dummy.vnid;
		dummy.magic = ~VNODE_MAGIC;

		result = get_vnode(vol->id, *vnid, (void **)&file);
		if (result < B_OK) {
			if (vol->fs_flags & FS_FLAGS_OP_SYNC)
				_dosfs_sync(vol);
			goto bi;
		}
	} else {
		goto bi;
	}
	
	cookie->magic = FILECOOKIE_MAGIC;
	cookie->mode = omode;
	cookie->ccache.iteration = file->iteration;
	cookie->ccache.index = 0;
	cookie->ccache.cluster = file->cluster;
	*_cookie = cookie;

	notify_listener(B_ENTRY_CREATED, vol->id, dir->vnid, 0, *vnid, name);

	result = 0;

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

bi:	if (result != B_OK) free(cookie);

	UNLOCK_VOL(vol);
	
	if (result != B_OK) DPRINTF(0, ("dosfs_create (%s)\n", strerror(result)));

	return result;
}

int dosfs_mkdir(void *_vol, void *_dir, const char *name, int perms)
{
	nspace *vol = (nspace *)_vol;
	vnode *dir = (vnode *)_dir, dummy;
	status_t result = EINVAL;
	struct csi csi;
	uchar *buffer;
	uint32 i;
	
	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_mkdir") ||
		check_vnode_magic(dir, "dosfs_mkdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if (is_vnode_removed(vol->id, dir->vnid) > 0) {
		dprintf("dosfs_mkdir() called in removed directory. disallowed.\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	DPRINTF(0, ("dosfs_mkdir called: %Lx/%s (perm %o)\n", dir->vnid, name, perms));

	if ((dir->mode & FAT_SUBDIR) == 0) {
		dprintf("dosfs_mkdir: vnode id %Lx is not a directory\n", dir->vnid);
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	// S_IFDIR is never set in perms, so we patch it
	perms &= ~S_IFMT; perms |= S_IFDIR;

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("mkdir called on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	/* only used to create directory entry */
	dummy.magic = VNODE_MAGIC;
	dummy.dir_vnid = dir->vnid;
	if ((result = allocate_n_fat_entries(vol, 1, (int32 *)&(dummy.cluster))) < 0) {
		dprintf("dosfs_mkdir: error allocating space for %s (%s))\n", name, strerror(result));
		goto bi;
	}
	dummy.end_cluster = dummy.cluster;
	dummy.mode = FAT_SUBDIR;
	if (!(perms & (S_IWUSR | S_IWGRP | S_IWGRP))) {
		dummy.mode |= FAT_READ_ONLY;
	}
	dummy.st_size = vol->bytes_per_sector*vol->sectors_per_cluster;
	time(&(dummy.st_time));

	dummy.vnid = GENERATE_DIR_CLUSTER_VNID(dummy.dir_vnid, dummy.cluster);
	// XXX: dangerous construct
	if (find_vnid_in_vcache(vol, dummy.vnid) == B_OK) {
		dummy.vnid = generate_unique_vnid(vol);
		if ((result = add_to_vcache(vol, dummy.vnid, GENERATE_DIR_CLUSTER_VNID(dummy.dir_vnid, dummy.cluster))) < 0)
			goto bi2;
	}

	if ((result = dlist_add(vol, dummy.vnid)) < 0) {
		dprintf("dosfs_mkdir: error adding directory %s to dlist (%s)\n", name, strerror(result));
		goto bi3;
	}

	buffer = malloc(vol->bytes_per_sector);
	if (!buffer) {
		result = ENOMEM;
		goto bi4;
	}

	if ((result = create_dir_entry(vol, dir, &dummy, name, &(dummy.sindex), &(dummy.eindex))) != B_OK) {
		dprintf("dosfs_mkdir: error creating directory entry for %s (%s))\n", name, strerror(result));
		goto bi5;
	}

	// create '.' and '..' entries and then end of directories
	memset(buffer, 0, vol->bytes_per_sector);
	memset(buffer, ' ', 11);
	memset(buffer+0x20, ' ', 11);
	buffer[0] = buffer[0x20] = buffer[0x21] = '.';
	buffer[0x0b] = buffer[0x2b] = 0x30;
	i = time_t2dos(dummy.st_time);
	buffer[0x16] = i & 0xff;
	buffer[0x17] = (i >> 8) & 0xff;
	buffer[0x18] = (i >> 16) & 0xff;
	buffer[0x19] = (i >> 24) & 0xff;
	i = time_t2dos(dir->st_time);
	buffer[0x36] = i & 0xff;
	buffer[0x37] = (i >> 8) & 0xff;
	buffer[0x38] = (i >> 16) & 0xff;
	buffer[0x39] = (i >> 24) & 0xff;
	buffer[0x1a] = dummy.cluster & 0xff;
	buffer[0x1b] = (dummy.cluster >> 8) & 0xff;
	if (vol->fat_bits == 32) {
		buffer[0x14] = (dummy.cluster >> 16) & 0xff;
		buffer[0x15] = (dummy.cluster >> 24) & 0xff;
	}
	// root directory is always denoted by cluster 0, even for fat32 (!)
	if (dir->vnid != vol->root_vnode.vnid) {
		buffer[0x3a] = dir->cluster & 0xff;
		buffer[0x3b] = (dir->cluster >> 8) & 0xff;
		if (vol->fat_bits == 32) {
			buffer[0x34] = (dir->cluster >> 16) & 0xff;
			buffer[0x35] = (dir->cluster >> 24) & 0xff;
		}
	}

	init_csi(vol, dummy.cluster, 0, &csi);
	csi_write_block(&csi, buffer);

	// clear out rest of cluster to keep scandisk happy
	memset(buffer, 0, vol->bytes_per_sector);
	for (i=1;i<vol->sectors_per_cluster;i++) {
		if (iter_csi(&csi, 1) != B_OK) {
			dprintf("dosfs_mkdir: error writing directory cluster\n");
			break;
		}
		csi_write_block(&csi, buffer);
	}

	free(buffer);

	notify_listener(B_ENTRY_CREATED, vol->id, dir->vnid, 0, dummy.vnid, name);

	result = B_OK;

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

	UNLOCK_VOL(vol);
	return result;

bi5:free(buffer);
bi4:dlist_remove(vol, dummy.vnid);
bi3:if (IS_ARTIFICIAL_VNID(dummy.vnid)) remove_from_vcache(vol, dummy.vnid);
bi2:clear_fat_chain(vol, dummy.cluster);
	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);
bi:	dummy.magic = ~VNODE_MAGIC;

	UNLOCK_VOL(vol);
	if (result != B_OK) DPRINTF(0, ("dosfs_mkdir (%s)\n", strerror(result)));
	return result;
}

int dosfs_rename(void *_vol, void *_odir, const char *oldname,
	void *_ndir, const char *newname)
{
	status_t result = EINVAL;
	nspace *vol = (nspace *)_vol;
	vnode *odir = (vnode *)_odir, *ndir = (vnode *)_ndir, *file, *file2;
	uint32 ns, ne;
	bool dups_exist;
	bool dirty = false;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_rename") ||
		check_vnode_magic(odir, "dosfs_rename") ||
		check_vnode_magic(ndir, "dosfs_rename")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_rename called: %Lx/%s->%Lx/%s\n", odir->vnid, oldname, ndir->vnid, newname));

	if(!is_filename_legal(newname)) {
		dprintf("dosfs_rename called with invalid name '%s'\n", newname);
		result = EINVAL;
		goto bi;
	}

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("rename called on read-only volume\n");
		result = EROFS;
		goto bi;
	}

	if (!oldname || !(*oldname) || !newname || !(*newname)) {
		result = EINVAL;
		goto bi;
	}

	if ((odir->vnid == ndir->vnid) && !strcmp(oldname, newname)) {
		result = EPERM;
		goto bi;
	}

	// locate the file
	if ((result = findfile_case(vol,odir,oldname,NULL,&file)) != B_OK) {
		DPRINTF(0, ("dosfs_rename: can't find file %s in directory %Lx\n", oldname, odir->vnid));
		goto bi;
	}

	if (file->disk_image) {
		dprintf("rename called on disk image or disk image directory\n");
		result = EPERM;
		goto bi1;
	}

	if (check_vnode_magic(file, "dosfs_rename")) {
		result = EINVAL;
		goto bi1;
	}

	// don't move a directory into one of its children
	if (file->mode & FAT_SUBDIR) {
		vnode_id vnid = ndir->vnid;
		while (1) {
			vnode *dir;
			vnode_id parent;

			if (vnid == file->vnid) {
				result = EINVAL;
				goto bi1;
			}

			if (vnid == vol->root_vnode.vnid)
				break;

			result = get_vnode(vol->id, vnid, (void **)&dir);
			if (result < B_OK)
				goto bi1;
			parent = dir->dir_vnid;
			put_vnode(vol->id, vnid);
			vnid = parent;
		}
	}

	// see if file already exists and erase it if it does
	result = findfile_case_duplicates(vol, ndir, newname, NULL, &file2, &dups_exist);
	if (result == B_OK) {
		if (file2->mode & FAT_SUBDIR) {
			dprintf("destination already occupied by a directory\n");
			result = EPERM;
			goto bi2;
		}

		if (file2->disk_image) {
			DPRINTF(0, ("dosfs_rename: can't replace disk image or disk image directory\n"));
			result = EPERM;
			goto bi2;
		}
		ns = file2->sindex; ne = file2->eindex;

		// let others know the old file is gone
		notify_listener(B_ENTRY_REMOVED, vol->id, ndir->vnid, 0, file2->vnid, NULL);

		// Make sure this vnode 1) is in the vcache and 2) no longer has a
		// location associated with it. See discussion in dosfs_unlink()
		vcache_set_entry(vol, file2->vnid, generate_unique_vnid(vol));

		// mark vnode for removal (dosfs_remove_vnode will clear the fat chain)
		// note we don't have to lock the file because the fat chain doesn't
		// get wiped from the disk until dosfs_remove_vnode() is called; we'll
		// have a phantom chain in effect until the last file is closed.
		remove_vnode(vol->id, file2->vnid); // must be done in this order
		put_vnode(vol->id, file2->vnid);

		dirty = true;

		// erase old directory entry
		if ((result = erase_dir_entry(vol, file)) != B_OK) {
			dprintf("dosfs_rename: error erasing old directory entry for %s (%s)\n", newname, strerror(result));
			goto bi1;
		}	
	} else if (result == ENOENT && (!dups_exist || (odir->vnid == ndir->vnid && !strcasecmp(oldname, newname)))) {
		// there isn't an entry and there are no duplicates in the target dir or
		// there isn't an entry and the target dir is the same as the source dir and
		//   the source and target name are the same, case-insensitively

		// erase old directory entry
		if ((result = erase_dir_entry(vol, file)) != B_OK) {
			dprintf("dosfs_rename: error erasing old directory entry for %s (%s)\n", newname, strerror(result));
			goto bi1;
		}

		dirty = true;
	
		// create the new directory entry
		if ((result = create_dir_entry(vol, ndir, file, newname, &ns, &ne)) != B_OK) {
			dprintf("dosfs_rename: error creating directory entry for %s\n", newname);
			goto bi1;
		}
	} else if (result == ENOENT && dups_exist) {
		// the entry doesn't exist but a non-case entry does, so we can't do it
		result = EEXIST;
		goto bi1;
	} else {
		goto bi1;
	}

	// shrink the directory (an error here is not disastrous)
	compact_directory(vol, odir);

	dirty = true;

	// update vnode information
	file->dir_vnid = ndir->vnid;
	file->sindex = ns;
	file->eindex = ne;

	// update vcache
	vcache_set_entry(vol, file->vnid,
			(file->st_size) ?
			GENERATE_DIR_CLUSTER_VNID(file->dir_vnid, file->cluster) : 
			GENERATE_DIR_INDEX_VNID(file->dir_vnid, file->sindex));

	// XXX: only write changes in the directory entry if needed	
	//      (i.e. old entry, not new)
	write_vnode_entry(vol, file);

	if (file->mode & FAT_SUBDIR) {
		// update '..' directory entry if needed
		// this should most properly be in write_vnode, but it is safe
		// to keep it here since this is the only way the cluster of
		// the parent can change.
		struct diri diri;
		uint8 *buffer;
		if ((buffer = diri_init(vol, file->cluster, 1, &diri)) == NULL) {
			dprintf("error opening directory :(\n");
			result = EIO;
			goto bi2;
		}
		if (memcmp(buffer, "..         ", 11)) {
			dprintf("invalid directory :(\n");
			result = EIO;
			goto bi2;
		}
		if (ndir->vnid == vol->root_vnode.vnid) {
			// root directory always has cluster = 0
			buffer[0x1a] = buffer[0x1b] = 0;
		} else {
			buffer[0x1a] = ndir->cluster & 0xff;
			buffer[0x1b] = (ndir->cluster >> 8) & 0xff;
			if (vol->fat_bits == 32) {
				buffer[0x14] = (ndir->cluster >> 16) & 0xff;
				buffer[0x15] = (ndir->cluster >> 24) & 0xff;
			}
		}
		diri_mark_dirty(&diri);
		diri_free(&diri);
	}

#if TRACK_FILENAME
	if (file->filename) free(file->filename);
	file->filename = malloc(strlen(newname) + 1);
	if (file->filename) strcpy(file->filename, newname);
#endif

	notify_listener(B_ENTRY_MOVED, vol->id, odir->vnid, ndir->vnid, file->vnid, newname);

	// update MIME information
	if(!(file->mode & FAT_SUBDIR)) {
		set_mime_type(file, newname);
		notify_listener(B_ATTR_CHANGED, vol->id, 0, 0, file->vnid, "BEOS:TYPE");
	}

	result = 0;
	
bi2:
	if (result != B_OK) put_vnode(vol->id, file2->vnid);
bi1:
	put_vnode(vol->id, file->vnid);
bi:
	if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && dirty)
		_dosfs_sync(vol);
	UNLOCK_VOL(vol);
	if (result != B_OK) DPRINTF(0, ("dosfs_rename (%s)\n", strerror(result)));
	return result;
}

int dosfs_remove_vnode(void *_vol, void *_node, char reenter)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	if (!reenter) { LOCK_VOL(vol); }

	if (check_nspace_magic(vol, "dosfs_remove_vnode") ||
		check_vnode_magic(node, "dosfs_remove_vnode")) {
		if (!reenter) UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_remove_vnode (%Lx)\n", node->vnid));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("dosfs_remove_vnode: read-only volume\n");
		if (!reenter) UNLOCK_VOL(vol);
		return EROFS;
	}

	// clear the fat chain
	ASSERT((node->cluster == 0) || IS_DATA_CLUSTER(node->cluster));
	/* XXX: the following assertion was tripped */
	ASSERT((node->cluster != 0) || (node->st_size == 0));
	if (node->cluster != 0)
		clear_fat_chain(vol, node->cluster);

	/* remove vnode id from the cache */
	if (find_vnid_in_vcache(vol, node->vnid) == B_OK)
		remove_from_vcache(vol, node->vnid);

	/* and from the dlist as well */
	if (node->mode & FAT_SUBDIR)
		dlist_remove(vol, node->vnid);

	node->magic = ~VNODE_MAGIC; // munge magic number to be safe
	free(node);
	
	if (!reenter) {
		if (vol->fs_flags & FS_FLAGS_OP_SYNC) {
			// sync the entire filesystem,
			// but only if we're not reentrant. Presumably the
			// function that called this will sync.
			_dosfs_sync(vol);
		}
		UNLOCK_VOL(vol);
	}

	return 0;
}

// get rid of node or directory
static int do_unlink(void *_vol, void *_dir, const char *name, bool is_file)
{
	int result = EINVAL;
	nspace *vol = (nspace *)_vol;
	vnode *dir = (vnode *)_dir, *file;
	vnode_id vnid;

	if (!strcmp(name, "."))
		return EPERM;

	if (!strcmp(name, ".."))
		return EPERM;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "do_unlink") ||
		check_vnode_magic(dir, "do_unlink")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("do_unlink %Lx/%s\n", dir->vnid, name));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("do_unlink: read-only volume\n");
		result = EROFS;
		goto bi;
	}

	// locate the file
	if ((result = findfile_case(vol,dir,name,&vnid,&file)) != B_OK) {
		DPRINTF(0, ("do_unlink: can't find file %s in directory %Lx\n", name, dir->vnid));
		result = ENOENT;
		goto bi;
	}

	if (check_vnode_magic(file, "do_unlink")) {
		result = EINVAL;
		goto bi1;
	}

	if (file->disk_image) {
		DPRINTF(0, ("do_unlink: can't unlink disk image or disk image directory\n"));
		result = EPERM;
		goto bi1;
	}

	// don't need to check file permissions because it will be done for us
	// also don't need to lock the file (see dosfs_rename for reasons why)
	if (is_file) {
		if (file->mode & FAT_SUBDIR) {
			result = EISDIR;
			goto bi1;
		}
	} else {
		if ((file->mode & FAT_SUBDIR) == 0) {
			result = ENOTDIR;
			goto bi1;
		}

		if (file->vnid == vol->root_vnode.vnid) {
			// this actually isn't a problem since the root vnode
			// will always be busy while the volume mounted
			dprintf("dosfs_rmdir: don't call this on the root directory\n");
			result = EPERM;
			goto bi1;
		}

		if ((result = check_dir_empty(vol, file)) < 0) {
			if (result == ENOTEMPTY) DPRINTF(0, ("dosfs_rmdir called on non-empty directory\n"));
			goto bi1;
		}
	}

	// erase the entry in the parent directory
	if ((result = erase_dir_entry(vol, file)) != B_OK)
		goto bi1;

	// shrink the parent directory (errors here are not disastrous)
	compact_directory(vol, dir);

	notify_listener(B_ENTRY_REMOVED, vol->id, dir->vnid, 0, file->vnid, NULL);

	/* Set the loc to a unique value. This effectively removes it from the
	 * vcache without releasing its vnid for reuse. It also nicely reserves
	 * the vnid from use by other nodes. This is okay because the vnode is
	 * locked in memory after this point and loc will not be referenced from
	 * here on.
	 */
	vcache_set_entry(vol, file->vnid, generate_unique_vnid(vol));

	// fsil doesn't call dosfs_write_vnode for us, so we have to free the
	// vnode manually here.
	remove_vnode(vol->id, file->vnid);

	result = 0;
	
	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

bi1:put_vnode(vol->id, vnid);		// get 1 free
bi:	UNLOCK_VOL(vol);

	if (result != B_OK) DPRINTF(0, ("do_unlink (%s)\n", strerror(result)));

	return result;
}

int dosfs_unlink(void *vol, void *dir, const char *name)
{
	DPRINTF(1, ("dosfs_unlink called\n"));

	return do_unlink(vol,dir,name,true);
}

int dosfs_rmdir(void *vol, void *dir, const char *name)
{
	DPRINTF(1, ("dosfs_rmdir called\n"));
	
	return do_unlink(vol,dir,name,false);
}
