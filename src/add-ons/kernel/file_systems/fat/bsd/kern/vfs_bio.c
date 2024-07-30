/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2004 Poul-Henning Kamp
 * Copyright (c) 1994,1997 John S. Dyson
 * Copyright (c) 2013 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by Konstantin Belousov
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


// Modified to support the Haiku FAT driver. These functions, as implemented here, assume
// that the volume's block cache was created with a blockSize equal to DEV_BSIZE. We also
// support access to the file cache via these functions, even though the driver doesn't
// need that capability in its current form.

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/malloc.h"
#include "sys/vnode.h"
#include "sys/conf.h"

#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/msdosfsmount.h"

#include "dosfs.h"


#ifdef USER
#define dprintf printf
#endif


int msdosfs_bmap(struct vnode* a_vp, daddr_t a_bn, struct bufobj** a_bop, daddr_t* a_bnp,
	int* a_runp, int* a_runb);
int getblkx(struct vnode* vp, daddr_t blkno, daddr_t dblkno, int size, int slpflag, int slptimeo,
	int flags, struct buf** bpp);

static status_t allocate_data(struct buf* buf, int size);
static status_t put_buf(struct buf* buf);


/*! The FAT driver uses this in combination with vm_page_count_severe to detect low system
	resources. However, there is no analagous Haiku function to map this to.
*/
int
buf_dirty_count_severe(void)
{
	return 0;
}


/*!	Get a buffer with the specified data.
	@param blkno The logical block being requested. If the vnode type is VREG (blkno is relative
	to the start of the file), msdosfs_bmap will be called to convert blkno into a block number
	relative to the start of the volume. If the vnode type is VBLK, blkno is already relative to
	the start of the volume.
	@param cred Ignored in the port.
	@post bpp Points to the requested struct buf*, if successful. If an error is returned, *bpp is
	NULL.
*/
int
bread(struct vnode* vp, daddr_t blkno, int size, struct ucred* cred, struct buf** bpp)
{
	struct buf* buf = NULL;
	int error;

	error = getblkx(vp, blkno, blkno, size, 0, 0, 0, &buf);

	if (error == 0)
		*bpp = buf;

	return error;
}


/*! Added for the Haiku port:  common initial steps for bdwrite, bawrite, and bwrite.

*/
static status_t
_bwrite(struct buf* buf)
{
	struct vnode* deviceNode = buf->b_vp;
	struct mount* bsdVolume = deviceNode->v_rdev->si_mountpt;
	void* blockCache = bsdVolume->mnt_cache;
	struct msdosfsmount* fatVolume = (struct msdosfsmount*)bsdVolume->mnt_data;
	status_t status = B_OK;

	ASSERT(MOUNTED_READ_ONLY(fatVolume) == 0);
		// we should not have gotten this far if this is a read-only volume

	if (buf->b_vreg != NULL) {
		// copy b_data to the file cache
		struct vnode* bsdNode = buf->b_vreg;
		struct denode* fatNode = (struct denode*)bsdNode->v_data;
		off_t fileOffset = 0;
		size_t bytesWritten = 0;

		if (bsdNode->v_resizing == true)
			return status;

		ASSERT((fatNode->de_Attributes & ATTR_READONLY) == 0);

		fileOffset = de_cn2off(fatVolume, buf->b_lblkno);
		ASSERT_ALWAYS((u_long)(fileOffset + buf->b_bufsize) <= fatNode->de_FileSize);

		bytesWritten = (size_t)buf->b_bufsize;
		status = file_cache_write(bsdNode->v_cache, NULL, fileOffset, buf->b_data, &bytesWritten);
		if (bytesWritten != (size_t)buf->b_bufsize)
			return EIO;
	} else if (buf->b_owned == false) {
		// put the single block cache block that was modified
		block_cache_put(blockCache, buf->b_blkno);
	} else {
		// copy b_data into mutiple block cache blocks and put them
		uint32 cBlockCount = buf->b_bufsize / CACHED_BLOCK_SIZE;
		uint32 i;
		for (i = 0; i < cBlockCount && buf->b_bcpointers[i] != NULL; ++i) {
			memcpy((caddr_t)buf->b_bcpointers[i], buf->b_data + (i * CACHED_BLOCK_SIZE),
				CACHED_BLOCK_SIZE);
			block_cache_put(blockCache, buf->b_blkno + i);
			buf->b_bcpointers[i] = NULL;
		}
	}

	return status;
}


