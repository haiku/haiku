/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <pwd.h>

#include <errno.h>
#include <unistd.h>

#include <new>

#include <OS.h>

#include <libroot_private.h>

#include "user_group_common.h"


using BPrivate::PasswdDB;
using BPrivate::PasswdDBReader;
using BPrivate::PasswdEntryHandler;
using BPrivate::UserGroupLocker;

static PasswdDB* sPasswdDB = NULL;

static struct passwd sPasswdBuffer;
static char sPasswdStringBuffer[MAX_PASSWD_BUFFER_SIZE];


namespace {

class PasswdEntryFindHandler : public PasswdEntryHandler {
public:
	PasswdEntryFindHandler(const char* name, uid_t uid,
			passwd* entry, char* buffer, size_t bufferSize)
		:
		fName(name),
		fUID(uid),
		fEntry(entry),
		fBuffer(buffer),
		fBufferSize(bufferSize)
	{
	}

	virtual status_t HandleEntry(const char* name, const char* password,
		uid_t uid, gid_t gid, const char* home, const char* shell,
		const char* realName)
	{
		if (fName != NULL ? strcmp(fName, name) != 0 : fUID != uid)
			return 0;

		// found
		status_t error = BPrivate::copy_passwd_to_buffer(name, password, uid,
			gid, home, shell, realName, fEntry, fBuffer, fBufferSize);
		return error == B_OK ? 1 : error;
	}

private:
	const char*	fName;
	uid_t		fUID;
	passwd*		fEntry;
	char*		fBuffer;
	size_t		fBufferSize;
};

}	// empty namespace


static PasswdDB*
init_passwd_db()
{
	if (sPasswdDB != NULL)
		return sPasswdDB;

	sPasswdDB = new(nothrow) PasswdDB;
	if (sPasswdDB == NULL)
		return NULL;

	if (sPasswdDB->Init() != B_OK) {
		delete sPasswdDB;
		sPasswdDB = NULL;
	}

	return sPasswdDB;
}


// #pragma mark -


struct passwd*
getpwent(void)
{
	struct passwd* result = NULL;
	int status = getpwent_r(&sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getpwent_r(struct passwd* passwd, char* buffer, size_t bufferSize,
	struct passwd** _result)
{
	UserGroupLocker _;

	int status = B_NO_MEMORY;

	*_result = NULL;

	if (PasswdDB* db = init_passwd_db()) {
		status = db->GetNextEntry(passwd, buffer, bufferSize);
		if (status == 0)
			*_result = passwd;

	}

	return status;
}


void
setpwent(void)
{
	UserGroupLocker _;

	if (PasswdDB* db = init_passwd_db())
		db->RewindEntries();
}


void
endpwent(void)
{
	UserGroupLocker locker;

	PasswdDB* db = sPasswdDB;
	sPasswdDB = NULL;

	locker.Unlock();

	delete db;
}


struct passwd *
getpwnam(const char *name)
{
	struct passwd* result = NULL;
	int status = getpwnam_r(name, &sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getpwnam_r(const char *name, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	PasswdEntryFindHandler handler(name, 0, passwd, buffer, bufferSize);
	status_t status = PasswdDBReader(&handler).Read(BPrivate::kPasswdFile);

	*_result = (status == 1 ? passwd : NULL);
	return (status == 1 ? 0 : (status == 0 ? ENOENT : status));
}


struct passwd *
getpwuid(uid_t uid)
{
	struct passwd* result = NULL;
	int status = getpwuid_r(uid, &sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		errno = status;
	return result;
}


int
getpwuid_r(uid_t uid, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	PasswdEntryFindHandler handler(NULL, uid, passwd, buffer, bufferSize);
	status_t status = PasswdDBReader(&handler).Read(BPrivate::kPasswdFile);

	*_result = (status == 1 ? passwd : NULL);
	return (status == 1 ? 0 : (status == 0 ? ENOENT : status));
}
