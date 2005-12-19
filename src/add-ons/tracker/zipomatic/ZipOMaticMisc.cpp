// license: public domain
// authors: jonas.sundstrom@kirilla.com

#include "ZipOMaticMisc.h"

#include <Debug.h>

status_t  
find_and_create_directory (directory_which a_which, BVolume * a_volume, const char * a_relative_path, BPath * a_full_path)
{
	status_t	status	=	B_OK;
	BPath		path;
	mode_t		mask	=	0;
	
	status	=	find_directory (a_which, & path, true, a_volume); // create = true

	if (status != B_OK)
		return status;
	
	if (a_relative_path != NULL)
	{
		path.Append(a_relative_path);
		
		mask =	umask (0);
		umask (mask);
	
		status	=	create_directory (path.Path(), mask);
		if (status != B_OK)
			return status;
	}
	
	if (a_full_path != NULL)
	{
		status	=	a_full_path->SetTo(path.Path());
		if (status != B_OK)
			return status;
	}
	
	return B_OK;
}

void 
error_message	(const char * a_text, int32 a_status)
{
	PRINT(("%s: %s\n", a_text, strerror(a_status)));
}

