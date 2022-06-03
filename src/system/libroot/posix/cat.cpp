/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <nl_types.h>


nl_catd 
catopen(const char *name, int oflag)
{
	return (nl_catd)1;
}


char *
catgets(nl_catd cat, int setID, int msgID, const char *defaultMessage)
{
	// should return "const char *"...
	return const_cast<char *>(defaultMessage);
}


int 
catclose(nl_catd)
{
	return 0;
}

