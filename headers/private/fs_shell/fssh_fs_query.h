/* File System attribute queries
**
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_FS_QUERY_H
#define _FSSH_FS_QUERY_H


#include "fssh_os.h"
#include "fssh_dirent.h"


/* Flags for fs_open_[live_]query() */

#define FSSH_B_LIVE_QUERY			0x00000001
	// Note, if you specify B_LIVE_QUERY, you have to use fs_open_live_query();
	// it will be ignored in fs_open_query().
#define FSSH_B_QUERY_NON_INDEXED	0x00000002
	// Only enable this feature for non time-critical things, it might
	// take a long time to proceed.
	// Also, not every file system might support this feature.


#ifdef  __cplusplus
extern "C" {
#endif

extern fssh_DIR*			fssh_fs_open_query(fssh_dev_t device,
									const char *query, uint32_t flags);
extern fssh_DIR*			fssh_fs_open_live_query(fssh_dev_t device,
									const char *query, uint32_t flags,
									fssh_port_id port, int32_t token);
extern int					fssh_fs_close_query(fssh_DIR *d);
extern struct fssh_dirent*	fssh_fs_read_query(fssh_DIR *d);

extern fssh_status_t		fssh_get_path_for_dirent(struct fssh_dirent *dent,
								char *buf, fssh_size_t len);

#ifdef  __cplusplus
}
#endif

#endif	/* _FSSH_FS_QUERY_H */
