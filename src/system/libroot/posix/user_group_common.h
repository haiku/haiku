
/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_USER_GROUP_COMMON_H
#define _LIBROOT_USER_GROUP_COMMON_H

#include <grp.h>
#include <pwd.h>

#include <SupportDefs.h>

#include <AutoLocker.h>


namespace BPrivate {

class FileLineReader;
class Tokenizer;

extern const char* kPasswdFile;
extern const char* kGroupFile;


// locking

status_t user_group_lock();
status_t user_group_unlock();


class UserGroupLocking {
public:
    inline bool Lock(int*)
    {
        return user_group_lock() == B_OK;
    }

    inline void Unlock(int*)
    {
        user_group_unlock();
    }
};


class UserGroupLocker : public AutoLocker<int, UserGroupLocking> {
public:
	UserGroupLocker()
		: AutoLocker<int, UserGroupLocking>((int*)1)
	{
	}
};


// #pragma mark - File DB Base Classes


class FileDBEntry {
public:
	FileDBEntry();
	virtual ~FileDBEntry();

	const char*	Name() const		{ return fName; }
	int32 ID() const				{ return fID; }

	FileDBEntry* Next() const		{ return fNext; }
	void SetNext(FileDBEntry* next)	{ fNext = next; }

	virtual int CopyToBuffer(void* entryBuffer, char* buffer,
		size_t bufferSize) = 0;

protected:
	char*			fName;
	int32			fID;
	FileDBEntry*	fNext;
};


class FileDBReader {
public:
	FileDBReader();
	virtual ~FileDBReader();

	status_t Read(const char* path);

protected:
	virtual status_t ParseEntryLine(Tokenizer& tokenizer) = 0;
};


class FileDB {
public:
	FileDB();
	virtual ~FileDB();

	int GetNextEntry(void* entryBuffer, char* buffer, size_t bufferSize);
	void RewindEntries();

	FileDBEntry* FindEntry(const char* name) const;
	FileDBEntry* FindEntry(int32 id) const;

	int GetEntry(const char* name, void* entryBuffer, char* buffer,
		size_t bufferSize) const;
	int GetEntry(int32 id, void* entryBuffer, char* buffer,
		size_t bufferSize) const;

protected:
	void AddEntry(FileDBEntry* entry);

protected:
	FileDBEntry*	fEntries;
	FileDBEntry*	fLastEntry;
};


// #pragma mark - Passwd DB


status_t
copy_passwd_to_buffer(const char* name, const char* password, uid_t uid,
	gid_t gid, const char* home, const char* shell, const char* realName,
	passwd* entry, char* buffer, size_t bufferSize);


class PasswdDBEntry : public FileDBEntry {
public:
	PasswdDBEntry();
	virtual ~PasswdDBEntry();

	bool Init(const char* name, const char* password, uid_t uid, gid_t gid,
		const char* home, const char* shell, const char* realName);

	virtual int CopyToBuffer(void* entryBuffer, char* buffer,
		size_t bufferSize);

protected:
	gid_t			fGID;
	char*			fPassword;
	char*			fHome;
	char*			fShell;
	char*			fRealName;
};


class PasswdEntryHandler {
public:
	virtual ~PasswdEntryHandler();

	virtual status_t HandleEntry(const char* name, const char* password,
		uid_t uid, gid_t gid, const char* home, const char* shell,
		const char* realName) = 0;
};


class PasswdDBReader : public FileDBReader {
public:
	PasswdDBReader(PasswdEntryHandler* handler);

protected:
	virtual status_t ParseEntryLine(Tokenizer& tokenizer);

private:
	PasswdEntryHandler*	fHandler;
};


class PasswdDB : public FileDB, private PasswdEntryHandler {
public:
	status_t Init();

private:
	virtual status_t HandleEntry(const char* name, const char* password,
		uid_t uid, gid_t gid, const char* home, const char* shell,
		const char* realName);
};


// #pragma mark - Group DB


status_t
copy_group_to_buffer(const char* name, const char* password, gid_t gid,
	const char* const* members, int memberCount, group* entry, char* buffer,
	size_t bufferSize);


class GroupDBEntry : public FileDBEntry {
public:
	GroupDBEntry();
	virtual ~GroupDBEntry();

	bool Init(const char* name, const char* password, gid_t gid,
		const char* const* members, int memberCount);

	virtual int CopyToBuffer(void* entryBuffer, char* buffer,
		size_t bufferSize);

protected:
	char*			fPassword;
	char**			fMembers;
	int				fMemberCount;
};


class GroupEntryHandler {
public:
	virtual ~GroupEntryHandler();

	virtual status_t HandleEntry(const char* name, const char* password,
		gid_t gid, const char* const* members, int memberCount) = 0;
};


class GroupDBReader : public FileDBReader {
public:
	GroupDBReader(GroupEntryHandler* handler);

protected:
	virtual status_t ParseEntryLine(Tokenizer& tokenizer);

private:
	GroupEntryHandler*	fHandler;
};


class GroupDB : public FileDB, private GroupEntryHandler {
public:
	status_t Init();

private:
	virtual status_t HandleEntry(const char* name, const char* password,
		gid_t gid, const char* const* members, int memberCount);
};


}	// namespace BPrivate


#endif	// _LIBROOT_USER_GROUP_COMMON_H