/*! The block_cache block(s) corresponding to bp are put or, if a regular file is being
	written, the file cache is updated. Nothing is flushed to disk at this time.
*/
void
bdwrite(struct buf* bp)
{
	if (_bwrite(bp) != B_OK)
		return;

	put_buf(bp);

	return;
}


/*! In FreeBSD, this flushes bp to disk asynchronously. However, Haiku's block cache
	has no asynchronous flush option, so the operation is only asynchronous if we are
	working with the file cache (i.e. when writing to a regular file). The driver
	only uses bawrite if it detects low system resources.
*/
void
bawrite(struct buf* bp)
{
	_bwrite(bp);

	if (bp->b_vreg != NULL) {
		if (bp->b_vreg->v_resizing == false)
			file_cache_sync(bp->b_vreg->v_cache);
	} else {
		void* blockCache = bp->b_vp->v_rdev->si_mountpt->mnt_cache;

		if (bp->b_owned == false) {
			block_cache_sync_etc(blockCache, bp->b_blkno, 1);
		} else {
			block_cache_sync_etc(blockCache, bp->b_blkno,
				howmany(bp->b_bufsize, CACHED_BLOCK_SIZE));
		}
	}

	put_buf(bp);

	return;
}


/*! Each bread call must be balanced with either a b(d/a)write (to write changes) or a brelse.

*/
void
brelse(struct buf* bp)
{
	struct mount* bsdVolume = bp->b_vp->v_rdev->si_mountpt;
	void* blockCache = bsdVolume->mnt_cache;
	bool readOnly = MOUNTED_READ_ONLY(VFSTOMSDOSFS(bsdVolume));

	if (bp->b_vreg != NULL) {
		put_buf(bp);
	} else if (bp->b_owned == false) {
		if (readOnly == true)
			block_cache_set_dirty(blockCache, bp->b_blkno, false, -1);
		block_cache_put(blockCache, bp->b_blkno);
		put_buf(bp);
	} else {
		uint32 cBlockCount = bp->b_bufsize / CACHED_BLOCK_SIZE;
		uint32 i;
		for (i = 0; i < cBlockCount && bp->b_bcpointers[i] != NULL; ++i) {
			if (readOnly == true)
				block_cache_set_dirty(blockCache, bp->b_blkno + i, false, -1);
			block_cache_put(blockCache, bp->b_blkno + i);
			bp->b_bcpointers[i] = NULL;
		}

		put_buf(bp);
	}

	return;
}


/*! Similar to bread, but can be used when it's not necessary to read the existing contents of
	the block. As currently implemented, it is not any faster than bread. The last 3 parameters
	are ignored; the driver always passes 0 for each of them.
	@param size The number of blocks to get.
*/
struct buf*
getblk(struct vnode* vp, daddr_t blkno, int size, int slpflag, int slptimeo, int flags)
{
	struct buf* buf = NULL;
	int error = 0;

	error = getblkx(vp, blkno, blkno, size, slpflag, slptimeo, flags, &buf);
	if (error != 0)
		return NULL;

	return buf;
}


