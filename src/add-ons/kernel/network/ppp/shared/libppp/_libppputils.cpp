//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <OS.h>
#include "_libppputils.h"
#include <cstdlib>
#include <cstring>


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
