/* General File System informations/capabilities
**
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_FS_INFO_H
#define	_FSSH_FS_INFO_H

#include "fssh_defs.h"
#include "fssh_os.h"


/* fs_info.flags */
#define	FSSH_B_FS_IS_READONLY		0x00000001
#define	FSSH_B_FS_IS_REMOVABLE		0x00000002
#define	FSSH_B_FS_IS_PERSISTENT		0x00000004
#define	FSSH_B_FS_IS_SHARED			0x00000008
#define	FSSH_B_FS_HAS_MIME			0x00010000
#define	FSSH_B_FS_HAS_ATTR			0x00020000
#define	FSSH_B_FS_HAS_QUERY			0x00040000
// those additions are preliminary and may be removed
#define FSSH_B_FS_HAS_SELF_HEALING_LINKS	0x00080000
#define FSSH_B_FS_HAS_ALIASES				0x00100000
#define FSSH_B_FS_SUPPORTS_NODE_MONITORING	0x00200000
#define FSSH_B_FS_SUPPORTS_MONITOR_CHILDREN	0x00400000

typedef struct fssh_fs_info {
	fssh_dev_t	dev;								/* volume dev_t */
	fssh_ino_t	root;								/* root ino_t */
	uint32_t	flags;								/* flags (see above) */
	fssh_off_t	block_size;							/* fundamental block size */
	fssh_off_t	io_size;							/* optimal i/o size */
	fssh_off_t	total_blocks;						/* total number of blocks */
	fssh_off_t	free_blocks;						/* number of free blocks */
	fssh_off_t	total_nodes;						/* total number of nodes */
	fssh_off_t	free_nodes;							/* number of free nodes */
	char		device_name[128];					/* device holding fs */
	char		volume_name[FSSH_B_FILE_NAME_LENGTH];	/* volume name */
	char		fsh_name[FSSH_B_OS_NAME_LENGTH];		/* name of fs handler */
} fssh_fs_info;


#ifdef  __cplusplus
extern "C" {
#endif

extern fssh_dev_t	dev_for_path(const char *path);
extern fssh_dev_t	next_dev(int32_t *pos);
extern int			fs_stat_dev(fssh_dev_t dev, fssh_fs_info *info);

#ifdef  __cplusplus
}
#endif

#endif	/* _FSSH_FS_INFO_H */
