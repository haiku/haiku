#ifndef _BEOS_COMPATIBILITY_H_
#define _BEOS_COMPATIBILITY_H_
#ifndef HAIKU_TARGET_PLATFORM_HAIKU

typedef struct benaphore {
	sem_id	sem;
	int32	count;
} benaphore;


static inline status_t
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


static inline void
benaphore_destroy(benaphore *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


static inline status_t
benaphore_lock(benaphore *ben)
{
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem(ben->sem);

	return B_OK;
}


static inline status_t
benaphore_unlock(benaphore *ben)
{
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);

	return B_OK;
}

#endif // HAIKU_TARGET_PLATFORM_HAIKU
#endif // _BEOS_COMPATIBILITY_H_
