/* File System volume functions
**
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _FS_VOLUME_H
#define	_FS_VOLUME_H

#include <OS.h>


/* mount flags */
#define B_MOUNT_READ_ONLY		1
#define B_MOUNT_VIRTUAL_DEVICE	2

/* unmount flags */
#define B_FORCE_UNMOUNT			1


#ifdef  __cplusplus
extern "C" {
#endif

extern status_t	fs_mount_volume(const char *filesystem, const char *where,
					const char *device, uint32 flags, const char *parameters);
extern status_t	fs_unmount_volume(const char *path, uint32 flags);

extern status_t fs_initialize_volume(const char *filesystem, const char *volumeName,
					const char *device, uint32 flags, const char *parameters);

#ifdef  __cplusplus
}
#endif

#endif	/* _FS_VOLUME_H */
