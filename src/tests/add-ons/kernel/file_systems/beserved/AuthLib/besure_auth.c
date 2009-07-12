#define _BUILDING_BESURE_

#include "FindDirectory.h"

#include "betalk.h"
#include "sysdepdefs.h"
#include "besure_auth.h"
#include "md5.h"

#include "netdb.h"
#include "utime.h"
#include "ctype.h"
#include "time.h"
#include "signal.h"
#include "stdlib.h"
#include "errno.h"
#include "socket.h"
#include "signal.h"
#include "stdio.h"

typedef struct
{
	char *key;
	char *value;
	int valLength;
} key_value;


static uint32 genUniqueId(char *str);
//static void strlwr(char *str);
static void removeFiles(char *folder, char *file);
static void recvAlarm(int signal);
static key_value *GetNextKey(FILE *fp);
static void FreeKeyValue(key_value *kv);
static int32 queryServers(void *data);


bool addUserToGroup(char *user, char *group)
{
	char userPath[B_PATH_NAME_LENGTH], groupPath[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, userPath, sizeof(userPath));
	strcpy(groupPath, userPath);
	strcat(userPath, "/domains/default/users/");
	strcat(userPath, user);
	strcat(groupPath, "/domains/default/groups/");
	strcat(groupPath, group);
	strcat(groupPath, "/");
	strcat(groupPath, user);

	strlwr(userPath);
	strlwr(groupPath);

	if (symlink(userPath, groupPath) == -1)
		return false;

	return true;
}

bool removeUserFromGroup(char *user, char *group)
{
	char userPath[B_PATH_NAME_LENGTH];

	strlwr(user);
	strlwr(group);

	// You can't remove someone from the group "everyone" -- sorry.
	if (strcmp(group, "everyone") == 0)
		return false;

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, userPath, sizeof(userPath));
	strcat(userPath, "/domains/default/groups/");
	strcat(userPath, group);
	strcat(userPath, "/");
	strcat(userPath, user);

	remove(userPath);

	return true;
}

bool isUserInGroup(char *user, char *group)
{
	char userPath[B_PATH_NAME_LENGTH];

	strlwr(user);
	strlwr(group);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, userPath, sizeof(userPath));
	strcat(userPath, "/domains/default/groups/");
	strcat(userPath, group);
	strcat(userPath, "/");
	strcat(userPath, user);

	return (access(userPath, 0) == 0);
}

bool createUser(char *user, char *password)
{
	char path[B_PATH_NAME_LENGTH];
	uint32 userID;

	// All usernames are saved in lowercase.
	strlwr(user);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);

	// Generate a user ID.
	userID = genUniqueId(user);

	SetKeyValue(path, "Password", password);
	SetKeyInt(path, "Flags", 0);
	SetKeyValue(path, "Home", "/boot/home");
	SetKeyValue(path, "Group", "everyone");
	SetKeyInt(path, "UID", userID);

//	fs_write_attr(file, "BEOS:TYPE", B_STRING_TYPE, 0, BT_MIME_USER, strlen(BT_MIME_USER));

	// Add the user to the everyone group.
	addUserToGroup(user, "everyone");
	return true;
}

bool createGroup(char *group)
{
	char path[B_PATH_NAME_LENGTH];
	uint32 groupID;

	// All usernames are saved in lowercase.
	strlwr(group);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups/");
	strcat(path, group);

	// Generate a group ID.
	groupID = genUniqueId(group);

	if (mkdir(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1)
		return false;

	strcat(path, "/.attrib");
	SetKeyInt(path, "GID", groupID);
//	fs_write_attr(file, "BEOS:TYPE", B_STRING_TYPE, 0, BT_MIME_GROUP, strlen(BT_MIME_GROUP));
	return true;
}

bool removeUser(char *user)
{
	DIR *dir;
	struct dirent *dirInfo;
	char path[B_PATH_NAME_LENGTH], groupPath[B_PATH_NAME_LENGTH];

	// All usernames are saved in lowercase.
	strlwr(user);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);

	// Remove the user file.
	remove(path);

	// Now remove all instances of this user from any groups to which
	// it belonged.
	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups");

	// Scan the list of groups.  For each group, use removeFiles() to remove
	// the user file from the that directory.
	dir = opendir(path);
	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (strcmp(dirInfo->d_name, ".") && strcmp(dirInfo->d_name, ".."))
			{
				sprintf(groupPath, "%s/%s", path, dirInfo->d_name);
				removeFiles(groupPath, user);
			}

		closedir(dir);
	}

	return true;
}

