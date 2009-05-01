/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Locale.h>
#include <langinfo.h>


char *
nl_langinfo(nl_item id)
{
	if (be_locale != NULL)
		be_locale->GetString(id);

	// ToDo: return default values!	
	return "";
}

