/* File System device functions
**
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _FS_DEVICE_H
#define	_FS_DEVICE_H

#include <OS.h>


/* mount flags */
#define B_MOUNT_READ_ONLY		1
#define B_MOUNT_VIRTUAL_DEVICE	2


#ifdef  __cplusplus
extern "C" {
#endif

extern status_t	fs_mount_device(const char *filesystem, const char *where, const char *device,
					uint32 flags, const char *parameters);
extern status_t	fs_unmount_device(const char *path);

extern status_t fs_initialize_device(const char *filesystem, const char *device, uint32 flags,
					const char *parameters);

#ifdef  __cplusplus
}
#endif

#endif	/* _FS_DEVICE_H */
