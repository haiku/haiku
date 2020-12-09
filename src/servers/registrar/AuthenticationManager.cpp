/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AuthenticationManager.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>

#include <map>
#include <new>
#include <set>
#include <string>

#include <DataIO.h>
#include <StringList.h>

#include <AutoDeleter.h>
#include <AutoDeleterPosix.h>
#include <LaunchRoster.h>
#include <RegistrarDefs.h>

#include <libroot_private.h>
#include <user_group.h>
#include <util/KMessage.h>


using std::map;
using std::string;

using namespace BPrivate;


typedef std::set<std::string> StringSet;


class AuthenticationManager::FlatStore {
public:
	FlatStore()
		: fSize(0)
	{
		fBuffer.SetBlockSize(1024);
	}

	void WriteData(size_t offset, const void* data, size_t length)
	{
		ssize_t result = fBuffer.WriteAt(offset, data, length);
		if (result < 0)
			throw status_t(result);
	}

	template<typename Type>
	void WriteData(size_t offset, const Type& data)
	{
		WriteData(&data, sizeof(Type));
	}

	size_t ReserveSpace(size_t length, bool align)
	{
		if (align)
			fSize = _ALIGN(fSize);

		size_t pos = fSize;
		fSize += length;

		return pos;
	}

	void* AppendData(const void* data, size_t length, bool align)
	{
		size_t pos = ReserveSpace(length, align);
		WriteData(pos, data, length);
		return (void*)(addr_t)pos;
	}

	template<typename Type>
	Type* AppendData(const Type& data)
	{
		return (Type*)AppendData(&data, sizeof(Type), true);
	}

	char* AppendString(const char* string)
	{
		return (char*)AppendData(string, strlen(string) + 1, false);
	}

	char* AppendString(const string& str)
	{
		return (char*)AppendData(str.c_str(), str.length() + 1, false);
	}

	const void* Buffer() const
	{
		return fBuffer.Buffer();
	}

	size_t BufferLength() const
	{
		return fSize;
	}

private:
	BMallocIO	fBuffer;
	size_t		fSize;
};


class AuthenticationManager::User {
public:
	User()
		:
		fUID(0),
		fGID(0),
		fLastChanged(0),
		fMin(-1),
		fMax(-1),
		fWarn(-1),
		fInactive(-1),
		fExpiration(-1),
		fFlags(0)
	{
	}

	User(const char* name, const char* password, uid_t uid, gid_t gid,
		const char* home, const char* shell, const char* realName)
		:
		fUID(uid),
		fGID(gid),
		fName(name),
		fPassword(password),
		fHome(home),
		fShell(shell),
		fRealName(realName),
		fLastChanged(0),
		fMin(-1),
		fMax(-1),
		fWarn(-1),
		fInactive(-1),
		fExpiration(-1),
		fFlags(0)
	{
	}

	User(const User& other)
		:
		fUID(other.fUID),
		fGID(other.fGID),
		fName(other.fName),
		fPassword(other.fPassword),
		fHome(other.fHome),
		fShell(other.fShell),
		fRealName(other.fRealName),
		fShadowPassword(other.fShadowPassword),
		fLastChanged(other.fLastChanged),
		fMin(other.fMin),
		fMax(other.fMax),
		fWarn(other.fWarn),
		fInactive(other.fInactive),
		fExpiration(other.fExpiration),
		fFlags(other.fFlags)
	{
	}

	const string& Name() const	{ return fName; }
	const uid_t UID() const		{ return fUID; }

	void SetShadowInfo(const char* password, int lastChanged, int min, int max,
		int warn, int inactive, int expiration, int flags)
	{
		fShadowPassword = password;
		fLastChanged = lastChanged;
		fMin = min;
		fMax = max;
		fWarn = warn;
		fInactive = inactive;
		fExpiration = expiration;
		fFlags = flags;
	}

