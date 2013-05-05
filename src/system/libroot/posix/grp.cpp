/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <grp.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <OS.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <RegistrarDefs.h>
#include <user_group.h>

#include <util/KMessage.h>


using BPrivate::UserGroupLocker;
using BPrivate::relocate_pointer;


static KMessage sGroupDBReply;
static group** sGroupEntries = NULL;
static size_t sGroupEntryCount = 0;
static size_t sIterationIndex = 0;

static struct group sGroupBuffer;
static char sGroupStringBuffer[MAX_GROUP_BUFFER_SIZE];


static status_t
query_group_entry(const char* name, gid_t _gid, struct group *group,
	char *buffer, size_t bufferSize, struct group **_result)
{
	*_result = NULL;

	KMessage message(BPrivate::B_REG_GET_GROUP);
	if (name)
		message.AddString("name", name);
	else
		message.AddInt32("gid", _gid);

	KMessage reply;
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		reply);
	if (error != B_OK)
		return error;

	int32 gid;
	const char* password;

	if ((error = reply.FindInt32("gid", &gid)) != B_OK
		|| (error = reply.FindString("name", &name)) != B_OK
		|| (error = reply.FindString("password", &password)) != B_OK) {
		return error;
	}

	const char* members[MAX_GROUP_MEMBER_COUNT];
	int memberCount = 0;
	for (int memberCount = 0; memberCount < MAX_GROUP_MEMBER_COUNT;) {
		if (reply.FindString("members", members + memberCount) != B_OK)
			break;
		memberCount++;
	}

	error = BPrivate::copy_group_to_buffer(name, password, gid, members,
		memberCount, group, buffer, bufferSize);
	if (error == B_OK)
		*_result = group;

	return error;
}


static status_t
init_group_db()
{
	if (sGroupEntries != NULL)
		return B_OK;

	// ask the registrar
	KMessage message(BPrivate::B_REG_GET_GROUP_DB);
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		sGroupDBReply);
	if (error != B_OK)
		return error;

	// unpack the reply
	int32 count;
	group** entries;
	int32 numBytes;
	if ((error = sGroupDBReply.FindInt32("count", &count)) != B_OK
		|| (error = sGroupDBReply.FindData("entries", B_RAW_TYPE,
				(const void**)&entries, &numBytes)) != B_OK) {
		return error;
	}

	// relocate the entries
	addr_t baseAddress = (addr_t)entries;
	for (int32 i = 0; i < count; i++) {
		group* entry = relocate_pointer(baseAddress, entries[i]);
		relocate_pointer(baseAddress, entry->gr_name);
		relocate_pointer(baseAddress, entry->gr_passwd);
		relocate_pointer(baseAddress, entry->gr_mem);
		int32 k = 0;
		for (; entry->gr_mem[k] != (void*)-1; k++)
			relocate_pointer(baseAddress, entry->gr_mem[k]);
		entry->gr_mem[k] = NULL;
	}

	sGroupEntries = entries;
	sGroupEntryCount = count;

	return B_OK;
}


// #pragma mark -


struct group*
getgrent(void)
{
	struct group* result = NULL;
	int status = getgrent_r(&sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getgrent_r(struct group* group, char* buffer, size_t bufferSize,
	struct group** _result)
{
	UserGroupLocker _;

	int status = B_NO_MEMORY;

	*_result = NULL;

	if ((status = init_group_db()) == B_OK) {
		if (sIterationIndex >= sGroupEntryCount)
			return ENOENT;

		status = BPrivate::copy_group_to_buffer(
			sGroupEntries[sIterationIndex], group, buffer, bufferSize);

		if (status == B_OK) {
			sIterationIndex++;
			*_result = group;
		}
	}

	return status;
}


void
setgrent(void)
{
	UserGroupLocker _;

	sIterationIndex = 0;
}


void
endgrent(void)
{
	UserGroupLocker locker;

	sGroupDBReply.Unset();
	sGroupEntries = NULL;
	sGroupEntryCount = 0;
	sIterationIndex = 0;
}


struct group *
getgrnam(const char *name)
{
	struct group* result = NULL;
	int status = getgrnam_r(name, &sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getgrnam_r(const char *name, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	return query_group_entry(name, 0, group, buffer, bufferSize, _result);
}


struct group *
getgrgid(gid_t gid)
{
	struct group* result = NULL;
	int status = getgrgid_r(gid, &sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getgrgid_r(gid_t gid, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	return query_group_entry(NULL, gid, group, buffer, bufferSize, _result);
}


int
getgrouplist(const char* user, gid_t baseGroup, gid_t* groupList,
	int* groupCount)
{
	int maxGroupCount = *groupCount;
	*groupCount = 0;

	status_t error = B_OK;

	// prepare request
	KMessage message(BPrivate::B_REG_GET_USER_GROUPS);
	if (message.AddString("name", user) != B_OK
		|| message.AddInt32("max count", maxGroupCount) != B_OK) {
		return -1;
	}

	// send request
	KMessage reply;
	error = BPrivate::send_authentication_request_to_registrar(message, reply);
	if (error != B_OK)
		return -1;

	// unpack reply
	int32 count;
	const int32* groups;
	int32 groupsSize;
	if (reply.FindInt32("count", &count) != B_OK
		|| reply.FindData("groups", B_INT32_TYPE, (const void**)&groups,
				&groupsSize) != B_OK) {
		return -1;
	}

	memcpy(groupList, groups, groupsSize);
	*groupCount = count;

	// add the base group
	if (*groupCount < maxGroupCount)
		groupList[*groupCount] = baseGroup;
	++*groupCount;

	return *groupCount <= maxGroupCount ? *groupCount : -1;
}


int
initgroups(const char* user, gid_t baseGroup)
{
	gid_t groups[NGROUPS_MAX + 1];
	int groupCount = NGROUPS_MAX + 1;
	if (getgrouplist(user, baseGroup, groups, &groupCount) < 0)
		return -1;

	return setgroups(groupCount, groups);
}
