/* File System attribute queries
**
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_FS_QUERY_H
#define _FSSH_FS_QUERY_H


#include "fssh_os.h"
#include "fssh_dirent.h"


/* Flags for fs_open_[live_]query() */

#define FSSH_B_LIVE_QUERY			(1 << 0)
#define FSSH_B_QUERY_NON_INDEXED	(1 << 1)
#define FSSH_B_QUERY_WATCH_ALL		(1 << 2)


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
