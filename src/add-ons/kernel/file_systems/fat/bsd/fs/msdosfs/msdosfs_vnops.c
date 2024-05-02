/* $FreeBSD$ */
/*	$NetBSD: msdosfs_vnops.c,v 1.68 1998/02/10 14:10:04 mrg Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */


// Modified to support the Haiku FAT driver.

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/vnode.h"

#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"


int msdosfs_bmap(struct vnode* a_vp, daddr_t a_bn, struct bufobj** a_bop, daddr_t* a_bnp,
	int* a_runp, int* a_runb);


/*
 * Some general notes:
 *
 * In the ufs filesystem the inodes, superblocks, and indirect blocks are
 * read/written using the vnode for the filesystem. Blocks that represent
 * the contents of a file are read/written using the vnode for the file
 * (including directories when they are read/written as files). This
 * presents problems for the dos filesystem because data that should be in
 * an inode (if dos had them) resides in the directory itself. Since we
 * must update directory entries without the benefit of having the vnode
 * for the directory we must use the vnode for the filesystem. This means
 * that when a directory is actually read/written (via read, write, or
 * readdir, or seek) we must use the vnode for the filesystem instead of
 * the vnode for the directory as would happen in ufs.
 */


/*!
	@param a_vp pointer to the file's vnode
	@param a_bn logical block number within the file (cluster number for us)
	@param a_bop where to return the bufobj of the special file containing the fs
	@param a_bnp where to return the "physical" block number corresponding to a_bn
		(relative to the special file; units are blocks of size DEV_BSIZE)
	@param a_runp where to return the "run past" a_bn. This is the count of logical blocks whose
		physical blocks (together with a_bn's physical block) are contiguous.
	@param a_runb where to return the "run before" a_bn.
*/
int
msdosfs_bmap(struct vnode* a_vp, daddr_t a_bn, struct bufobj** a_bop, daddr_t* a_bnp, int* a_runp,
	int* a_runb)
{
	struct fatcache savefc;
	struct denode* dep;
	struct mount* mp;
	struct msdosfsmount* pmp;
	struct vnode* vp;
	daddr_t runbn;
	u_long cn;
	int bnpercn, error, maxio, maxrun, run;

	vp = a_vp;
	dep = VTODE(vp);
	pmp = dep->de_pmp;
	if (a_bop != NULL)
		*a_bop = &pmp->pm_devvp->v_bufobj;
	if (a_bnp == NULL)
		return 0;
	if (a_runp != NULL)
		*a_runp = 0;
	if (a_runb != NULL)
		*a_runb = 0;
	cn = a_bn;
	if (cn != (u_long)a_bn)
		return EFBIG;
	error = pcbmap(dep, cn, a_bnp, NULL, NULL);
	if (error != 0 || (a_runp == NULL && a_runb == NULL))
		return error;

	/*
	 * Prepare to back out updates of the fatchain cache after the one
	 * for the first block done by pcbmap() above. Without the backout,
	 * then whenever the caller doesn't do i/o to all of the blocks that
	 * we find, the single useful cache entry would be too far in advance
	 * of the actual i/o to work for the next sequential i/o. Then the
	 * FAT would be searched from the beginning. With the backout, the
	 * FAT is searched starting at most a few blocks early. This wastes
	 * much less time. Time is also wasted finding more blocks than the
	 * caller will do i/o to. This is necessary because the runlength
	 * parameters are output-only.
	 */
	savefc = dep->de_fc[FC_LASTMAP];

	mp = vp->v_mount;
	maxio = mp->mnt_iosize_max / mp->mnt_stat.f_iosize;
	bnpercn = de_cn2bn(pmp, 1);
	if (a_runp != NULL) {
		maxrun = ulmin(maxio - 1, pmp->pm_maxcluster - cn);
		for (run = 1; run <= maxrun; run++) {
			if (pcbmap(dep, cn + run, &runbn, NULL, NULL) != 0 || runbn != *a_bnp + run * bnpercn)
				break;
		}
		*a_runp = run - 1;
	}
	if (a_runb != NULL) {
		maxrun = ulmin(maxio - 1, cn);
		for (run = 1; run < maxrun; run++) {
			if (pcbmap(dep, cn - run, &runbn, NULL, NULL) != 0 || runbn != *a_bnp - run * bnpercn)
				break;
		}
		*a_runb = run - 1;
	}
	dep->de_fc[FC_LASTMAP] = savefc;
	return 0;
}


/* Global vfs data structures for msdosfs */
struct vop_vector msdosfs_vnodeops;
