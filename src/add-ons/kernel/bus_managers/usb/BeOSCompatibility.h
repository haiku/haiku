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
#define B_SPINLOCK_INITIALIZER	0
#define PCI_usb_ehci			0x20

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

// wrong, but it won't change for BeOS anymore
typedef uint32 addr_t;


typedef struct mutex {
	sem_id	sem;
	int32	count;
} mutex;


inline status_t
mutex_init(mutex *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return B_BAD_VALUE;

	ben->count = 1;
	ben->sem = create_sem(0, name);
	if (ben->sem >= B_OK)
		return B_OK;

	return ben->sem;
}


#define MUTEX_FLAG_CLONE_NAME 1
inline status_t
mutex_init_etc(mutex *ben, const char *name, int32 flags)
{
	return mutex_init(ben, name);
}


inline void
mutex_destroy(mutex *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


inline status_t
mutex_lock(mutex *ben)
{
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem(ben->sem);
	return B_OK;
}


inline status_t
mutex_unlock(mutex *ben)
{
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);
	return B_OK;
}


class ConditionVariableEntry {
public:
	ConditionVariableEntry()
		:	fSem(-1)
	{
	}

	~ConditionVariableEntry()
	{
		if (fSem >= 0)
			delete_sem(fSem);
	}

	sem_id Added()
	{
		if (fSem < 0)
			fSem = create_sem(0, "condvar entry");
		return fSem;
	}

	status_t Wait(uint32 flags, bigtime_t timeout)
	{
		return acquire_sem_etc(fSem, 1, flags, timeout);
	}

private:
	sem_id					fSem;
};


class ConditionVariable {
public:
	ConditionVariable()
		:	fObject(NULL),
			fName("condvar"),
			fSemCount(0)
	{
	}

	void Init(void *object, const char *name)
	{
		fObject = object;
		fName = name;
	}

	void Add(ConditionVariableEntry *entry)
	{
		fSems[fSemCount++] = entry->Added();
	}

	void NotifyAll()
	{
		int32 semCount = fSemCount;
		sem_id sems[semCount];
		memcpy(sems, fSems, sizeof(sem_id) * semCount);
		fSemCount = 0;

		for (int32 i = 0; i < semCount; i++)
			release_sem(sems[i]);
	}

private:
	void *		fObject;
	const char *fName;
	sem_id		fSems[30];
	int32		fSemCount;
};


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


inline int32
atomic_get(vint32 *value)
{
	return atomic_or(value, 0);
}


inline int32
atomic_set(vint32 *value, int32 setValue)
{
	int32 result = atomic_and(value, 0);
	int32 previous = atomic_add(value, setValue);
	if (previous != 0)
		result = previous;
	return result;
}


inline status_t
user_memcpy(void *target, void *source, size_t length)
{
	memcpy(target, source, length);
	return B_OK;
}


#undef B_KERNEL_READ_AREA
#define B_KERNEL_READ_AREA 0
#undef B_KERNEL_WRITE_AREA
#define B_KERNEL_WRITE_AREA 0

#endif // !HAIKU_TARGET_PLATFORM_HAIKU
#endif // !_USB_BEOS_COMPATIBILITY_H_
