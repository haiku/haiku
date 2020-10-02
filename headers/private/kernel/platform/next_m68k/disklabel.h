/* adapted from NeXT headers:
 * usr/include/sys/disktab.h
 * usr/include/nextdev/disk.h
 * simplified since we don't ware about disktab, and version < 3.
 * see also:
 * https://doxygen.reactos.org/d7/d53/bootblock_8h.html
 */

#include <sys/types.h>

/* actually not packed, but aligned at 2 bytes */
#pragma pack(push,2)

struct disk_label {
#define	DL_V1		0x4e655854	/* version #1: "NeXT" */
#define	DL_V2		0x646c5632	/* version #2: "dlV2" */
#define	DL_V3		0x646c5633	/* version #3: "dlV3" */
#define	DL_VERSION	DL_V3		/* default version */
	int	dl_version;		/* label version number */
	int	dl_label_blkno;		/* block # where this label is */
	int	dl_size;		/* size of media area (sectors) */
#define	MAXLBLLEN	24
	char	dl_label[MAXLBLLEN];	/* media label */
	u_int32_t	dl_flags;		/* flags */
#define	DL_UNINIT	0x80000000	/* label is uninitialized */
	u_int32_t	dl_tag;			/* volume tag */
	/* the rest really is a struct	disktab dl_dt;*/		/* common info in disktab */
#define	MAXDNMLEN	24
	char	dl_name[MAXDNMLEN];	/* drive name */
#define	MAXTYPLEN	24
	char	dl_type[MAXTYPLEN];	/* drive type */
	int	dl_secsize;		/* sector size in bytes */
	int	dl_ntrack;		/* # tracks/cylinder */
	int	dl_nsect;		/* # sectors/track */
	int	dl_ncyl;		/* # cylinders */
	int	dl_rpm;			/* revolutions/minute */
	short	dl_front;		/* size of front porch (sectors) */
	short	dl_back;			/* size of back porch (sectors) */
	short	dl_ngroups;		/* number of alt groups */
	short	dl_ag_size;		/* alt group size (sectors) */
	short	dl_ag_alts;		/* alternate sectors / alt group */
	short	dl_ag_off;		/* sector offset to first alternate */
#define	NBOOTS	2
	int	dl_boot0_blkno[NBOOTS];	/* "blk 0" boot locations */
#define	MAXBFLEN 24
	char	dl_bootfile[MAXBFLEN];	/* default bootfile */
#define	MAXHNLEN 32
	char	dl_hostname[MAXHNLEN];	/* host name */
	char	dl_rootpartition;	/* root partition e.g. 'a' */
	char	dl_rwpartition;		/* r/w partition e.g. 'b' */
#define	NPART	8
	struct	partition {
		int	p_base;		/* base sector# of partition */
		int	p_size;		/* #sectors in partition */
		short	p_bsize;	/* block size in bytes */
		short	p_fsize;	/* frag size in bytes */
		char	p_opt;		/* 's'pace/'t'ime optimization pref */
		short	p_cpg;		/* cylinders per group */
		short	p_density;	/* bytes per inode density */
		char	p_minfree;	/* minfree (%) */
		char	p_newfs;	/* run newfs during init */
#define	MAXMPTLEN	16
		char	p_mountpt[MAXMPTLEN];/* mount point */
		char	p_automnt;	/* auto-mount when inserted */
#define	MAXFSTLEN	8
		char	p_type[MAXFSTLEN];/* file system type */
	} dl_part[NPART];

	/*
	 *  if dl_version >= DL_V3 then the bad block table is relocated
	 *  to a structure separate from the disk label.
	 */
	union {
		u_int16_t	DL_v3_checksum;
#define	NBAD	1670			/* sized to make label ~= 8KB */
		int	DL_bad[NBAD];	/* block number that is bad */
	} dl_un;
#define	dl_v3_checksum	dl_un.DL_v3_checksum
#define	dl_bad		dl_un.DL_bad
	u_int16_t	dl_checksum;		/* ones complement checksum */

	/* add things here so dl_checksum stays in a fixed place */
};

#pragma pack(pop)

