//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <OS.h>
#include "_libppputils.h"
#include <cstdlib>
#include <cstring>


// R5 only: strlcat is needed by driver_settings API
/** Concatenates the source string to the destination, writes
 *	as much as "maxLength" bytes to the dest string.
 *	Always null terminates the string as long as maxLength is
 *	larger than the dest string.
 *	Returns the length of the string that it tried to create
 *	to be able to easily detect string truncation.
 */
size_t
strlcat(char *dest, const char *source, size_t maxLength)
{
	size_t destLength = strnlen(dest, maxLength);
	
	// This returns the wrong size, but it's all we can do
	if (maxLength == destLength)
		return destLength + strlen(source);
	
	dest += destLength;
	maxLength -= destLength;
	
	size_t i = 0;
	for (; i < maxLength - 1 && source[i]; i++) {
		dest[i] = source[i];
	}
	
	dest[i] = '\0';
	
	return destLength + i + strlen(source + i);
}


char*
get_stack_driver_path()
{
	char *path;
	
	// user-defined stack driver path?
	path = getenv("NET_STACK_DRIVER_PATH");
	if(path)
		return path;
	
	// use the default stack driver path
	return NET_STACK_DRIVER_PATH;
}
