/* socket.h
 * Very simply file. This will grow!
 */

/**
 * @file ksocket.h
 * @brief Kernel socket definitions
 * These never need to be seen outside of the kernel and no user
 * application should ever include this file.
 */

#ifndef _KERNEL_KSOCKET_H
#define _KERNEL_KSOCKET_H

/**
 * @defgroup KSOCKET Kernel Socket Definitions
 * @brief Kernel socket prototypes and definitions
 * @ingroup OpenBeOS_Kernel
 * @{
 */
 
#ifdef _KERNEL_MODE
int socket(int, int, int, bool);



#endif
/** @} */

#endif	/* _KERNEL_KSOCKET_H */