bool removeGroup(char *group)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups/");
	strcat(path, group);
	strlwr(path);

	removeFiles(path, NULL);
	remove(path);

	return true;
}

bool setUserFullName(char *user, char *fullName)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyValue(path, "FullName", fullName);
}

bool setUserDesc(char *user, char *desc)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyValue(path, "Description", desc);
}

bool setUserPassword(char *user, char *password)
{
	char path[B_PATH_NAME_LENGTH], code[35];

	// Encrypt the password using MD5.
	md5EncodeString(password, code);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyValue(path, "Password", code);
}

bool setUserFlags(char *user, uint32 flags)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyInt(path, "Flags", flags);
}

bool setUserDaysToExpire(char *user, uint32 days)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyInt(path, "DaysToExpire", days);
}

bool setUserHome(char *user, char *home)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyValue(path, "Home", home);
}

bool setUserGroup(char *user, char *group)
{
	char path[B_PATH_NAME_LENGTH];

	if (!addUserToGroup(user, group))
		return false;

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return SetKeyValue(path, "Group", group);
}

bool setGroupDesc(char *group, char *desc)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups/");
	strcat(path, group);
	strcat(path, "/.attrib");
	strlwr(path);

	return SetKeyValue(path, "Description", desc);
}

bool getUserFullName(char *user, char *fullName, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyValue(path, "FullName", fullName, bufSize);
}

bool getUserDesc(char *user, char *desc, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyValue(path, "Description", desc, bufSize);
}

bool getUserPassword(char *user, char *password, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyValue(path, "Password", password, bufSize);
}

bool getUserFlags(char *user, uint32 *flags)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyInt(path, "Flags", (int *) &flags);
}

bool getUserDaysToExpire(char *user, uint32 *days)
{
	char path[B_PATH_NAME_LENGTH];

	days = 0;

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyInt(path, "DaysToExpire", (int *) &days);
}

bool getUserHome(char *user, char *home, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyValue(path, "Home", home, bufSize);
}

bool getUserGroup(char *user, char *group, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users/");
	strcat(path, user);
	strlwr(path);

	return GetKeyValue(path, "Group", group, bufSize);
}

bool getGroupDesc(char *group, char *desc, int bufSize)
{
	char path[B_PATH_NAME_LENGTH];

	// All usernames are saved in lowercase.
	strlwr(group);

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups/");
	strcat(path, group);
	strcat(path, "/.attrib");

	return GetKeyValue(path, "Description", desc, bufSize);
}

// strlwr()
/*
static void strlwr(char *str)
{
	char *p;
	for (p = str; *p; p++)
		*p = tolower(*p);
}*/

// genUniqueId()
//
// This function generates a unique number based upon a string.  Currently,
// is uses the upper-most byte of a 4-byte integer to contain the length.
// This restricts the length of the supplied string to 256 characters.  Then,
// the sum of each character multiplied by its position in the string is
// calculated and occupies the remaining 3 bytes.
//
static uint32 genUniqueId(char *str)
{
	char *p;
	int i;
	uint32 value = 0;

	for (p = str, i = 0; *p; p++, i++)
		value += i * (int) *p;

	value = value | (i << 24);
	return value;
}

static void removeFiles(char *folder, char *file)
{
	DIR *dir;
	struct dirent *dirInfo;
	char path[B_PATH_NAME_LENGTH];

	// If we've only been given one file, then just delete it.
	if (file)
	{
		sprintf(path, "%s/%s", folder, file);
		remove(path);
		return;
	}

	// Otherwise, we need to query all files in the directory and
	// remove them.
	dir = opendir(folder);
	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (strcmp(dirInfo->d_name, ".") && strcmp(dirInfo->d_name, ".."))
			{
				sprintf(path, "%s/%s", folder, dirInfo->d_name);
				remove(path);
			}

		closedir(dir);
	}
}

// OpenUsers()
//
DIR *OpenUsers()
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/users");

	return opendir(path);
}

// ReadUser()
//
dirent_t *ReadUser(DIR *dir)
{
	struct dirent *dirInfo;

	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (dirInfo->d_name[0] !=  '.')
				return dirInfo;
	}

	return NULL;
}

// CloseUsers()
//
void CloseUsers(DIR *dir)
{
	if (dir)
		closedir(dir);
}

// OpenGroups()
//
DIR *OpenGroups()
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups");

	return opendir(path);
}

