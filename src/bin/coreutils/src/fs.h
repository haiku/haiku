/* Define the magic numbers as given by statfs(2).
   Please send additions to bug-coreutils@gnu.org and meskes@debian.org.
   This file is generated automatically from ./stat.c. */

#if defined __linux__
# define S_MAGIC_ADFS 0xADF5
# define S_MAGIC_AFFS 0xADFF
# define S_MAGIC_AUTOFS 0x187
# define S_MAGIC_BEFS 0x42465331
# define S_MAGIC_BFS 0x1BADFACE
# define S_MAGIC_BINFMT_MISC 0x42494e4d
# define S_MAGIC_CODA 0x73757245
# define S_MAGIC_COH 0x012FF7B7
# define S_MAGIC_CRAMFS 0x28CD3D45
# define S_MAGIC_DEVFS 0x1373
# define S_MAGIC_DEVPTS 0x1CD1
# define S_MAGIC_EFS 0x414A53
# define S_MAGIC_EXT 0x137D
# define S_MAGIC_EXT2 0xEF53
# define S_MAGIC_EXT2_OLD 0xEF51
# define S_MAGIC_FAT 0x4006
# define S_MAGIC_FUSECTL 0x65735543
# define S_MAGIC_HPFS 0xF995E849
# define S_MAGIC_HUGETLBFS 0x958458f6
# define S_MAGIC_ISOFS 0x9660
# define S_MAGIC_ISOFS_R_WIN 0x4004
# define S_MAGIC_ISOFS_WIN 0x4000
# define S_MAGIC_JFFS2 0x72B6
# define S_MAGIC_JFFS 0x07C0
# define S_MAGIC_JFS 0x3153464A
# define S_MAGIC_LUSTRE 0x0BD00BD0
# define S_MAGIC_MINIX 0x137F
# define S_MAGIC_MINIX_30 0x138F
# define S_MAGIC_MINIX_V2 0x2468
# define S_MAGIC_MINIX_V2_30 0x2478
# define S_MAGIC_MSDOS 0x4D44
# define S_MAGIC_NCP 0x564C
# define S_MAGIC_NFS 0x6969
# define S_MAGIC_NFSD 0x6E667364
# define S_MAGIC_NTFS 0x5346544E
# define S_MAGIC_OPENPROM 0x9fa1
# define S_MAGIC_PROC 0x9FA0
# define S_MAGIC_QNX4 0x002F
# define S_MAGIC_RAMFS 0x858458F6
# define S_MAGIC_REISERFS 0x52654973
# define S_MAGIC_ROMFS 0x7275
# define S_MAGIC_SMB 0x517B
# define S_MAGIC_SQUASHFS 0x73717368
# define S_MAGIC_SYSFS 0x62656572
# define S_MAGIC_SYSV2 0x012FF7B6
# define S_MAGIC_SYSV4 0x012FF7B5
# define S_MAGIC_TMPFS 0x1021994
# define S_MAGIC_UDF 0x15013346
# define S_MAGIC_UFS 0x00011954
# define S_MAGIC_UFS_BYTESWAPPED 0x54190100
# define S_MAGIC_USBDEVFS 0x9FA2
# define S_MAGIC_VXFS 0xA501FCF5
# define S_MAGIC_XENIX 0x012FF7B4
# define S_MAGIC_XFS 0x58465342
# define S_MAGIC_XIAFS 0x012FD16D
#elif defined __GNU__
# include <hurd/hurd_types.h>
#endif
