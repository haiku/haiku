#ifndef _FILE_SHARES_H_
#define _FILE_SHARES_H_

#include "betalk.h"

#define BT_MAX_FILE_SHARES	128

typedef struct userRights
{
	char *user;
	int rights;
	bool isGroup;
	struct userRights *next;
} bt_user_rights;

typedef struct fileShare
{
	char path[B_PATH_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];

	bool used;
	bool readOnly;

	// What rights does each user have?
	bt_user_rights *rights;
	int security;

	struct fileShare *next;
} bt_fileShare_t;

#endif
