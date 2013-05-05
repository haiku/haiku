/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <shadow.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <OS.h>

#include <AutoDeleter.h>
#include <errno_private.h>
#include <libroot_private.h>
#include <RegistrarDefs.h>
#include <user_group.h>

#include <util/KMessage.h>


using BPrivate::UserGroupLocker;
using BPrivate::relocate_pointer;


static KMessage sShadowPwdDBReply;
static spwd** sShadowPwdEntries = NULL;
static size_t sShadowPwdEntryCount = 0;
static size_t sIterationIndex = 0;

static struct spwd sShadowPwdBuffer;
static char sShadowPwdStringBuffer[MAX_SHADOW_PWD_BUFFER_SIZE];


static status_t
init_shadow_pwd_db()
{
	if (sShadowPwdEntries != NULL)
		return B_OK;

	// ask the registrar
	KMessage message(BPrivate::B_REG_GET_SHADOW_PASSWD_DB);
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		sShadowPwdDBReply);
	if (error != B_OK)
		return error;

	// unpack the reply
	int32 count;
	spwd** entries;
	int32 numBytes;
	if ((error = sShadowPwdDBReply.FindInt32("count", &count)) != B_OK
		|| (error = sShadowPwdDBReply.FindData("entries", B_RAW_TYPE,
				(const void**)&entries, &numBytes)) != B_OK) {
		return error;
	}

	// relocate the entries
	addr_t baseAddress = (addr_t)entries;
	for (int32 i = 0; i < count; i++) {
		spwd* entry = relocate_pointer(baseAddress, entries[i]);
		relocate_pointer(baseAddress, entry->sp_namp);
		relocate_pointer(baseAddress, entry->sp_pwdp);
	}

	sShadowPwdEntries = entries;
	sShadowPwdEntryCount = count;

	return B_OK;
}


// #pragma mark -


struct spwd*
getspent(void)
{
	struct spwd* result = NULL;
	int status = getspent_r(&sShadowPwdBuffer, sShadowPwdStringBuffer,
		sizeof(sShadowPwdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getspent_r(struct spwd* spwd, char* buffer, size_t bufferSize,
	struct spwd** _result)
{
	UserGroupLocker _;

	int status = B_NO_MEMORY;

	*_result = NULL;

	if ((status = init_shadow_pwd_db()) == B_OK) {
		if (sIterationIndex >= sShadowPwdEntryCount)
			return ENOENT;

		status = BPrivate::copy_shadow_pwd_to_buffer(
			sShadowPwdEntries[sIterationIndex], spwd, buffer, bufferSize);

		if (status == B_OK) {
			sIterationIndex++;
			*_result = spwd;
		}
	}

	return status;
}


void
setspent(void)
{
	UserGroupLocker _;

	sIterationIndex = 0;
}


void
endspent(void)
{
	UserGroupLocker locker;

	sShadowPwdDBReply.Unset();
	sShadowPwdEntries = NULL;
	sShadowPwdEntryCount = 0;
	sIterationIndex = 0;
}


struct spwd *
getspnam(const char *name)
{
	struct spwd* result = NULL;
	int status = getspnam_r(name, &sShadowPwdBuffer, sShadowPwdStringBuffer,
		sizeof(sShadowPwdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getspnam_r(const char *name, struct spwd *spwd, char *buffer,
	size_t bufferSize, struct spwd **_result)
{
	*_result = NULL;

	KMessage message(BPrivate::B_REG_GET_USER);
	message.AddString("name", name);
	message.AddBool("shadow", true);

	KMessage reply;
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		reply);
	if (error != B_OK)
		return error;

	const char* password;
	int32 lastChanged;
	int32 min;
	int32 max;
	int32 warn;
	int32 inactive;
	int32 expiration;
	int32 flags;

	if ((error = reply.FindString("name", &name)) != B_OK
		|| (error = reply.FindString("shadow password", &password)) != B_OK
		|| (error = reply.FindInt32("last changed", &lastChanged)) != B_OK
		|| (error = reply.FindInt32("min", &min)) != B_OK
		|| (error = reply.FindInt32("max", &max)) != B_OK
		|| (error = reply.FindInt32("warn", &warn)) != B_OK
		|| (error = reply.FindInt32("inactive", &inactive)) != B_OK
		|| (error = reply.FindInt32("expiration", &expiration)) != B_OK
		|| (error = reply.FindInt32("flags", &flags)) != B_OK) {
		return error;
	}

	error = BPrivate::copy_shadow_pwd_to_buffer(name, password, lastChanged,
		min, max, warn, inactive, expiration, flags, spwd, buffer, bufferSize);
	if (error == B_OK)
		*_result = spwd;

	return error;
}


struct spwd*
sgetspent(const char* line)
{
	struct spwd* result = NULL;
	int status = sgetspent_r(line, &sShadowPwdBuffer, sShadowPwdStringBuffer,
		sizeof(sShadowPwdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
sgetspent_r(const char* _line, struct spwd *spwd, char *buffer,
	size_t bufferSize, struct spwd** _result)
{
	*_result = NULL;

	if (_line == NULL)
		return B_BAD_VALUE;

	// we need a mutable copy of the line
	char* line = strdup(_line);
	if (line == NULL)
		return B_NO_MEMORY;
	MemoryDeleter _(line);

	char* name;
	char* password;
	int lastChanged;
	int min;
	int max;
	int warn;
	int inactive;
	int expiration;
	int flags;

	status_t status = BPrivate::parse_shadow_pwd_line(line, name, password,
		lastChanged, min, max, warn, inactive, expiration, flags);
	if (status != B_OK)
		return status;

	status = BPrivate::copy_shadow_pwd_to_buffer(name, password, lastChanged,
		min, max, warn, inactive, expiration, flags, spwd, buffer, bufferSize);
	if (status != B_OK)
		return status;

	*_result = spwd;
	return 0;
}


struct spwd*
fgetspent(FILE* file)
{
	struct spwd* result = NULL;
	int status = fgetspent_r(file, &sShadowPwdBuffer, sShadowPwdStringBuffer,
		sizeof(sShadowPwdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
fgetspent_r(FILE* file, struct spwd* spwd, char* buffer, size_t bufferSize,
	struct spwd** _result)
{
	*_result = NULL;

	// read a line
	char lineBuffer[LINE_MAX + 1];
	__set_errno(0);
	char* line = fgets(lineBuffer, sizeof(lineBuffer), file);
	if (line == NULL) {
		if (errno != 0)
			return errno;
		return ENOENT;
	}

	return sgetspent_r(line, spwd, buffer, bufferSize, _result);
}