	void UpdateFromMessage(const KMessage& message)
	{
		int32 intValue;
		const char* stringValue;

		if (message.FindInt32("uid", &intValue) == B_OK)
			fUID = intValue;

		if (message.FindInt32("gid", &intValue) == B_OK)
			fGID = intValue;

		if (message.FindString("name", &stringValue) == B_OK)
			fName = stringValue;

		if (message.FindString("password", &stringValue) == B_OK)
			fPassword = stringValue;

		if (message.FindString("home", &stringValue) == B_OK)
			fHome = stringValue;

		if (message.FindString("shell", &stringValue) == B_OK)
			fShell = stringValue;

		if (message.FindString("real name", &stringValue) == B_OK)
			fRealName = stringValue;

		if (message.FindString("shadow password", &stringValue) == B_OK) {
			fShadowPassword = stringValue;
			// TODO:
			// fLastChanged = now;
		}

		if (message.FindInt32("last changed", &intValue) == B_OK)
			fLastChanged = intValue;

		if (message.FindInt32("min", &intValue) == B_OK)
			fMin = intValue;

		if (message.FindInt32("max", &intValue) == B_OK)
			fMax = intValue;

		if (message.FindInt32("warn", &intValue) == B_OK)
			fWarn = intValue;

		if (message.FindInt32("inactive", &intValue) == B_OK)
			fInactive = intValue;

		if (message.FindInt32("expiration", &intValue) == B_OK)
			fExpiration = intValue;

		if (message.FindInt32("flags", &intValue) == B_OK)
			fFlags = intValue;
	}

	passwd* WriteFlatPasswd(FlatStore& store) const
	{
		struct passwd passwd;

		passwd.pw_uid = fUID;
		passwd.pw_gid = fGID;
		passwd.pw_name = store.AppendString(fName);
		passwd.pw_passwd = store.AppendString(fPassword);
		passwd.pw_dir = store.AppendString(fHome);
		passwd.pw_shell = store.AppendString(fShell);
		passwd.pw_gecos = store.AppendString(fRealName);

		return store.AppendData(passwd);
	}

	spwd* WriteFlatShadowPwd(FlatStore& store) const
	{
		struct spwd spwd;

		spwd.sp_namp = store.AppendString(fName);
		spwd.sp_pwdp = store.AppendString(fShadowPassword);
		spwd.sp_lstchg = fLastChanged;
		spwd.sp_min = fMin;
		spwd.sp_max = fMax;
		spwd.sp_warn = fWarn;
		spwd.sp_inact = fInactive;
		spwd.sp_expire = fExpiration;
		spwd.sp_flag = fFlags;

		return store.AppendData(spwd);
	}

	status_t WriteToMessage(KMessage& message, bool addShadowPwd)
	{
		status_t error;
		if ((error = message.AddInt32("uid", fUID)) != B_OK
			|| (error = message.AddInt32("gid", fGID)) != B_OK
			|| (error = message.AddString("name", fName.c_str())) != B_OK
			|| (error = message.AddString("password", fPassword.c_str()))
					!= B_OK
			|| (error = message.AddString("home", fHome.c_str())) != B_OK
			|| (error = message.AddString("shell", fShell.c_str())) != B_OK
			|| (error = message.AddString("real name", fRealName.c_str()))
					!= B_OK) {
			return error;
		}

		if (!addShadowPwd)
			return B_OK;

		if ((error = message.AddString("shadow password",
					fShadowPassword.c_str())) != B_OK
			|| (error = message.AddInt32("last changed", fLastChanged)) != B_OK
			|| (error = message.AddInt32("min", fMin)) != B_OK
			|| (error = message.AddInt32("max", fMax)) != B_OK
			|| (error = message.AddInt32("warn", fWarn)) != B_OK
			|| (error = message.AddInt32("inactive", fInactive)) != B_OK
			|| (error = message.AddInt32("expiration", fExpiration)) != B_OK
			|| (error = message.AddInt32("flags", fFlags)) != B_OK) {
			return error;
		}

		return B_OK;
	}

	void WritePasswdLine(FILE* file)
	{
		fprintf(file, "%s:%s:%d:%d:%s:%s:%s\n",
			fName.c_str(), fPassword.c_str(), (int)fUID, (int)fGID,
			fRealName.c_str(), fHome.c_str(), fShell.c_str());
	}

