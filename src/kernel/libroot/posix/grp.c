/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

/* Dumb group functions - only know the "users" group with group ID 0 right
 * now - this should definitely be thought over :-)
 */


#include <SupportDefs.h>

#include <grp.h>
#include <string.h>


static int32 sGroupIndex;
static struct group sGroup;


static void
get_users(struct group *gr)
{
	static char *nothing = "";

	gr->gr_name = "users";
	gr->gr_passwd = NULL;
	gr->gr_gid = 0;
	gr->gr_mem = &nothing;
}


static void
init_users()
{
	if (sGroupIndex == 0) {
		// initial construction of the group
		get_users(&sGroup);
		sGroupIndex = 1;
	}
}


struct group *
getgrgid(gid_t gid)
{
	if (gid != 0)
		return NULL;

	init_users();
	return &sGroup;
}


int
getgrgid_r(gid_t gid, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	if (gid != 0) {
		*_result = NULL;
		return B_ENTRY_NOT_FOUND;
	}

	get_users(group);
	*_result = group;
	return B_OK;
}


struct group *
getgrnam(const char *name)
{
	if (strcmp("users", name))
		return NULL;

	init_users();
	return &sGroup;
}


int
getgrnam_r(const char *name, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	if (strcmp("users", name)) {
		*_result = NULL;
		return B_ENTRY_NOT_FOUND;
	}

	get_users(group);
	*_result = group;
	return B_OK;
}


struct group *
getgrent(void)
{
	if (sGroupIndex > 1)
		return NULL;

	init_users();
	sGroupIndex++;

	return &sGroup;
}


void
setgrent(void)
{
	init_users();
}


void
endgrent(void)
{
	init_users();
}

