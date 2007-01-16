#ifndef _USB_BEOS_COMPATIBILITY_H_
#define _USB_BEOS_COMPATIBILITY_H_
#ifndef HAIKU_TARGET_PLATFORM_HAIKU

#include <lock.h>
#include <stdarg.h>
#include <stdio.h>


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

#endif // !HAIKU_TARGET_PLATFORM_HAIKU
#endif // !_USB_BEOS_COMPATIBILITY_H_
