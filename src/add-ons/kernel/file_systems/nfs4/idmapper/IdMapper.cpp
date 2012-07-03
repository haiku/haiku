/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "IdMapper.h"

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <File.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>


port_id		gRequestPort;
port_id		gReplyPort;

const char*	kNobodyName		= "nobody";
uid_t		gNobodyId;

const char*	kNogroupName	= "nobody";
uid_t		gNogroupId;

const char* gDomainName		= "localdomain";


status_t
SendError(status_t error)
{
	return write_port(gReplyPort, MsgError, &error, sizeof(error));
}


status_t
MatchDomain(char* name)
{
	char* domain = strchr(name, '@');
	if (domain == NULL)
		return B_MISMATCHED_VALUES;

	if (strcmp(domain + 1, gDomainName) != 0)
		return B_BAD_VALUE;

	*domain = '\0';

	return B_OK;
}


char*
AddDomain(const char* name)
{
	uint32 fullLength = strlen(name) + strlen(gDomainName) + 2;
	char* fullName = reinterpret_cast<char*>(malloc(fullLength));

	strcpy(fullName, name);
	strcat(fullName, "@");
	strcat(fullName, gDomainName);

	return fullName;
}


status_t
NameToUID(void* buffer)
{
	char* userName = reinterpret_cast<char*>(buffer);

	struct passwd* userInfo = NULL;

	if (MatchDomain(userName) == B_OK)
		userInfo = getpwnam(userName);

	if (userInfo == NULL)
		return write_port(gReplyPort, MsgReply, &gNobodyId, sizeof(gNobodyId));

	return write_port(gReplyPort, MsgReply, &userInfo->pw_uid, sizeof(uid_t));
}


status_t
UIDToName(void* buffer)
{
	uid_t userId = *reinterpret_cast<uid_t*>(buffer);

	const char* fullName = kNobodyName;

	struct passwd* userInfo = getpwuid(userId);
	if (userInfo != NULL) {
		const char* name = userInfo->pw_name;
		fullName = AddDomain(name);
	}

	status_t result = write_port(gReplyPort, MsgReply, fullName,
		strlen(fullName) + 1);
	free(const_cast<char*>(fullName));

	return result;
}


status_t
NameToGID(void* buffer)
{
	char* groupName = reinterpret_cast<char*>(buffer);
	
	struct group* groupInfo = NULL;

	if (MatchDomain(groupName) == B_OK)
		groupInfo = getgrnam(groupName);

	if (groupInfo == NULL) {
		return write_port(gReplyPort, MsgReply, &gNogroupId,
			sizeof(gNogroupId));
	}

	return write_port(gReplyPort, MsgReply, &groupInfo->gr_gid, sizeof(gid_t));
}


status_t
GIDToName(void* buffer)
{
	gid_t groupId = *reinterpret_cast<gid_t*>(buffer);

	const char* fullName = kNogroupName;

	struct group* groupInfo = getgrgid(groupId);
	if (groupInfo != NULL) {
		const char* name = groupInfo->gr_name;
		fullName = AddDomain(name);
	}

	status_t result = write_port(gReplyPort, MsgReply, fullName,
		strlen(fullName) + 1);
	free(const_cast<char*>(fullName));

	return result;
}


status_t
ParseRequest(int32 code, void* buffer)
{
	switch (code) {
		case MsgNameToUID:
			return NameToUID(buffer);

		case MsgUIDToName:
			return UIDToName(buffer);

		case MsgNameToGID:
			return NameToGID(buffer);

		case MsgGIDToName:
			return GIDToName(buffer);

		default:
			return SendError(B_BAD_VALUE);
	}
}


status_t
MainLoop()
{
	do {
		ssize_t size = port_buffer_size(gRequestPort);
		if (size < B_OK)
			return 0;

		void* buffer = malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;

		int32 code;
		status_t result = read_port(gRequestPort, &code, buffer, size);
		if (size < B_OK) {
			free(buffer);
			return 0;
		}

		result = ParseRequest(code, buffer);
		free(buffer);

		if (result == B_BAD_PORT_ID)
			return 0;

	} while (true);
}


status_t
ReadSettings()
{
	BPath path;
	status_t result = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
	if (result != B_OK)
		return result;
	result = path.Append("nfs4_idmapper.conf");
	if (result != B_OK)
		return result;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return file.InitCheck();

	off_t size;
	result = file.GetSize(&size);
	if (result != B_OK)
		return result;

	void* buffer = malloc(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	file.Read(buffer, size);

	gDomainName = reinterpret_cast<char*>(buffer);

	return B_OK;
}


int
main(int argc, char** argv)
{
	gRequestPort = find_port(kRequestPortName);
	if (gRequestPort == B_NAME_NOT_FOUND) {
		fprintf(stderr, "%s\n", strerror(gRequestPort));
		return gRequestPort;
	}

	gReplyPort = find_port(kReplyPortName);
	if (gReplyPort == B_NAME_NOT_FOUND) {
		fprintf(stderr, "%s\n", strerror(gReplyPort));
		return gReplyPort;
	}

	ReadSettings();

	struct passwd* userInfo = getpwnam(kNobodyName);
	if (userInfo != NULL)
		gNobodyId = userInfo->pw_uid;
	else
		gNobodyId = 0;

	struct group* groupInfo = getgrnam(kNogroupName);
	if (groupInfo != NULL)
		gNogroupId = groupInfo->gr_gid;
	else
		gNogroupId = 0;

	return MainLoop();
}

