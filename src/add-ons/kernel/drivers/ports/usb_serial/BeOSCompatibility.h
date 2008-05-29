#ifndef _BEOS_COMPATIBILITY_H_
#define _BEOS_COMPATIBILITY_H_
#ifndef HAIKU_TARGET_PLATFORM_HAIKU

typedef struct mutex {
	sem_id	sem;
	int32	count;
} mutex;


static inline status_t
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


static inline void
mutex_destroy(mutex *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


static inline status_t
mutex_lock(mutex *ben)
{
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem(ben->sem);

	return B_OK;
}


static inline status_t
mutex_unlock(mutex *ben)
{
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);

	return B_OK;
}

#endif // HAIKU_TARGET_PLATFORM_HAIKU
#endif // _BEOS_COMPATIBILITY_H_
