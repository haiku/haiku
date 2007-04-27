#include <BeOSBuildCompatibility.h>

#include <string.h>

#include <OS.h>
#include <SupportDefs.h>


int32
atomic_set(vint32 *value, int32 newValue)
{
	int32 oldValue = *value;
	*value = newValue;
	return oldValue;
}


int32
atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
	int32 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	return oldValue;
}


int32
atomic_add(vint32 *value, int32 addValue)
{
	int32 oldValue = *value;
	*value += addValue;
	return oldValue;
}


int32
atomic_and(vint32 *value, int32 andValue)
{
	int32 oldValue = *value;
	*value &= andValue;
	return oldValue;
}


int32
atomic_or(vint32 *value, int32 orValue)	
{
	int32 oldValue = *value;
	*value |= orValue;
	return oldValue;
}


int32
atomic_get(vint32 *value)
{
	return *value;
}
