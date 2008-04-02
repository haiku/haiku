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

#include <libroot_private.h>

#include "user_group_common.h"


using BPrivate::GroupDB;
using BPrivate::GroupDBReader;
using BPrivate::GroupEntryHandler;
using BPrivate::UserGroupLocker;

static GroupDB* sGroupDB = NULL;

static struct group sGroupBuffer;
static char sGroupStringBuffer[MAX_GROUP_BUFFER_SIZE];


namespace {

class GroupEntryFindHandler : public GroupEntryHandler {
public:
	GroupEntryFindHandler(const char* name, uid_t gid,
			group* entry, char* buffer, size_t bufferSize)
		:
		fName(name),
		fGID(gid),
		fEntry(entry),
		fBuffer(buffer),
		fBufferSize(bufferSize)
	{
	}

	virtual status_t HandleEntry(const char* name, const char* password,
		gid_t gid, const char* const* members, int memberCount)
	{
		if (fName != NULL ? strcmp(fName, name) != 0 : fGID != gid)
			return 0;

		// found
		status_t error = BPrivate::copy_group_to_buffer(name, password, gid,
			members, memberCount, fEntry, fBuffer, fBufferSize);

		return error == B_OK ? 1 : error;
	}

private:
	const char*	fName;
	gid_t		fGID;
	group*		fEntry;
	char*		fBuffer;
	size_t		fBufferSize;
};


class UserGroupEntryHandler : public BPrivate::GroupEntryHandler {
public:
	UserGroupEntryHandler(const char* user, gid_t* groupList, int maxGroupCount,
			int* groupCount)
		:
		fUser(user),
		fGroupList(groupList),
		fMaxGroupCount(maxGroupCount),
		fGroupCount(groupCount)
	{
	}

	virtual status_t HandleEntry(const char* name, const char* password,
		gid_t gid, const char* const* members, int memberCount)
	{
		for (int i = 0; i < memberCount; i++) {
			const char* member = members[i];
			if (*member != '\0' && strcmp(member, fUser) == 0) {
				if (*fGroupCount < fMaxGroupCount)
					fGroupList[*fGroupCount] = gid;
				++*fGroupCount;
			}
		}

		return 0;
	}

private:
	const char*	fUser;
	gid_t*		fGroupList;
	int			fMaxGroupCount;
	int*		fGroupCount;
};

}	// empty namespace


static GroupDB*
init_group_db()
{
	if (sGroupDB != NULL)
		return sGroupDB;

	sGroupDB = new(std::nothrow) GroupDB;
	if (sGroupDB == NULL)
		return NULL;

	if (sGroupDB->Init() != B_OK) {
		delete sGroupDB;
		sGroupDB = NULL;
	}

	return sGroupDB;
}


// #pragma mark -


struct group*
getgrent(void)
{
	struct group* result = NULL;
	int status = getgrent_r(&sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getgrent_r(struct group* group, char* buffer, size_t bufferSize,
	struct group** _result)
{
	UserGroupLocker _;

	int status = B_NO_MEMORY;

	*_result = NULL;

	if (GroupDB* db = init_group_db()) {
		status = db->GetNextEntry(group, buffer, bufferSize);
		if (status == 0)
			*_result = group;

	}

	return status;
}


void
setgrent(void)
{
	UserGroupLocker _;

	if (GroupDB* db = init_group_db())
		db->RewindEntries();
}


void
endgrent(void)
{
	UserGroupLocker locker;

	GroupDB* db = sGroupDB;
	sGroupDB = NULL;

	locker.Unlock();

	delete db;
}


struct group *
getgrnam(const char *name)
{
	struct group* result = NULL;
	int status = getgrnam_r(name, &sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getgrnam_r(const char *name, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	GroupEntryFindHandler handler(name, 0, group, buffer, bufferSize);
	status_t status = GroupDBReader(&handler).Read(BPrivate::kGroupFile);

	*_result = (status == 1 ? group : NULL);
	return (status == 1 ? 0 : (status == 0 ? ENOENT : status));
}


struct group *
getgrgid(gid_t gid)
{
	struct group* result = NULL;
	int status = getgrgid_r(gid, &sGroupBuffer, sGroupStringBuffer,
		sizeof(sGroupStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getgrgid_r(gid_t gid, struct group *group, char *buffer,
	size_t bufferSize, struct group **_result)
{
	GroupEntryFindHandler handler(NULL, gid, group, buffer, bufferSize);
	status_t status = GroupDBReader(&handler).Read(BPrivate::kGroupFile);

	*_result = (status == 1 ? group : NULL);
	return (status == 1 ? 0 : (status == 0 ? ENOENT : status));
}


int
getgrouplist(const char* user, gid_t baseGroup, gid_t* groupList,
	int* groupCount)
{
	int maxGroupCount = *groupCount;
	*groupCount = 0;

	UserGroupEntryHandler handler(user, groupList, maxGroupCount, groupCount);
	BPrivate::GroupDBReader(&handler).Read(BPrivate::kGroupFile);

	// put in the base group
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
