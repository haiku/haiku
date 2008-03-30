/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>


template<typename T>
static inline T
set_errno_if_necessary(const T& result)
{
	if (result < 0) {
		errno = result;
		return -1;
	}

	return result;
}


class FileLineReader {
public:
	FileLineReader(int fd)
		: fFD(fd),
		  fSize(0),
		  fOffset(0)
	{
	}

	char* NextLine()
	{
		char* eol;
		if (fOffset >= fSize
			|| (eol = strchr(fBuffer + fOffset, '\n')) == NULL) {
			_ReadBuffer();
			if (fOffset >= fSize)
				return NULL;

			eol = strchr(fBuffer + fOffset, '\n');
			if (eol == NULL)
				eol = fBuffer + fSize;
		}

		char* result = fBuffer + fOffset;
		*eol = '\0';
		fOffset = eol + 1 - fBuffer;
		return result;
	}

	char* NextNonEmptyLine()
	{
		while (char* line = NextLine()) {
			while (*line != '\0' && isspace(*line))
				line++;

			if (*line != '\0' && *line != '#')
				return line;
		}

		return NULL;
	}

private:
	void _ReadBuffer()
	{
		// catch special cases: full buffer or already done with the file
		if (fSize == LINE_MAX || fFD < 0)
			return;

		// move buffered bytes to the beginning of the buffer
		int leftBytes = 0;
		if (fOffset < fSize) {
			leftBytes = fSize - fOffset;
			memmove(fBuffer, fBuffer + fOffset, leftBytes);
		}

		fOffset = 0;
		fSize = leftBytes;

		// read
		ssize_t bytesRead = read(fFD, fBuffer + leftBytes,
			LINE_MAX - leftBytes);
		if (bytesRead > 0)
			fSize += bytesRead;
		else
			fFD = -1;

		// null-terminate
		fBuffer[fSize] = '\0';
	}

private:
	int		fFD;
	char	fBuffer[LINE_MAX + 1];
	int		fSize;
	int		fOffset;
};


class Tokenizer {
public:
	Tokenizer(char* string)
		: fString(string)
	{
	}

	char* NextToken(char separator)
	{
		if (fString == NULL)
			return NULL;

		char* token = fString;
		fString = strchr(fString, separator);
		if (fString != NULL) {
			*fString = '\0';
			fString++;
		} 

		return token;
	}

	char* NextTrimmedToken(char separator)
	{
		char* token = NextToken(separator);
		if (token == NULL)
			return NULL;

		// skip spaces at the beginning
		while (*token != '\0' && isspace(*token))
			token++;

		// cut off spaces at the end
		char* end = token + strlen(token);
		while (end != token && isspace(end[-1]))
			end--;
		*end = '\0';

		return token;
	}

private:
	char*		fString;
};


//	#pragma mark -


gid_t
getegid(void)
{
	return _kern_getgid(true);
}


uid_t
geteuid(void)
{
	return _kern_getuid(true);
}


gid_t
getgid(void)
{
	return _kern_getgid(false);
}


uid_t 
getuid(void)
{
	return _kern_getuid(false);
}


int
setgid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setregid(gid, (gid_t)-1, true));
}


int
setuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setreuid(uid, (uid_t)-1, true));
}


int
setegid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setregid((gid_t)-1, gid, false));
}


int
seteuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setreuid((uid_t)-1, uid, false));
}


int
setregid(gid_t rgid, gid_t egid)
{
	return set_errno_if_necessary(_kern_setregid(rgid, egid, false));
}


int
setreuid(uid_t ruid, uid_t euid)
{
	return set_errno_if_necessary(_kern_setreuid(ruid, euid, false));
}


int
getgrouplist(const char* user, gid_t baseGroup, gid_t* groupList,
	int* groupCount)
{
	int maxGroupCount = *groupCount;
	*groupCount = 0;

	// read group file
	int fd = open("/etc/group", O_RDONLY);
	FileLineReader reader(fd);	

	while (char* line = reader.NextNonEmptyLine()) {
		Tokenizer lineTokenizer(line);
		lineTokenizer.NextTrimmedToken(':');	// group name
		lineTokenizer.NextTrimmedToken(':');	// group password
		char* groupID = lineTokenizer.NextTrimmedToken(':');

		if (groupID == NULL || !isdigit(*groupID))
			continue;

		gid_t gid = atol(groupID);
		if (gid == baseGroup)
			continue;

		while (char* groupUser = lineTokenizer.NextTrimmedToken(',')) {
			if (*groupUser != '\0' && strcmp(groupUser, user) == 0) {
				if (*groupCount < maxGroupCount)
					groupList[*groupCount] = gid;
				++*groupCount;
			}
		}
	}

	if (fd >= 0)
		close(fd);

	// put in the base group
	if (*groupCount < maxGroupCount)
		groupList[*groupCount] = baseGroup;
	++*groupCount;

	return *groupCount <= maxGroupCount ? *groupCount : -1;
}


int
getgroups(int groupCount, gid_t groupList[])
{
	return set_errno_if_necessary(_kern_getgroups(groupCount, groupList));
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


int
setgroups(int groupCount, const gid_t* groupList)
{
	return set_errno_if_necessary(_kern_setgroups(groupCount, groupList));
}