	void WriteShadowPwdLine(FILE* file)
	{
		fprintf(file, "%s:%s:%d:", fName.c_str(), fShadowPassword.c_str(),
			fLastChanged);

		// The following values are supposed to be printed as empty strings,
		// if negative.
		int values[5] = { fMin, fMax, fWarn, fInactive, fExpiration };
		for (int i = 0; i < 5; i++) {
			if (values[i] >= 0)
				fprintf(file, "%d", values[i]);
			fprintf(file, ":");
		}

		fprintf(file, "%d\n", fFlags);
	}

private:
	uid_t	fUID;
	gid_t	fGID;
	string	fName;
	string	fPassword;
	string	fHome;
	string	fShell;
	string	fRealName;
	string	fShadowPassword;
	int		fLastChanged;
	int		fMin;
	int		fMax;
	int		fWarn;
	int		fInactive;
	int		fExpiration;
	int		fFlags;
};


class AuthenticationManager::Group {
public:
	Group()
		:
		fGID(0),
		fName(),
		fPassword(),
		fMembers()
	{
	}

	Group(const char* name, const char* password, gid_t gid,
		const char* const* members, int memberCount)
		:
		fGID(gid),
		fName(name),
		fPassword(password),
		fMembers()
	{
		for (int i = 0; i < memberCount; i++)
			fMembers.insert(members[i]);
	}

	~Group()
	{
	}

	const string& Name() const	{ return fName; }
	const gid_t GID() const		{ return fGID; }

	bool HasMember(const char* name)
	{
		try {
			return fMembers.find(name) != fMembers.end();
		} catch (...) {
			return false;
		}
	}

	bool MemberRemoved(const std::string& name)
	{
		return fMembers.erase(name) > 0;
	}

	void UpdateFromMessage(const KMessage& message)
	{
		int32 intValue;
		if (message.FindInt32("gid", &intValue) == B_OK)
			fGID = intValue;

		const char* stringValue;
		if (message.FindString("name", &stringValue) == B_OK)
			fName = stringValue;

		if (message.FindString("password", &stringValue) == B_OK)
			fPassword = stringValue;

		if (message.FindString("members", &stringValue) == B_OK) {
			fMembers.clear();
			for (int32 i = 0;
				(stringValue = message.GetString("members", i, NULL)) != NULL;
				i++) {
				if (stringValue != NULL && *stringValue != '\0')
					fMembers.insert(stringValue);
			}
		}
	}

	group* WriteFlatGroup(FlatStore& store) const
	{
		struct group group;

		char* members[MAX_GROUP_MEMBER_COUNT + 1];
		int32 count = 0;
		for (StringSet::const_iterator it = fMembers.begin();
			it != fMembers.end(); ++it) {
			members[count++] = store.AppendString(it->c_str());
		}
		members[count] = (char*)-1;

		group.gr_gid = fGID;
		group.gr_name = store.AppendString(fName);
		group.gr_passwd = store.AppendString(fPassword);
		group.gr_mem = (char**)store.AppendData(members,
			sizeof(char*) * (count + 1), true);

		return store.AppendData(group);
	}

	status_t WriteToMessage(KMessage& message)
	{
		status_t error;
		if ((error = message.AddInt32("gid", fGID)) != B_OK
			|| (error = message.AddString("name", fName.c_str())) != B_OK
			|| (error = message.AddString("password", fPassword.c_str()))
					!= B_OK) {
			return error;
		}

		for (StringSet::const_iterator it = fMembers.begin();
			it != fMembers.end(); ++it) {
			if ((error = message.AddString("members", it->c_str())) != B_OK)
				return error;
		}

		return B_OK;
	}

	void WriteGroupLine(FILE* file)
	{
		fprintf(file, "%s:%s:%d:",
			fName.c_str(), fPassword.c_str(), (int)fGID);
		for (StringSet::const_iterator it = fMembers.begin();
			it != fMembers.end(); ++it) {
			if (it == fMembers.begin())
				fprintf(file, "%s", it->c_str());
			else
				fprintf(file, ",%s", it->c_str());
		}
		fputs("\n", file);
	}

private:
	gid_t		fGID;
	string		fName;
	string		fPassword;
	StringSet	fMembers;
};


