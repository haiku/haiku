#ifndef _USB_BEOS_COMPATIBILITY_H_
#define _USB_BEOS_COMPATIBILITY_H_
#ifndef HAIKU_TARGET_PLATFORM_HAIKU

// prevent inclusion of original lock.h as it conflicts with what we have here
#define _KERNEL_LOCK_H

#include <stdarg.h>
#include <stdio.h>
#include <OS.h>

#define IS_USER_ADDRESS(x)		(((uint32)x & 0x80000000) > 0)
#define IS_KERNEL_ADDRESS(x)	(((uint32)x & 0x80000000) == 0)

#ifndef HAIKU_TARGET_PLATFORM_DANO
enum {
	B_DEV_INVALID_PIPE = B_DEV_DOOR_OPEN + 1,
	B_DEV_CRC_ERROR,
	B_DEV_STALLED,
	B_DEV_BAD_PID,
	B_DEV_UNEXPECTED_PID,
	B_DEV_DATA_OVERRUN,
	B_DEV_DATA_UNDERRUN,
	B_DEV_FIFO_OVERRUN,
	B_DEV_FIFO_UNDERRUN,
	B_DEV_PENDING,
	B_DEV_MULTIPLE_ERRORS,
	B_DEV_TOO_LATE,
};
#endif


typedef struct benaphore {
	sem_id	sem;
	int32	count;
} benaphore;


inline status_t
benaphore_init(benaphore *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return B_BAD_VALUE;

	ben->count = 1;
	ben->sem = create_sem(0, name);
	if (ben->sem >= B_OK)
		return B_OK;

	return ben->sem;
}


inline void
benaphore_destroy(benaphore *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


inline status_t
benaphore_lock(benaphore *ben)
{
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem(ben->sem);
	return B_OK;
}


inline status_t
benaphore_unlock(benaphore *ben)
{
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);
	return B_OK;
}


inline void
load_driver_symbols(char *driver)
{
	/* nothing */
}


inline int
snprintf(char *buffer, size_t bufferSize, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vsprintf(buffer, format, args);
	va_end(args);
	return result;
}

#undef B_KERNEL_READ_AREA
#define B_KERNEL_READ_AREA 0
#undef B_KERNEL_WRITE_AREA
#define B_KERNEL_WRITE_AREA 0

#endif // !HAIKU_TARGET_PLATFORM_HAIKU
#endif // !_USB_BEOS_COMPATIBILITY_H_
