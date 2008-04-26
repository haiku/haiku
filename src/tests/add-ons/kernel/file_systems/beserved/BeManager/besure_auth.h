#ifndef _BESURE_AUTH_H_
#define _BESURE_AUTH_H_

#include <dirent.h>

#define USER_FLAG_DISABLED		0x00000001
#define USER_FLAG_INIT_EXPIRE	0x00000002
#define USER_FLAG_DAYS_EXPIRE	0x00000004
#define USER_FLAG_PW_LOCKED		0x00000008

#ifdef _BUILDING_BESURE_
#define _IMPEXP_BESURE		__declspec(dllexport)
#else
#define _IMPEXP_BESURE		__declspec(dllimport)
#endif

typedef void (*recordServerFunc)(char *name, uint32 addr);

#ifdef __cplusplus
extern "C"
{
#endif

_IMPEXP_BESURE bool createUser(char *user, char *password);
_IMPEXP_BESURE bool createGroup(char *group);
_IMPEXP_BESURE bool removeUser(char *user);
_IMPEXP_BESURE bool removeGroup(char *group);
_IMPEXP_BESURE bool addUserToGroup(char *user, char *group);
_IMPEXP_BESURE bool removeUserFromGroup(char *user, char *group);
_IMPEXP_BESURE bool isUserInGroup(char *user, char *group);

_IMPEXP_BESURE bool setUserFullName(char *user, char *fullName);
_IMPEXP_BESURE bool setUserDesc(char *user, char *desc);
_IMPEXP_BESURE bool setUserPassword(char *user, char *password);
_IMPEXP_BESURE bool setUserFlags(char *user, uint32 flags);
_IMPEXP_BESURE bool setUserDaysToExpire(char *user, uint32 days);
_IMPEXP_BESURE bool setUserHome(char *user, char *home);
_IMPEXP_BESURE bool setUserGroup(char *user, char *group);
_IMPEXP_BESURE bool setGroupDesc(char *group, char *desc);

_IMPEXP_BESURE bool getUserFullName(char *user, char *fullName, int bufSize);
_IMPEXP_BESURE bool getUserDesc(char *user, char *desc, int bufSize);
_IMPEXP_BESURE bool getUserPassword(char *user, char *password, int bufSize);
_IMPEXP_BESURE bool getUserFlags(char *user, uint32 *flags);
_IMPEXP_BESURE bool getUserDaysToExpire(char *user, uint32 *days);
_IMPEXP_BESURE bool getUserHome(char *user, char *home, int bufSize);
_IMPEXP_BESURE bool getUserGroup(char *user, char *group, int bufSize);
_IMPEXP_BESURE bool getGroupDesc(char *group, char *desc, int bufSize);

_IMPEXP_BESURE bool isServerRecordingLogins(char *server);
_IMPEXP_BESURE bool setServerRecordingLogins(char *server, bool recording);

_IMPEXP_BESURE void MakeServerKey(char *server, char *key);

_IMPEXP_BESURE DIR *OpenUsers();
_IMPEXP_BESURE dirent_t *ReadUser(DIR *dir);
_IMPEXP_BESURE void CloseUsers(DIR *dir);
_IMPEXP_BESURE DIR *OpenGroups();
_IMPEXP_BESURE dirent_t *ReadGroup(DIR *dir);
_IMPEXP_BESURE void CloseGroups(DIR *dir);
_IMPEXP_BESURE DIR *OpenGroup(char *group);
_IMPEXP_BESURE dirent_t *ReadGroupMember(DIR *dir);
_IMPEXP_BESURE void CloseGroup(DIR *dir);
_IMPEXP_BESURE int GetServerList(recordServerFunc recServ);

_IMPEXP_BESURE bool GetKeyValue(const char *fileName, const char *key, char *value, int bufSize);
_IMPEXP_BESURE bool GetKeyInt(const char *fileName, const char *key, int *value);
_IMPEXP_BESURE bool SetKeyValue(const char *fileName, const char *key, const char *value);
_IMPEXP_BESURE bool SetKeyInt(const char *fileName, const char *key, int value);

#ifdef __cplusplus
}
#endif

#endif
