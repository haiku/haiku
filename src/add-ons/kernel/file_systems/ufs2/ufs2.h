/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef UFS2_H
#define UFS2_H

#include "system_dependencies.h"

#define UFS2_SUPER_BLOCK_MAGIC		0x19540119
#define UFS2_SUPER_BLOCK_OFFSET		65536
#define MAXMNTLEN					512
#define	MAXCSBUFS					32

struct ufs2_super_block {
	uint32	ufs2_link;		/* linked list of file systems */
	uint32	ufs2_rolled;		/* logging only: fs fully rolled */
	uint32	ufs2_sblkno;		/* addr of super-block in filesys */
	uint32	ufs2_cblkno;		/* offset of cyl-block in filesys */
	uint32	ufs2_iblkno;		/* offset of inode-blocks in filesys */
	uint32	ufs2_dblkno;		/* offset of first data after cg */
	uint32	ufs2_cgoffset;		/* cylinder group offset in cylinder */
	uint32	ufs2_cgmask;		/* used to calc mod ufs2_ntrak */
	uint64	ufs2_time;		/* last time written */
	uint64	ufs2_size;		/* number of blocks in fs */
	uint64	ufs2_dsize;		/* number of data blocks in fs */
	uint32	ufs2_ncg;			/* number of cylinder groups */
	uint32	ufs2_bsize;		/* size of basic blocks in fs */
	uint32	ufs2_fsize;		/* size of frag blocks in fs */
	uint32	ufs2_frag;		/* number of frags in a block in fs */
	uint32	ufs2_minfree;		/* minimum percentage of free blocks */
	uint32	ufs2_rotdelay;		/* num of ms for optimal next block */
	uint32	ufs2_rps;			/* disk revolutions per second */
	/* these fields can be computed from the others */
	uint32	ufs2_bmask;		/* ``blkoff'' calc of blk offsets */
	uint32	ufs2_fmask;		/* ``fragoff'' calc of frag offsets */
	uint32	ufs2_bshift;		/* ``lblkno'' calc of logical blkno */
	uint32	ufs2_fshift;		/* ``numfrags'' calc number of frags */
	/* these are configuration parameters */
	uint32	ufs2_maxcontig;		/* max number of contiguous blks */
	uint32	ufs2_maxbpg;		/* max number of blks per cyl group */
	/* these fields can be computed from the others */
	uint32	ufs2_fragshift;		/* block to frag shift */
	uint32	ufs2_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	uint32	ufs2_sbsize;		/* actual size of super block */
	uint32	ufs2_csmask;		/* csum block offset */
	uint32	ufs2_csshift;		/* csum block number */
	uint32	ufs2_nindir;		/* value of NINDIR */
	uint32	ufs2_inopb;		/* value of INOPB */
	uint32	ufs2_nspf;		/* value of NSPF */
	/* yet another configuration parameter */
	uint32	ufs2_optim;		/* optimization preference, see below */
	/* these fields are derived from the hardware */
	/* USL SVR4 compatibility */
#ifdef _LITTLE_ENDIAN
			/*
			 *	 * USL SVR4 compatibility
			 */
			uint32	ufs2_state;		/* file system state time stamp */
#else
		uint32	ufs2_npsect;		/* # sectors/track including spares */
#endif
	uint32 ufs2_si;			/* summary info state - lufs only */
	uint32	ufs2_trackskew;		/* sector 0 skew, per track */
	uint32	ufs2_id[2];		/* file system id */
	/* sizes determined by number of cylinder groups and their sizes */
	uint64	ufs2_csaddr;		/* blk addr of cyl grp summary area */
	uint32		ufs2_cssize;		/* size of cyl grp summary area */
	uint32	ufs2_cgsize;		/* cylinder group size */
	/* these fields are derived from the hardware */
	uint32	ufs2_ntrak;		/* tracks per cylinder */
	uint32	ufs2_nsect;		/* sectors per track */
	uint32	ufs2_spc;			/* sectors per cylinder */
	/* this comes from the disk driver partitioning */
	uint32	ufs2_ncyl;		/* cylinders in file system */
	/* these fields can be computed from the others */
	uint32	ufs2_cpg;			/* cylinders per group */
	uint32	ufs2_ipg;			/* inodes per group */
	uint32	ufs2_fpg;			/* blocks per group * ufs2_frag */
	/* these fields are cleared at mount time */
	uint32	ufs2_fmod;		/* super block modified flag */
	uint32	ufs2_clean;		/* file system state flag */
	uint32	ufs2_ronly;		/* mounted read-only flag */
	uint32	ufs2_flags;		/* largefiles flag, etc. */
	uint32	ufs2_fsmnt[MAXMNTLEN];	/* name mounted on */
	/* these fields retain the current block allocation info */
	uint32	ufs2_cgrotor;		/* last cg searched */
	union {				/* ufs2_cs (csum) info */
		uint32 ufs2_csp_pad[MAXCSBUFS];
		struct csum *ufs2_csp;
	} ufs2_u;
	uint32	ufs2_cpc;			/* cyl per cycle in postbl */
	uint64	ufs2_opostbl[16][8];	/* old rotation block list head */
	uint32	ufs2_sparecon[51];	/* reserved for future constants */
	uint32	ufs2_version;		/* minor version of ufs */
	uint32	ufs2_logbno;		/* block # of embedded log */
	uint32	ufs2_reclaim;		/* reclaim open, deleted files */
	uint32	ufs2_sparecon2;		/* reserved for future constant */
#ifdef _LITTLE_ENDIAN
	/* USL SVR4 compatibility */
	uint32	ufs2_npsect;		/* # sectors/track including spares */
#else
	uint32	ufs2_state;		/* file system state time stamp */
#endif
	uint64	ufs2_qbmask;		/* ~ufs2_bmask - for use with quad size */
	uint64	ufs2_qfmask;		/* ~ufs2_fmask - for use with quad size */
	uint32	ufs2_postblformat;	/* format of positional layout tables */
	uint32	ufs2_nrpos;		/* number of rotaional positions */
	uint32	ufs2_postbloff;		/* (short) rotation block list head */
	uint32	ufs2_rotbloff;		/* (uchar_t) blocks for each rotation */
	uint32	ufs2_magic;		/* magic number */
	uint32	ufs2_space[1];		/* list of blocks for each rotation */
	bool	IsValid();
};

#endif