// ReadGroup()
//
dirent_t *ReadGroup(DIR *dir)
{
	struct dirent *dirInfo;

	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (dirInfo->d_name[0] != '.')
				return dirInfo;
	}

	return NULL;
}

// CloseGroups()
//
void CloseGroups(DIR *dir)
{
	if (dir)
		closedir(dir);
}

// OpenGroup()
//
DIR *OpenGroup(char *group)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/groups/");
	strcat(path, group);

	return opendir(path);
}

// ReadGroupMember()
//
dirent_t *ReadGroupMember(DIR *dir)
{
	struct dirent *dirInfo;

	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (dirInfo->d_name[0] != '.')
				return dirInfo;
	}

	return NULL;
}

// CloseGroup()
//
void CloseGroup(DIR *dir)
{
	if (dir)
		closedir(dir);
}

// isServerRecordingLogins()
//
bool isServerRecordingLogins(char *server)
{
	char path[B_PATH_NAME_LENGTH];
	char buffer[20];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/servers/");
	strcat(path, server);
	strlwr(path);

	GetKeyValue(path, "RecordLogins", buffer, sizeof(buffer));
	return strcasecmp(buffer, "Yes") == 0;
}

// setServerRecordingLogins()
//
bool setServerRecordingLogins(char *server, bool recording)
{
	char path[B_PATH_NAME_LENGTH];

	find_directory(B_COMMON_SYSTEM_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/domains/default/servers/");
	strcat(path, server);
	strlwr(path);

	return SetKeyValue(path, "RecordLogins", recording ? "Yes" : "No");
}

// MakeServerKey()
//
void MakeServerKey(char *server, char *key)
{
	struct hostent *ent;
	char str[256];

	srand(time(NULL));
	sprintf(str, "%s-%x-", server, rand() * 0xFFFFFFFF);
	ent = gethostbyname(server);
	strcat(str, ent ? ent->h_name : "Unknown");

	md5EncodeString(str, key);
}

// GetServerList()
//
int GetServerList(recordServerFunc recServ)
{
	thread_id queryThread = spawn_thread(queryServers, "Server Query", B_NORMAL_PRIORITY, recServ);
	resume_thread(queryThread);
	return 0;
}

int32 queryServers(void *data)
{
	bt_request request;
	recordServerFunc recServ = (recordServerFunc) data;
	struct sockaddr_in ourAddr, toAddr, fromAddr;
	struct hostent *ent;
	char response[256], hostname[B_FILE_NAME_LENGTH];
	int sock, addrLen, bytes;
#ifdef SO_BROADCAST
	int on = 1;
#endif

	memset(&toAddr, 0, sizeof(toAddr));
	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(BT_QUERYHOST_PORT);
	toAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return -1;

	memset(&ourAddr, 0, sizeof(ourAddr));
	ourAddr.sin_family = AF_INET;
	ourAddr.sin_port = htons(BT_QUERYHOST_PORT);
	ourAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &ourAddr, sizeof(ourAddr)))
		if (errno != EADDRINUSE)
			return -1;

	// Normally, a setsockopt() call is necessary to turn broadcast mode on
	// explicitly, although some versions of Unix don't care.  BeOS doesn't
	// currently even define SO_BROADCAST, unless you have BONE installed.
#ifdef SO_BROADCAST
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
	{
		closesocket(sock);
		return -1;
	}