/*!	Return a specified block in a BSD-style struct buf.
	@param blkno If vp is the device node, a disk block number in units of DEV_BSIZE; otherwise, a
	file-relative block number in units of cluster size.
	@param dblkno Disk block number, if known by the client. If vp is not the device node, getblkx
	will calculate the disk block number from blkno and ignore this parameter.
	@param splflag Ignored in the port.
	@param slptimeo Ignored in the port.
	@param flags Ignored in the port.
*/
int
getblkx(struct vnode* vp, daddr_t blkno, daddr_t dblkno, int size, int slpflag, int slptimeo,
	int flags, struct buf** bpp)
{
	struct msdosfsmount* fatVolume;
	struct vnode* deviceNode;
	status_t status = B_OK;

	bool readOnly = true;
	bool foundExisting = false;

	uint32 i;
	void* blockCache = NULL;
	uint32 cBlockCount;
		// the number of block cache blocks spanned by the client's request
	struct buf* newBuf = NULL;
		// the buf to be returned

	if (vp->v_type == VREG) {
		fatVolume = vp->v_mount->mnt_data;
		// convert blkno from file-relative to volume-relative
		msdosfs_bmap(vp, blkno, NULL, &dblkno, NULL, NULL);
			// output (dblkno) is always in units of DEV_BSIZE, even if blkno is in clusters
		blockCache = vp->v_mount->mnt_cache;
		readOnly
			= MOUNTED_READ_ONLY(fatVolume) || ((VTODE(vp))->de_Attributes & ATTR_READONLY) != 0;
		deviceNode = fatVolume->pm_devvp;
	} else if (vp->v_type == VBLK) {
		fatVolume = vp->v_rdev->si_mountpt->mnt_data;
		blockCache = vp->v_rdev->si_mountpt->mnt_cache;
		readOnly = MOUNTED_READ_ONLY(fatVolume);
		deviceNode = vp;
	} else {
		return ENOTSUP;
	}

	// Before allocating memory for a new struct buf, try to reuse an existing one
	// in the device vnode's lists.
	rw_lock_write_lock(&deviceNode->v_bufobj.bo_lock.haikuRW);
	if (size == CACHED_BLOCK_SIZE && vp->v_type != VREG
		&& SLIST_EMPTY(&deviceNode->v_bufobj.bo_emptybufs) == false) {
		// Get a buf with no data space. It will just point to a block cache block.
		newBuf = SLIST_FIRST(&deviceNode->v_bufobj.bo_emptybufs);
		SLIST_REMOVE_HEAD(&deviceNode->v_bufobj.bo_emptybufs, link);
		--deviceNode->v_bufobj.bo_empties;
		foundExisting = true;
	} else if (size == (int)fatVolume->pm_bpcluster
		&& SLIST_EMPTY(&deviceNode->v_bufobj.bo_clusterbufs) == false) {
		// Get a buf with cluster-size data storage from the free list.
		newBuf = SLIST_FIRST(&deviceNode->v_bufobj.bo_clusterbufs);
		SLIST_REMOVE_HEAD(&deviceNode->v_bufobj.bo_clusterbufs, link);
		--deviceNode->v_bufobj.bo_clusters;
		foundExisting = true;
	} else if (size == (int)fatVolume->pm_fatblocksize
		&& SLIST_EMPTY(&deviceNode->v_bufobj.bo_fatbufs) == false) {
		// This branch will never be reached in FAT16 or FAT32 so long as pm_fatblocksize and
		// CACHED_BLOCK_SIZE are both 512.
		newBuf = SLIST_FIRST(&deviceNode->v_bufobj.bo_fatbufs);
		SLIST_REMOVE_HEAD(&deviceNode->v_bufobj.bo_fatbufs, link);
		--deviceNode->v_bufobj.bo_fatblocks;
		foundExisting = true;
	}
	rw_lock_write_unlock(&deviceNode->v_bufobj.bo_lock.haikuRW);

	if (foundExisting == false) {
		newBuf = malloc(sizeof(struct buf), 0, 0);
		newBuf->b_data = NULL;
		newBuf->b_bufsize = 0;
		for (i = 0; i < 128; ++i)
			newBuf->b_bcpointers[i] = NULL;
	}

	// set up / reset the buf
	newBuf->b_bcount = size;
	newBuf->b_resid = size;
	newBuf->b_blkno = dblkno;
		// units of DEV_BSIZE, always
	newBuf->b_flags = 0;
	newBuf->b_lblkno = blkno;
		// units depend on vnode type
	newBuf->b_vp = deviceNode;
		// note that b_vp does not point to the node passed as vp, unless vp is the deviceNode
	newBuf->b_owned = false;
	newBuf->b_vreg = vp->v_type == VREG ? vp : NULL;

	ASSERT(size == newBuf->b_resid);
	cBlockCount = howmany(size, CACHED_BLOCK_SIZE);

	// Three branches:
	// For regular files, copy from file cache into b_data.
	// Otherwise, if the requested size equals the cached block size, use the block cache directly.
	// Otherwise, copy from the block cache into b_data.
	if (vp->v_type == VREG) {
		// The occasions when regular file data is accessed through the ported BSD code
		// are limited (e.g. deextend) and occur when the node is locked. If we go down this
		// branch, we tend to return early because vp->v_resizing is true.

		off_t fileOffset;
		size_t bytesRead;

		newBuf->b_owned = true;
		status = allocate_data(newBuf, size);
		if (status != B_OK)
			return B_TO_POSIX_ERROR(status);

		// Don't use the file cache while resizing; wait until node lock is released to avoid
		// deadlocks.
		if (vp->v_resizing == true) {
			(*bpp) = newBuf;
				// we need to return a buffer with b_data allocated even in this case,
				// because detrunc may zero out the unused space at the end of the last cluster
			return B_TO_POSIX_ERROR(status);
		}

		fileOffset = de_cn2off(fatVolume, blkno);

		ASSERT(size <= (int)newBuf->b_bufsize);
		bytesRead = (size_t)size;
		status = file_cache_read(vp->v_cache, NULL, fileOffset, newBuf->b_data, &bytesRead);
		if (status != B_OK) {
			put_buf(newBuf);
			return B_TO_POSIX_ERROR(status);
		}
		if (bytesRead != (size_t)size) {
			put_buf(newBuf);
			return EIO;
		}
	} else if (size == CACHED_BLOCK_SIZE && vp->v_type != VREG) {
		if (readOnly == true)
			newBuf->b_data = (void*)block_cache_get(blockCache, dblkno);
		else
			newBuf->b_data = block_cache_get_writable(blockCache, dblkno, -1);
		if (newBuf->b_data == NULL) {
			put_buf(newBuf);
			return EIO;
		}
		newBuf->b_bufsize = CACHED_BLOCK_SIZE;
	} else {
		// need to get more than one cached block and copy them to make a continuous buffer
		newBuf->b_owned = true;
		status = allocate_data(newBuf, size);
		if (status != 0)
			return B_TO_POSIX_ERROR(status);

		for (i = 0; i < cBlockCount && status == B_OK; i++) {
			if (readOnly == true)
				newBuf->b_bcpointers[i] = (void*)block_cache_get(blockCache, dblkno + i);
			else
				newBuf->b_bcpointers[i] = block_cache_get_writable(blockCache, dblkno + i, -1);
			if (newBuf->b_bcpointers[i] == NULL) {
				put_buf(newBuf);
				return EIO;
			}
		}

		ASSERT(cBlockCount * CACHED_BLOCK_SIZE == (u_long)newBuf->b_bufsize);
		for (i = 0; i < cBlockCount; i++) {
			memcpy(newBuf->b_data + (i * CACHED_BLOCK_SIZE), (caddr_t)newBuf->b_bcpointers[i],
				CACHED_BLOCK_SIZE);
		}
	}

	newBuf->b_resid -= size;
	ASSERT(newBuf->b_resid == 0);

	*bpp = newBuf;

	return B_TO_POSIX_ERROR(status);
}


