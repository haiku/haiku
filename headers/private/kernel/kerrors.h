/* kerors.h
 *
 * Kernel ONLY error codes
 */

#ifndef _KERNEL_KERRORS_H
#define _KERNEL_KERRORS_H

/* These are the old newos errors - we should be trying to remove these
 * in favour of the codes in posix/errno.h or even os/support/Errors.h
 */

/* General errors */
#define ERR_UNIMPLEMENTED        ENOSYS

/* VFS errors */
#define ERR_VFS_NOT_MOUNTPOINT   -3072
#define ERR_VFS_ALREADY_MOUNTPOINT -3073

/* VM errors */
#define ERR_VM_GENERAL           -4096
#define ERR_VM_INVALID_ASPACE    ERR_VM_GENERAL-1
#define ERR_VM_INVALID_REGION    ERR_VM_GENERAL-2
#define ERR_VM_PAGE_NOT_PRESENT  ERR_VM_GENERAL-7
#define ERR_VM_NO_REGION_SLOT    ERR_VM_GENERAL-8

#endif /* _KERNEL_KERRORS_H */