#endif
	signal(SIGALRM, recvAlarm);

	strcpy(request.signature, BT_RPC_SIGNATURE);
	request.command = BT_REQ_HOST_PROBE;
	if (sendto(sock, (char *) &request, sizeof(request), 0, (struct sockaddr *) &toAddr, sizeof(toAddr)) == -1)
	{
		closesocket(sock);
		return -1;
	}

	memset(response, 0, sizeof(response));
	alarm(3);

	while (1)
	{
		addrLen = sizeof(fromAddr);
		bytes = recvfrom(sock, response, sizeof(response) - 1, 0, (struct sockaddr *) &fromAddr, &addrLen);
		if (bytes < 0)
		{
			if (errno == EINTR)
				break;
		}

		if (strncmp(response, BT_RPC_SIGNATURE, strlen(BT_RPC_SIGNATURE)) != 0)
		{
			struct sockaddr_in *sin = (struct sockaddr_in *) &fromAddr;
			ent = gethostbyaddr((char *) &sin->sin_addr, sizeof(sin->sin_addr), AF_INET);
			if (ent)
				(*recServ)(ent->h_name, *((uint32 *) &sin->sin_addr));
			else
			{
				uint8 *p = (uint8 *) &sin->sin_addr;
				sprintf(hostname, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
				(*recServ)(hostname, *((uint32 *) &sin->sin_addr));
			}
		}
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	closesocket(sock);
	return 0;
}

static void recvAlarm(int signal)
{
	return;
}

// ----- Key-Value Implementation ------------------------------------------------

bool GetKeyInt(const char *fileName, const char *key, int *value)
{
	char num[50];

	*value = 0;
	if (GetKeyValue(fileName, key, num, sizeof(num)))
	{
		*value = atoi(num);
		return true;
	}

	return false;
}

bool GetKeyValue(const char *fileName, const char *key, char *value, int bufSize)
{
	FILE *fp;
	key_value *kv;
	int length;

	value[0] = 0;

	fp = fopen(fileName, "r");
	if (!fp)
		return false;

	while ((kv = GetNextKey(fp)) != NULL)
		if (strcasecmp(kv->key, key) == 0)
		{
			length = min(bufSize - 1, kv->valLength);
			strncpy(value, kv->value, length);
			value[length] = 0;
			FreeKeyValue(kv);
			break;
		}
		else FreeKeyValue(kv);

	fclose(fp);
	return (*value != 0);
}

bool SetKeyInt(const char *fileName, const char *key, int value)
{
	char num[50];
	sprintf(num, "%d", value);
	return SetKeyValue(fileName, key, num);
}

bool SetKeyValue(const char *fileName, const char *key, const char *value)
{
	key_value *kv[100];
	FILE *fp;
	int i, j, length;
	bool found = false;

	i = 0;
	found = false;
	length = value ? strlen(value) : 0;

	fp = fopen(fileName, "r");
	if (fp)
	{
		while ((kv[i] = GetNextKey(fp)) != NULL)
		{
			if (strcasecmp(kv[i]->key, key) == 0)
			{
				found = true;
				free(kv[i]->value);
				kv[i]->value = (char *) malloc(strlen(value) + 1);
				if (kv[i]->value)
					if (length)
						strcpy(kv[i]->value, value);
					else
						kv[i]->value[0] = 0;

				kv[i]->valLength = length;
			}

			i++;
		}

		fclose(fp);
	}

	fp = fopen(fileName, "w");
	if (fp)
	{
		for (j = 0; j < i; j++)
			if (kv[j] && kv[j]->value && kv[j]->valLength)
			{
				fprintf(fp, "%s = %s\n", kv[j]->key, kv[j]->value);
				FreeKeyValue(kv[j]);
			}
	
		if (!found && length)
			fprintf(fp, "%s = %s\n", key, value);
	
		fclose(fp);
	}

	return true;
}

key_value *GetNextKey(FILE *fp)
{
	key_value *kv;
	char buffer[4096], key[35], *ch;
	int i, length;

	if (!fgets(buffer, sizeof(buffer), fp))
		return NULL;

	// The minimum length for a valid line is one character for the key, plus the
	// equal sign, plus one character for the value, or 3 characters.
	length = strlen(buffer);
	if (length < 3)
		return NULL;

	// Trim off any linefeed character.
	if (buffer[length - 1] == '\n')
		buffer[--length] = 0;

	// Now skip spaces before the key.
	ch = buffer;
	while (*ch && (*ch == ' ' || *ch == '\t'))
		ch++;

	for (i = 0; *ch && *ch != ' ' && *ch != '\t' && *ch != '='; i++, ch++)
		if (i < sizeof(key) - 1)
			key[i] = *ch;

	key[i] = 0;
	if (i == 0)
		return NULL;

	kv = (key_value *) malloc(sizeof(key_value));
	if (!kv)
		return NULL;

	kv->key = (char *) malloc(i + 1);
	if (!kv->key)
	{
		free(kv);
		return NULL;
	}

	strcpy(kv->key, key);

	// Skip to the equal sign.
	while (*ch && *ch != '=')
		ch++;

	if (*ch != '=')
	{
		free(kv->key);
		free(kv);
		return NULL;
	}

	ch++;
	while (*ch && (*ch == ' ' || *ch == '\t'))
		ch++;

	kv->valLength = buffer + length - ch;
	kv->value = (char *) malloc(kv->valLength + 1);
	if (!kv->value)
	{
		free(kv->key);
		free(kv);
		return NULL;
	}

	strcpy(kv->value, ch);
	return kv;
}

void FreeKeyValue(key_value *kv)
{
	if (kv->key)
		free(kv->key);

	if (kv->value)
		free(kv->value);

	free(kv);
}