/*! Used by deextend to update metadata of pages in the last added cluster.
	Not applicable in Haiku.
*/
void
vfs_bio_clrbuf(struct buf* bp)
{
	return;
}


/*! Used by deextend to zero out the remainder of a cluster beyond EOF. In the Haiku port we avoid
	file cache writes when the node is locked (as it is when deextend is called) to prevent
	deadlocks. This data must therefore be zero'd after return from deextend.
*/
void
vfs_bio_bzero_buf(struct buf* bp, int base, int size)
{
	return;
}


/*! Flush buffer to disk synchronously.

*/
int
bwrite(struct buf* bp)
{
	status_t status = _bwrite(bp);
	if (status != B_OK) {
		put_buf(bp);
		return B_TO_POSIX_ERROR(status);
	}

	if (bp->b_vreg != NULL) {
		// file cache
		if (bp->b_vreg->v_resizing == false) {
			bp->b_vreg->v_sync = true;
			status = file_cache_sync(bp->b_vreg->v_cache);
			bp->b_vreg->v_sync = false;
		}
	} else {
		// block cache
		void* blockCache = bp->b_vp->v_rdev->si_mountpt->mnt_cache;

		if (bp->b_owned == false) {
			// single block
			status = block_cache_sync_etc(blockCache, bp->b_blkno, 1);
		} else {
			// multiple blocks
			status = block_cache_sync_etc(blockCache, bp->b_blkno,
				howmany(bp->b_bufsize, CACHED_BLOCK_SIZE));
		}
	}

	put_buf(bp);

	return B_TO_POSIX_ERROR(status);
}


