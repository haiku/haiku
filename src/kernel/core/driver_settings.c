/* driver_settings - implements the driver settings API
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <driver_settings.h>


// ToDo: implement the driver settings! It currently only exports the API
// so that drivers will find it


status_t
unload_driver_settings(void *handle)
{
	return B_BAD_VALUE;
}


void *
load_driver_settings(const char *driverName)
{
	return NULL;
}


bool
get_driver_boolean_parameter(void *handle, const char *keyName, bool unknownValue, bool noArgValue)
{
	return unknownValue;
}


const char *
get_driver_parameter(void *handle, const char *keyName, const char *unknownValue, const char *noArgValue)
{
	return unknownValue;
}


const driver_settings *
get_driver_settings(void *handle)
{
	return NULL;
}

