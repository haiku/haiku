
#include "lock.h"

int
beos_new_lock(beos_lock *l, const char *name)
{
	if (!l)
		return B_BAD_VALUE;

	l->s = create_sem(0, name);
	if (l->s < 0)
		return l->s;

	l->c = 1;
	return B_OK;
}

int
beos_free_lock(beos_lock *l)
{
	if (!l)
		return B_BAD_VALUE;

	delete_sem(l->s);
	return 0;
}

int
beos_new_mlock(beos_mlock *l, long c, const char *name)
{
	if (!l)
		return B_BAD_VALUE;

	l->s = create_sem(c, name);
	if (l->s < 0)
		return l->s;

	return B_OK;
}

int
beos_free_mlock(beos_mlock *l)
{
	if (!l)
		return B_BAD_VALUE;

	delete_sem(l->s);
	return 0;
}