/*! Added for the Haiku port. Ensure that buf->b_data points to 'size' bytes of zero'd memory.

*/
static status_t
allocate_data(struct buf* buf, int size)
{
	if (buf->b_data == NULL) {
		// Either this is a newly created buf, or we are recycling a buf that
		// has no memory allocated for b_data.
		buf->b_data = (caddr_t)calloc(size, sizeof(char));
		if (buf->b_data == NULL)
			return B_NO_MEMORY;
		buf->b_bufsize = size;
	} else {
		// This is an existing buf with space allocated for b_data; maybe we can reuse it.
		if (buf->b_bufsize == size) {
			bzero(buf->b_data, buf->b_bufsize);
		} else {
			free(buf->b_data, 0);
			buf->b_data = (caddr_t)calloc(size, sizeof(char));
			if (buf->b_data == NULL)
				return B_NO_MEMORY;
			buf->b_bufsize = size;
		}
	}

	return B_OK;
}


/*! Added for the Haiku port. Either add buf to a list of unused bufs, or free it (and b_data, if
	necessary).
*/
static status_t
put_buf(struct buf* buf)
{
	struct vnode* deviceNode = buf->b_vp;
	struct msdosfsmount* fatVolume = (struct msdosfsmount*)deviceNode->v_rdev->si_mountpt->mnt_data;

	rw_lock_write_lock(&deviceNode->v_bufobj.bo_lock.haikuRW);
	if (buf->b_owned != 0) {
		if ((u_long)buf->b_bufsize == fatVolume->pm_bpcluster
			&& deviceNode->v_bufobj.bo_clusters < BUF_CACHE_SIZE) {
			SLIST_INSERT_HEAD(&deviceNode->v_bufobj.bo_clusterbufs, buf, link);
			++deviceNode->v_bufobj.bo_clusters;
		} else if ((u_long)buf->b_bufsize == fatVolume->pm_fatblocksize
			&& deviceNode->v_bufobj.bo_fatblocks < BUF_CACHE_SIZE) {
			SLIST_INSERT_HEAD(&deviceNode->v_bufobj.bo_fatbufs, buf, link);
			++deviceNode->v_bufobj.bo_fatblocks;
		} else {
			free(buf->b_data, 0);
			free(buf, 0);
		}
	} else if (deviceNode->v_bufobj.bo_empties < BUF_CACHE_SIZE) {
		buf->b_data = NULL;
		buf->b_bufsize = 0;
		SLIST_INSERT_HEAD(&deviceNode->v_bufobj.bo_emptybufs, buf, link);
		++deviceNode->v_bufobj.bo_empties;
	} else {
		free(buf, 0);
	}
	rw_lock_write_unlock(&deviceNode->v_bufobj.bo_lock.haikuRW);

	return B_OK;
}