class AuthenticationManager::UserDB {
public:
	status_t AddUser(User* user)
	{
		try {
			fUsersByID[user->UID()] = user;
		} catch (...) {
			return B_NO_MEMORY;
		}

		try {
			fUsersByName[user->Name()] = user;
		} catch (...) {
			fUsersByID.erase(fUsersByID.find(user->UID()));
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void RemoveUser(User* user)
	{
		fUsersByID.erase(fUsersByID.find(user->UID()));
		fUsersByName.erase(fUsersByName.find(user->Name()));
	}

	User* UserByID(uid_t uid) const
	{
		map<uid_t, User*>::const_iterator it = fUsersByID.find(uid);
		return (it == fUsersByID.end() ? NULL : it->second);
	}

	User* UserByName(const char* name) const
	{
		map<string, User*>::const_iterator it = fUsersByName.find(name);
		return (it == fUsersByName.end() ? NULL : it->second);
	}

	int32 WriteFlatPasswdDB(FlatStore& store) const
	{
		int32 count = fUsersByID.size();

		size_t entriesSpace = sizeof(passwd*) * count;
		size_t offset = store.ReserveSpace(entriesSpace, true);
		passwd** entries = new passwd*[count];
		ArrayDeleter<passwd*> _(entries);

		int32 index = 0;
		for (map<uid_t, User*>::const_iterator it = fUsersByID.begin();
			 it != fUsersByID.end(); ++it) {
			entries[index++] = it->second->WriteFlatPasswd(store);
		}

		store.WriteData(offset, entries, entriesSpace);

		return count;
	}

	int32 WriteFlatShadowDB(FlatStore& store) const
	{
		int32 count = fUsersByID.size();

		size_t entriesSpace = sizeof(spwd*) * count;
		size_t offset = store.ReserveSpace(entriesSpace, true);
		spwd** entries = new spwd*[count];
		ArrayDeleter<spwd*> _(entries);

		int32 index = 0;
		for (map<uid_t, User*>::const_iterator it = fUsersByID.begin();
			 it != fUsersByID.end(); ++it) {
			entries[index++] = it->second->WriteFlatShadowPwd(store);
		}

		store.WriteData(offset, entries, entriesSpace);

		return count;
	}

	void WriteToDisk()
	{
		// rename the old files
		string passwdBackup(kPasswdFile);
		string shadowBackup(kShadowPwdFile);
		passwdBackup += ".old";
		shadowBackup += ".old";

		rename(kPasswdFile, passwdBackup.c_str());
		rename(kShadowPwdFile, shadowBackup.c_str());
			// Don't check errors. We can't do anything anyway.

		// open files
		FILE* passwdFile = fopen(kPasswdFile, "w");
		if (passwdFile == NULL) {
			debug_printf("REG: Failed to open passwd file \"%s\" for "
				"writing: %s\n", kPasswdFile, strerror(errno));
		}
		FileCloser _1(passwdFile);

		FILE* shadowFile = fopen(kShadowPwdFile, "w");
		if (shadowFile == NULL) {
			debug_printf("REG: Failed to open shadow passwd file \"%s\" for "
				"writing: %s\n", kShadowPwdFile, strerror(errno));
		}
		FileCloser _2(shadowFile);

		// write users
		for (map<uid_t, User*>::const_iterator it = fUsersByID.begin();
			 it != fUsersByID.end(); ++it) {
			User* user = it->second;
			user->WritePasswdLine(passwdFile);
			user->WriteShadowPwdLine(shadowFile);
		}
	}

private:
	map<uid_t, User*>	fUsersByID;
	map<string, User*>	fUsersByName;
};


class AuthenticationManager::GroupDB {
public:
	status_t AddGroup(Group* group)
	{
		try {
			fGroupsByID[group->GID()] = group;
		} catch (...) {
			return B_NO_MEMORY;
		}

		try {
			fGroupsByName[group->Name()] = group;
		} catch (...) {
			fGroupsByID.erase(fGroupsByID.find(group->GID()));
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void RemoveGroup(Group* group)
	{
		fGroupsByID.erase(fGroupsByID.find(group->GID()));
		fGroupsByName.erase(fGroupsByName.find(group->Name()));
	}

	bool UserRemoved(const std::string& user)
	{
		bool changed = false;
		for (map<gid_t, Group*>::const_iterator it = fGroupsByID.begin();
			 it != fGroupsByID.end(); ++it) {
			Group* group = it->second;
			changed |= group->MemberRemoved(user);
		}
		return changed;
	}

	Group* GroupByID(gid_t gid) const
	{
		map<gid_t, Group*>::const_iterator it = fGroupsByID.find(gid);
		return (it == fGroupsByID.end() ? NULL : it->second);
	}

	Group* GroupByName(const char* name) const
	{
		map<string, Group*>::const_iterator it = fGroupsByName.find(name);
		return (it == fGroupsByName.end() ? NULL : it->second);
	}

	int32 GetUserGroups(const char* name, gid_t* groups, int maxCount)
	{
		int count = 0;

		for (map<gid_t, Group*>::const_iterator it = fGroupsByID.begin();
			 it != fGroupsByID.end(); ++it) {
			Group* group = it->second;
			if (group->HasMember(name)) {
				if (count < maxCount)
					groups[count] = group->GID();
				count++;
			}
		}

		return count;
	}


	int32 WriteFlatGroupDB(FlatStore& store) const
	{
		int32 count = fGroupsByID.size();

		size_t entriesSpace = sizeof(group*) * count;
		size_t offset = store.ReserveSpace(entriesSpace, true);
		group** entries = new group*[count];
		ArrayDeleter<group*> _(entries);

		int32 index = 0;
		for (map<gid_t, Group*>::const_iterator it = fGroupsByID.begin();
			 it != fGroupsByID.end(); ++it) {
			entries[index++] = it->second->WriteFlatGroup(store);
		}

		store.WriteData(offset, entries, entriesSpace);

		return count;
	}

	void WriteToDisk()
	{
		// rename the old files
		string groupBackup(kGroupFile);
		groupBackup += ".old";

		rename(kGroupFile, groupBackup.c_str());
			// Don't check errors. We can't do anything anyway.

		// open file
		FILE* groupFile = fopen(kGroupFile, "w");
		if (groupFile == NULL) {
			debug_printf("REG: Failed to open group file \"%s\" for "
				"writing: %s\n", kGroupFile, strerror(errno));
		}
		FileCloser _1(groupFile);

		// write groups
		for (map<gid_t, Group*>::const_iterator it = fGroupsByID.begin();
			it != fGroupsByID.end(); ++it) {
			Group* group = it->second;
			group->WriteGroupLine(groupFile);
		}
	}

private:
	map<uid_t, Group*>	fGroupsByID;
	map<string, Group*>	fGroupsByName;
};


AuthenticationManager::AuthenticationManager()
	:
	fRequestPort(-1),
	fRequestThread(-1),
	fUserDB(NULL),
	fGroupDB(NULL),
	fPasswdDBReply(NULL),
	fGroupDBReply(NULL),
	fShadowPwdDBReply(NULL)
{
}


AuthenticationManager::~AuthenticationManager()
{
	// Quit the request thread and wait for it to finish
	write_port(fRequestPort, 'quit', NULL, 0);
	wait_for_thread(fRequestThread, NULL);

	delete fUserDB;
	delete fGroupDB;
	delete fPasswdDBReply;
	delete fGroupDBReply;
	delete fShadowPwdDBReply;
}


status_t
AuthenticationManager::Init()
{
	fUserDB = new(std::nothrow) UserDB;
	fGroupDB = new(std::nothrow) GroupDB;
	fPasswdDBReply = new(std::nothrow) KMessage(1);
	fGroupDBReply = new(std::nothrow) KMessage(1);
	fShadowPwdDBReply = new(std::nothrow) KMessage(1);

	if (fUserDB == NULL || fGroupDB == NULL || fPasswdDBReply == NULL
			|| fGroupDBReply == NULL || fShadowPwdDBReply == NULL) {
		return B_NO_MEMORY;
	}

	fRequestPort = BLaunchRoster().GetPort(
		B_REGISTRAR_AUTHENTICATION_PORT_NAME);
	if (fRequestPort < 0)
		return fRequestPort;

	fRequestThread = spawn_thread(&_RequestThreadEntry,
		"authentication manager", B_NORMAL_PRIORITY + 1, this);
	if (fRequestThread < 0)
		return fRequestThread;

	resume_thread(fRequestThread);

	return B_OK;
}


status_t
AuthenticationManager::_RequestThreadEntry(void* data)
{
	return ((AuthenticationManager*)data)->_RequestThread();
}


status_t
AuthenticationManager::_RequestThread()
{
	// read the DB files
	_InitPasswdDB();
	_InitGroupDB();
	_InitShadowPwdDB();

    // get our team ID
	team_id registrarTeam = -1;
	{
		thread_info info;
		if (get_thread_info(find_thread(NULL), &info) == B_OK)
			registrarTeam = info.team;
	}

	// request loop
	while (true) {
		KMessage message;
		port_message_info messageInfo;
		status_t error = message.ReceiveFrom(fRequestPort, -1, &messageInfo);
		if (error != B_OK)
			return B_OK;

		bool isRoot = (messageInfo.sender == 0);

		switch (message.What()) {
			case B_REG_GET_PASSWD_DB:
			{
				// lazily build the reply
				try {
					if (fPasswdDBReply->What() == 1) {
						FlatStore store;
						int32 count = fUserDB->WriteFlatPasswdDB(store);
						if (fPasswdDBReply->AddInt32("count", count) != B_OK
							|| fPasswdDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fPasswdDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fPasswdDBReply, -1, -1, 0, registrarTeam);
				} else {
					_InvalidatePasswdDBReply();
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}

			case B_REG_GET_GROUP_DB:
			{
				// lazily build the reply
				try {
					if (fGroupDBReply->What() == 1) {
						FlatStore store;
						int32 count = fGroupDB->WriteFlatGroupDB(store);
						if (fGroupDBReply->AddInt32("count", count) != B_OK
							|| fGroupDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fGroupDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fGroupDBReply, -1, -1, 0, registrarTeam);
				} else {
					_InvalidateGroupDBReply();
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}


			case B_REG_GET_SHADOW_PASSWD_DB:
			{
				// only root may see the shadow passwd
				if (!isRoot)
					error = EPERM;

				// lazily build the reply
				try {
					if (error == B_OK && fShadowPwdDBReply->What() == 1) {
						FlatStore store;
						int32 count = fUserDB->WriteFlatShadowDB(store);
						if (fShadowPwdDBReply->AddInt32("count", count) != B_OK
							|| fShadowPwdDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fShadowPwdDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fShadowPwdDBReply, -1, -1, 0,
						registrarTeam);
				} else {
					_InvalidateShadowPwdDBReply();
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}

			case B_REG_GET_USER:
			{
				User* user = NULL;
				int32 uid;
				const char* name;

				// find user
				if (message.FindInt32("uid", &uid) == B_OK) {
					user = fUserDB->UserByID(uid);
				} else if (message.FindString("name", &name) == B_OK) {
					user = fUserDB->UserByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && user == NULL)
					error = ENOENT;

				bool getShadowPwd = message.GetBool("shadow", false);

				// only root may see the shadow passwd
				if (error == B_OK && getShadowPwd && !isRoot)
					error = EPERM;

				// add user to message
				KMessage reply;
				if (error == B_OK)
					error = user->WriteToMessage(reply, getShadowPwd);

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_GET_GROUP:
			{
				Group* group = NULL;
				int32 gid;
				const char* name;

				// find group
				if (message.FindInt32("gid", &gid) == B_OK) {
					group = fGroupDB->GroupByID(gid);
				} else if (message.FindString("name", &name) == B_OK) {
					group = fGroupDB->GroupByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && group == NULL)
					error = ENOENT;

				// add group to message
				KMessage reply;
				if (error == B_OK)
					error = group->WriteToMessage(reply);

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_GET_USER_GROUPS:
			{
				// get user name
				const char* name;
				int32 maxCount;
				if (message.FindString("name", &name) != B_OK
					|| message.FindInt32("max count", &maxCount) != B_OK
					|| maxCount <= 0) {
					error = B_BAD_VALUE;
				}

				// get groups
				gid_t groups[NGROUPS_MAX + 1];
				int32 count = 0;
				if (error == B_OK) {
					maxCount = min_c(maxCount, NGROUPS_MAX + 1);
					count = fGroupDB->GetUserGroups(name, groups, maxCount);
				}

				// add groups to message
				KMessage reply;
				if (error == B_OK) {
					if (reply.AddInt32("count", count) != B_OK
						|| reply.AddData("groups", B_INT32_TYPE,
								groups, min_c(maxCount, count) * sizeof(gid_t),
								false) != B_OK) {
						error = B_NO_MEMORY;
					}
				}

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_UPDATE_USER:
			{
				// find user
				User* user = NULL;
				int32 uid;
				const char* name;

				if (message.FindInt32("uid", &uid) == B_OK) {
					user = fUserDB->UserByID(uid);
				} else if (message.FindString("name", &name) == B_OK) {
					user = fUserDB->UserByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				// only root can change anything
				if (error == B_OK && !isRoot)
					error = EPERM;

				// check addUser vs. existing user
				bool addUser = message.GetBool("add user", false);
				if (error == B_OK) {
					if (addUser) {
						if (user != NULL)
							error = EEXIST;
					} else if (user == NULL)
						error = ENOENT;
				}

				// apply all changes
				if (error == B_OK) {
					// clone the user object and update it from the message
					User* oldUser = user;
					user = NULL;
					try {
						user = (oldUser != NULL ? new User(*oldUser)
							: new User);
						user->UpdateFromMessage(message);

						// uid and name should remain the same
						if (oldUser != NULL) {
							if (oldUser->UID() != user->UID()
								|| oldUser->Name() != user->Name()) {
								error = B_BAD_VALUE;
							}
						}

						// replace the old user and write DBs to disk
						if (error == B_OK) {
							fUserDB->AddUser(user);
							fUserDB->WriteToDisk();
							_InvalidatePasswdDBReply();
							_InvalidateShadowPwdDBReply();
						}
					} catch (...) {
						error = B_NO_MEMORY;
					}

					if (error == B_OK)
						delete oldUser;
					else
						delete user;
				}

				// send reply
				KMessage reply;
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_DELETE_USER:
			{
				// find user
				User* user = NULL;
				int32 uid;
				const char* name;

				if (message.FindInt32("uid", &uid) == B_OK) {
					user = fUserDB->UserByID(uid);
				} else if (message.FindString("name", &name) == B_OK) {
					user = fUserDB->UserByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && user == NULL)
					error = ENOENT;

				// only root can change anything
				if (error == B_OK && !isRoot)
					error = EPERM;

				// apply the change
				if (error == B_OK) {
					std::string userName = user->Name();

					fUserDB->RemoveUser(user);
					fUserDB->WriteToDisk();
					_InvalidatePasswdDBReply();
					_InvalidateShadowPwdDBReply();

					if (fGroupDB->UserRemoved(userName)) {
						fGroupDB->WriteToDisk();
						_InvalidateGroupDBReply();
					}
				}

				// send reply
				KMessage reply;
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_UPDATE_GROUP:
			{
				// find group
				Group* group = NULL;
				int32 gid;
				const char* name;

				if (message.FindInt32("gid", &gid) == B_OK) {
					group = fGroupDB->GroupByID(gid);
				} else if (message.FindString("name", &name) == B_OK) {
					group = fGroupDB->GroupByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				// only root can change anything
				if (error == B_OK && !isRoot)
					error = EPERM;

				// check addGroup vs. existing group
				bool addGroup = message.GetBool("add group", false);
				if (error == B_OK) {
					if (addGroup) {
						if (group != NULL)
							error = EEXIST;
					} else if (group == NULL)
						error = ENOENT;
				}

				// apply all changes
				if (error == B_OK) {
					// clone the group object and update it from the message
					Group* oldGroup = group;
					group = NULL;
					try {
						group = (oldGroup != NULL ? new Group(*oldGroup)
							: new Group);
						group->UpdateFromMessage(message);

						// gid and name should remain the same
						if (oldGroup != NULL) {
							if (oldGroup->GID() != group->GID()
								|| oldGroup->Name() != group->Name()) {
								error = B_BAD_VALUE;
							}
						}

						// replace the old group and write DBs to disk
						if (error == B_OK) {
							fGroupDB->AddGroup(group);
							fGroupDB->WriteToDisk();
							_InvalidateGroupDBReply();
						}
					} catch (...) {
						error = B_NO_MEMORY;
					}

					if (error == B_OK)
						delete oldGroup;
					else
						delete group;
				}

				// send reply
				KMessage reply;
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_DELETE_GROUP:
			{
				// find group
				Group* group = NULL;
				int32 gid;
				const char* name;

				if (message.FindInt32("gid", &gid) == B_OK) {
					group = fGroupDB->GroupByID(gid);
				} else if (message.FindString("name", &name) == B_OK) {
					group = fGroupDB->GroupByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && group == NULL)
					error = ENOENT;

				// only root can change anything
				if (error == B_OK && !isRoot)
					error = EPERM;

				// apply the change
				if (error == B_OK) {
					fGroupDB->RemoveGroup(group);
					fGroupDB->WriteToDisk();
					_InvalidateGroupDBReply();
				}

				// send reply
				KMessage reply;
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			default:
				debug_printf("REG: invalid message: %" B_PRIu32 "\n",
					message.What());
		}
	}
}


status_t
AuthenticationManager::_InitPasswdDB()
{
	FILE* file = fopen(kPasswdFile, "r");
	if (file == NULL) {
		debug_printf("REG: Failed to open passwd DB file \"%s\": %s\n",
			kPasswdFile, strerror(errno));
		return errno;
	}
	FileCloser _(file);

	char lineBuffer[LINE_MAX];
	while (char* line = fgets(lineBuffer, sizeof(lineBuffer), file)) {
		if (strlen(line) == 0)
			continue;

		char* name;
		char* password;
		uid_t uid;
		gid_t gid;
		char* home;
		char* shell;
		char* realName;

		status_t error = parse_passwd_line(line, name, password, uid, gid,
			home, shell, realName);
		if (error != B_OK) {
			debug_printf("REG: Unparsable line in passwd DB file: \"%s\"\n",
				strerror(errno));
			continue;
		}

		User* user = NULL;
		try {
			user = new User(name, password, uid, gid, home, shell, realName);
		} catch (...) {
		}

		if (user == NULL || fUserDB->AddUser(user) != B_OK) {
			delete user;
			debug_printf("REG: Out of memory\n");
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
AuthenticationManager::_InitGroupDB()
{
	FILE* file = fopen(kGroupFile, "r");
	if (file == NULL) {
		debug_printf("REG: Failed to open group DB file \"%s\": %s\n",
			kGroupFile, strerror(errno));
		return errno;
	}
	FileCloser _(file);

	char lineBuffer[LINE_MAX];
	while (char* line = fgets(lineBuffer, sizeof(lineBuffer), file)) {
		if (strlen(line) == 0)
			continue;

		char* name;
		char* password;
		gid_t gid;
		char* members[MAX_GROUP_MEMBER_COUNT];
		int memberCount;


		status_t error = parse_group_line(line, name, password, gid, members,
			memberCount);
		if (error != B_OK) {
			debug_printf("REG: Unparsable line in group DB file: \"%s\"\n",
				strerror(errno));
			continue;
		}

		Group* group = NULL;
		try {
			group = new Group(name, password, gid, members, memberCount);
		} catch (...) {
		}

		if (group == NULL || fGroupDB->AddGroup(group) != B_OK) {
			delete group;
			debug_printf("REG: Out of memory\n");
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
AuthenticationManager::_InitShadowPwdDB()
{
	FILE* file = fopen(kShadowPwdFile, "r");
	if (file == NULL) {
		debug_printf("REG: Failed to open shadow passwd DB file \"%s\": %s\n",
			kShadowPwdFile, strerror(errno));
		return errno;
	}
	FileCloser _(file);

	char lineBuffer[LINE_MAX];
	while (char* line = fgets(lineBuffer, sizeof(lineBuffer), file)) {
		if (strlen(line) == 0)
			continue;

		char* name;
		char* password;
		int lastChanged;
		int min;
		int max;
		int warn;
		int inactive;
		int expiration;
		int flags;

		status_t error = parse_shadow_pwd_line(line, name, password,
			lastChanged, min, max, warn, inactive, expiration, flags);
		if (error != B_OK) {
			debug_printf("REG: Unparsable line in shadow passwd DB file: "
				"\"%s\"\n", strerror(errno));
			continue;
		}

		User* user = fUserDB->UserByName(name);
		if (user == NULL) {
			debug_printf("REG: shadow pwd entry for unknown user \"%s\"\n",
				name);
			continue;
		}

		try {
			user->SetShadowInfo(password, lastChanged, min, max, warn, inactive,
				expiration, flags);
		} catch (...) {
			debug_printf("REG: Out of memory\n");
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


void
AuthenticationManager::_InvalidatePasswdDBReply()
{
	fPasswdDBReply->SetTo(1);
}


void
AuthenticationManager::_InvalidateGroupDBReply()
{
	fGroupDBReply->SetTo(1);
}


void
AuthenticationManager::_InvalidateShadowPwdDBReply()
{
	fShadowPwdDBReply->SetTo(1);
}
