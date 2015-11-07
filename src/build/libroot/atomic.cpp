#include <BeOSBuildCompatibility.h>

#include <string.h>

#include <OS.h>
#include <SupportDefs.h>


#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)) \
	&& !defined(__clang__)


void
atomic_set(int32 *value, int32 newValue)
{
	*value = newValue;
}


int32
atomic_get_and_set(int32 *value, int32 newValue)
{
	int32 oldValue = *value;
	*value = newValue;
	return oldValue;
}


int32
atomic_test_and_set(int32 *value, int32 newValue, int32 testAgainst)
{
	int32 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	return oldValue;
}


int32
atomic_add(int32 *value, int32 addValue)
{
	int32 oldValue = *value;
	*value += addValue;
	return oldValue;
}


int32
atomic_and(int32 *value, int32 andValue)
{
	int32 oldValue = *value;
	*value &= andValue;
	return oldValue;
}


int32
atomic_or(int32 *value, int32 orValue)	
{
	int32 oldValue = *value;
	*value |= orValue;
	return oldValue;
}


int32
atomic_get(int32 *value)
{
	return *value;
}


void
atomic_set64(int64 *value, int64 newValue)
{
	*value = newValue;
}


int64
atomic_get_and_set64(int64 *value, int64 newValue)
{
	int64 oldValue = *value;
	*value = newValue;
	return oldValue;
}

int64
atomic_test_and_set64(int64 *value, int64 newValue, int64 testAgainst)
{
	int64 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	return oldValue;
}

int64
atomic_add64(int64 *value, int64 addValue)
{
	int64 oldValue = *value;
	*value += addValue;
	return oldValue;
}

int64
atomic_and64(int64 *value, int64 andValue)
{
	int64 oldValue = *value;
	*value &= andValue;
	return oldValue;
}

int64
atomic_or64(int64 *value, int64 orValue)
{
	int64 oldValue = *value;
	*value |= orValue;
	return oldValue;
}

int64
atomic_get64(int64 *value)
{
	return *value;
}


#endif	// __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)

