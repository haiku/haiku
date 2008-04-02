/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "user_group_common.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <libroot_lock.h>
#include <libroot_private.h>


using BPrivate::FileLineReader;
using BPrivate::Tokenizer;
using BPrivate::FileDBEntry;
using BPrivate::FileDBReader;
using BPrivate::FileDB;
using BPrivate::PasswdDBEntry;
using BPrivate::PasswdEntryHandler;
using BPrivate::PasswdDBReader;
using BPrivate::PasswdDB;
using BPrivate::GroupDBEntry;
using BPrivate::GroupEntryHandler;
using BPrivate::GroupDBReader;
using BPrivate::GroupDB;


const char* BPrivate::kPasswdFile = "/etc/passwd";
const char* BPrivate::kGroupFile = "/etc/group";

static benaphore sUserGroupLock;


status_t
BPrivate::user_group_lock()
{
	return benaphore_lock(&sUserGroupLock);
}


status_t
BPrivate::user_group_unlock()
{
	return benaphore_unlock(&sUserGroupLock);
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


static char*
buffer_dup_string(const char* string, char*& buffer, size_t& bufferLen)
{
	if (string == NULL)
		return NULL;

	size_t size = strlen(string) + 1;
	if (size > bufferLen)
		return NULL;

	strcpy(buffer, string);
	char* result = buffer;
	buffer += size;
	bufferLen -= size;

	return result;
}


static void*
buffer_allocate(size_t size, size_t align, char*& buffer, size_t& bufferSize)
{
	// align padding
	addr_t pad = align - (((addr_t)buffer - 1)  & (align - 1)) - 1;
	if (pad + size > bufferSize)
		return NULL;

	char* result = buffer + pad;
	buffer = result + size;
	bufferSize -= pad + size;

	return result;
}


// #pragma mark - FileDBEntry


FileDBEntry::FileDBEntry()
	:
	fName(NULL),
	fID(-1),
	fNext(NULL)
{
}


FileDBEntry::~FileDBEntry()
{
}


// #pragma mark - FileDBReader


FileDBReader::FileDBReader()
{
}


FileDBReader::~FileDBReader()
{
}


status_t
FileDBReader::Read(const char* path)
{
	// read file
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return errno;

	FileLineReader reader(fd);	

	status_t error = B_OK;

	while (char* line = reader.NextNonEmptyLine()) {
		Tokenizer tokenizer(line);
		error = ParseEntryLine(tokenizer);
		if (error != B_OK)
			break;
	}

	close(fd);

	return error;
}


// #pragma mark - FileDB


FileDB::FileDB()
	:
	fEntries(NULL),
	fLastEntry(NULL)
{
}


FileDB::~FileDB()
{
	while (FileDBEntry* entry = fEntries) {
		fEntries = entry->Next();
		delete entry;
	}
	fLastEntry = NULL;
}


int
FileDB::GetNextEntry(void* entryBuffer, char* buffer, size_t bufferSize)
{
	FileDBEntry* entry = NULL;

	if (fLastEntry == NULL) {
		// rewound
		entry = fEntries;
	} else if (fLastEntry->Next() != NULL) {
		// get next entry
		entry = fLastEntry->Next();
	}

	// copy the entry, if we found one
	if (entry != NULL) {
		int result = entry->CopyToBuffer(entryBuffer, buffer, bufferSize);
		if (result == 0)
			fLastEntry = entry;
		return result;
	}

	return ENOENT;
}


void
FileDB::RewindEntries()
{
	fLastEntry = NULL;
}


FileDBEntry*
FileDB::FindEntry(const char* name) const
{
	// find the entry
	FileDBEntry* entry = fEntries;
	while (entry != NULL && strcmp(entry->Name(), name) != 0)
		entry = entry->Next();

	return entry;
}


FileDBEntry*
FileDB::FindEntry(int32 id) const
{
	// find the entry
	FileDBEntry* entry = fEntries;
	while (entry != NULL && entry->ID() != id)
		entry = entry->Next();

	return entry;
}


int
FileDB::GetEntry(const char* name, void* entryBuffer, char* buffer,
	size_t bufferSize) const
{
	FileDBEntry* entry = FindEntry(name);
	if (entry == NULL)
		return ENOENT;

	return entry->CopyToBuffer(entryBuffer, buffer, bufferSize);
}


int
FileDB::GetEntry(int32 id, void* entryBuffer, char* buffer,
	size_t bufferSize) const
{
	FileDBEntry* entry = FindEntry(id);
	if (entry == NULL)
		return ENOENT;

	return entry->CopyToBuffer(entryBuffer, buffer, bufferSize);
}


void
FileDB::AddEntry(FileDBEntry* entry)
{
	entry->SetNext(fEntries);
	fEntries = entry;
}


// #pragma mark - passwd support


status_t
BPrivate::copy_passwd_to_buffer(const char* name, const char* password,
	uid_t uid, gid_t gid, const char* home, const char* shell,
	const char* realName, passwd* entry, char* buffer, size_t bufferSize)
{
	entry->pw_uid = uid;
	entry->pw_gid = gid;

	entry->pw_name = buffer_dup_string(name, buffer, bufferSize);
	entry->pw_passwd = buffer_dup_string(password, buffer, bufferSize);
	entry->pw_dir = buffer_dup_string(home, buffer, bufferSize);
	entry->pw_shell = buffer_dup_string(shell, buffer, bufferSize);
	entry->pw_gecos = buffer_dup_string(realName, buffer, bufferSize);

	if (entry->pw_name && entry->pw_passwd && entry->pw_dir
			&& entry->pw_shell && entry->pw_gecos) {
		return 0;
	}

	return ERANGE;
}


// #pragma mark - PasswdDBEntry


PasswdDBEntry::PasswdDBEntry()
	: FileDBEntry(),
	fPassword(NULL),
	fHome(NULL),
	fShell(NULL),
	fRealName(NULL)
{
}


PasswdDBEntry::~PasswdDBEntry()
{
	free(fName);
}


bool
PasswdDBEntry::Init(const char* name, const char* password, uid_t uid,
	gid_t gid, const char* home, const char* shell, const char* realName)
{
	size_t bufferSize = strlen(name) + 1
		+ strlen(password) + 1
		+ strlen(home) + 1
		+ strlen(shell) + 1
		+ strlen(realName) + 1;

	char* buffer = (char*)malloc(bufferSize);
	if (buffer == NULL)
		return false;

	fID = uid;
	fGID = gid;
	fName = buffer_dup_string(name, buffer, bufferSize);
	fPassword = buffer_dup_string(password, buffer, bufferSize);
	fHome = buffer_dup_string(home, buffer, bufferSize);
	fShell = buffer_dup_string(shell, buffer, bufferSize);
	fRealName = buffer_dup_string(realName, buffer, bufferSize);

	return true;
}


int
PasswdDBEntry::CopyToBuffer(void* entryBuffer, char* buffer, size_t bufferSize)
{
	return copy_passwd_to_buffer(fName, fPassword, fID, fGID, fHome, fShell,
		fRealName, (passwd*)entryBuffer, buffer, bufferSize);
}


// #pragma mark - PasswdEntryHandler


PasswdEntryHandler::~PasswdEntryHandler()
{
}


// #pragma mark - PasswdDBReader


PasswdDBReader::PasswdDBReader(PasswdEntryHandler* handler)
	: fHandler(handler)
{
}


status_t
PasswdDBReader::ParseEntryLine(Tokenizer& tokenizer)
{
	char* name = tokenizer.NextTrimmedToken(':');
	char* password = tokenizer.NextTrimmedToken(':');
	char* userID = tokenizer.NextTrimmedToken(':');
	char* groupID = tokenizer.NextTrimmedToken(':');
	char* realName = tokenizer.NextTrimmedToken(':');
	char* home = tokenizer.NextTrimmedToken(':');
	char* shell = tokenizer.NextTrimmedToken(':');

	// skip if invalid
	size_t nameLen;
	if (shell == NULL || (nameLen = strlen(name)) == 0
			|| !isdigit(*userID) || !isdigit(*groupID)
		|| nameLen >= MAX_PASSWD_NAME_LEN
		|| strlen(password) >= MAX_PASSWD_PASSWORD_LEN
		|| strlen(realName) >= MAX_PASSWD_REAL_NAME_LEN
		|| strlen(home) >= MAX_PASSWD_HOME_DIR_LEN
		|| strlen(shell) >= MAX_PASSWD_SHELL_LEN) {
		return B_OK;
	}

	gid_t uid = atoi(userID);
	gid_t gid = atoi(groupID);

	return fHandler->HandleEntry(name, password, uid, gid, home, shell,
		realName);
}


// #pragma mark - PasswdDB


status_t
PasswdDB::Init()
{
	return PasswdDBReader(this).Read(kPasswdFile);
}


status_t
PasswdDB::HandleEntry(const char* name, const char* password, uid_t uid,
	gid_t gid, const char* home, const char* shell, const char* realName)
{
	PasswdDBEntry* entry = new(nothrow) PasswdDBEntry();
	if (entry == NULL || !entry->Init(name, password, uid, gid, home, shell,
			realName)) {
		delete entry;
		return B_NO_MEMORY;
	}

	AddEntry(entry);
	return B_OK;
}


// #pragma mark - passwd support


status_t
BPrivate::copy_group_to_buffer(const char* name, const char* password,
	gid_t gid, const char* const* members, int memberCount, group* entry,
	char* buffer, size_t bufferSize)
{
	entry->gr_gid = gid;

	// allocate member array (do that first for alignment reasons)
	entry->gr_mem = (char**)buffer_allocate(sizeof(char*) * (memberCount + 1),
		sizeof(char*), buffer, bufferSize);
	if (entry->gr_mem == NULL)
		return ERANGE;

	// copy name and password
	entry->gr_name = buffer_dup_string(name, buffer, bufferSize);
	entry->gr_passwd = buffer_dup_string(password, buffer, bufferSize);
	if (entry->gr_name == NULL || entry->gr_passwd == NULL)
		return ERANGE;

	// copy member array
	for (int i = 0; i < memberCount; i++) {
		entry->gr_mem[i] = buffer_dup_string(members[i], buffer, bufferSize);
		if (entry->gr_mem[i] == NULL)
			return ERANGE;
	}
	entry->gr_mem[memberCount] = NULL;

	return 0;
}


// #pragma mark - GroupDBEntry


GroupDBEntry::GroupDBEntry()
	: FileDBEntry(),
	fPassword(NULL),
	fMembers(NULL),
	fMemberCount(0)
{
}


GroupDBEntry::~GroupDBEntry()
{
	free(fMembers);
}


bool
GroupDBEntry::Init(const char* name, const char* password, gid_t gid,
	const char* const* members, int memberCount)
{
	size_t bufferSize = sizeof(char*) * (memberCount + 1)
		+ strlen(name) + 1
		+ strlen(password) + 1;

	for (int i = 0; i < memberCount; i++)
		bufferSize += strlen(members[i]) + 1;

	char* buffer = (char*)malloc(bufferSize);
	if (buffer == NULL)
		return false;

	// allocate member array first (for alignment reasons)
	fMembers = (char**)buffer_allocate(sizeof(char*) * (memberCount + 1),
		sizeof(char*), buffer, bufferSize);

	fID = gid;
	fName = buffer_dup_string(name, buffer, bufferSize);
	fPassword = buffer_dup_string(password, buffer, bufferSize);

	// copy members
	for (int i = 0; i < memberCount; i++)
		fMembers[i] = buffer_dup_string(members[i], buffer, bufferSize);
	fMembers[memberCount] = NULL;
	fMemberCount = memberCount;

	return true;
}


int
GroupDBEntry::CopyToBuffer(void* entryBuffer, char* buffer, size_t bufferSize)
{
	return copy_group_to_buffer(fName, fPassword, fID, fMembers, fMemberCount,
		(group*)entryBuffer, buffer, bufferSize);
}


// #pragma mark - GroupEntryHandler


GroupEntryHandler::~GroupEntryHandler()
{
}


// #pragma mark - GroupDBReader


GroupDBReader::GroupDBReader(GroupEntryHandler* handler)
	: fHandler(handler)
{
}


status_t
GroupDBReader::ParseEntryLine(Tokenizer& tokenizer)
{
	char* name = tokenizer.NextTrimmedToken(':');
	char* password = tokenizer.NextTrimmedToken(':');
	char* groupID = tokenizer.NextTrimmedToken(':');

	// skip if invalid
	size_t nameLen;
	if (groupID == NULL || (nameLen = strlen(name)) == 0 || !isdigit(*groupID)
		|| nameLen >= MAX_GROUP_NAME_LEN
		|| strlen(password) >= MAX_GROUP_PASSWORD_LEN) {
		return B_OK;
	}

	gid_t gid = atol(groupID);

	const char* members[MAX_GROUP_MEMBER_COUNT];
	int memberCount = 0;

	while (char* groupUser = tokenizer.NextTrimmedToken(',')) {
		// ignore invalid members
		if (*groupUser == '\0' || strlen(groupUser) >= MAX_PASSWD_NAME_LEN)
			continue;

		members[memberCount++] = groupUser;

		// ignore excess members
		if (memberCount == MAX_GROUP_MEMBER_COUNT)
			break;
	}

	return fHandler->HandleEntry(name, password, gid, members, memberCount);
}


// #pragma mark - GroupDB


status_t
GroupDB::Init()
{
	return GroupDBReader(this).Read(kGroupFile);
}


status_t
GroupDB::HandleEntry(const char* name, const char* password, gid_t gid,
	const char* const* members, int memberCount)
{
	GroupDBEntry* entry = new(nothrow) GroupDBEntry();
	if (entry == NULL || !entry->Init(name, password, gid, members,
			memberCount)) {
		delete entry;
		return B_NO_MEMORY;
	}

	AddEntry(entry);
	return B_OK;
}


// #pragma mark -


void
__init_pwd_backend(void)
{
	benaphore_init(&sUserGroupLock, "user group");
}


void
__reinit_pwd_backend_after_fork(void)
{
	benaphore_init(&sUserGroupLock, "user group");
}
