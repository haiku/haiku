/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "IdMap.h"

#include <FindDirectory.h>
#include <team.h>
#include <util/AutoLock.h>

#include "idmapper/IdMapper.h"


IdMap*	gIdMapper	= NULL;


IdMap::IdMap()
{
	mutex_init(&fLock, NULL);
	_Repair();
}


IdMap::~IdMap()
{
	delete_port(fRequestPort);
	delete_port(fReplyPort);
	mutex_destroy(&fLock);
}


uid_t
IdMap::GetUserId(const char* owner)
{
	return _GetValue<uid_t>(owner, MsgNameToUID);
}


gid_t
IdMap::GetGroupId(const char* ownerGroup)
{
	return _GetValue<gid_t>(ownerGroup, MsgNameToGID);
}


char*
IdMap::GetOwner(uid_t user)
{
	return reinterpret_cast<char*>(_GetBuffer(user, MsgUIDToName));
}


char*
IdMap::GetOwnerGroup(gid_t group)
{
	return reinterpret_cast<char*>(_GetBuffer(group, MsgGIDToName));
}


template<typename T>
T
IdMap::_GetValue(const char* buffer, int32 code)
{
	MutexLocker _(fLock);
	do {
		status_t result = write_port(fRequestPort, MsgNameToUID, buffer,
			strlen(buffer) + 1);
		if (result != B_OK) {
			if (_Repair() != B_OK)
				return 0;
			else
				continue;
		}

		int32 code;
		T value;
		result = read_port(fReplyPort, &code, &value, sizeof(T));
		if (result < B_OK) {
			if (_Repair() != B_OK)
				return 0;
			else
				continue;
		}

		if (code != MsgReply)
			return 0;
		else
			return value;
	} while (true);
}


template<typename T>
void*
IdMap::_GetBuffer(T value, int32 code)
{
	MutexLocker _(fLock);
	do {
		status_t result = write_port(fRequestPort, code, &value, sizeof(value));
		if (result != B_OK) {
			if (_Repair() != B_OK)
				return NULL;
			else
				continue;
		}

		ssize_t size = port_buffer_size(fReplyPort);
		if (size < B_OK) {
			if (_Repair() != B_OK)
				return NULL;
			else
				continue;
		}

		int32 code;
		void* buffer = malloc(size);
		if (buffer == NULL)
			return NULL;

		result = read_port(fReplyPort, &code, buffer, size);
		if (result < B_OK) {
			free(buffer);

			if (_Repair() != B_OK)
				return 0;
			else
				continue;
		}

		if (code != MsgReply) {
			free(buffer);
			return NULL;
		} else
			return buffer;
	} while (true);
}


status_t
IdMap::_Repair()
{
	status_t result = B_OK;

	fRequestPort = create_port(1, kRequestPortName);
	if (fRequestPort < B_OK)
		return fRequestPort;

	fReplyPort = create_port(1, kReplyPortName);
	if (fReplyPort < B_OK) {
		delete_port(fRequestPort);
		return fReplyPort;
	}

	char path[256];
	if (find_directory(B_SYSTEM_SERVERS_DIRECTORY, static_cast<dev_t>(-1),
		false, path, sizeof(path)) != B_OK) {
		delete_port(fReplyPort);
		delete_port(fRequestPort);
		return B_NAME_NOT_FOUND;
	}
	strlcat(path, "/nfs4_idmapper_server", sizeof(path));

	const char* args[] = { path, NULL };
	thread_id thread = load_image_etc(1, args, NULL, B_NORMAL_PRIORITY,
		B_SYSTEM_TEAM, 0);
	if (thread < B_OK) {
		delete_port(fReplyPort);
		delete_port(fRequestPort);
		return thread;
	}

	set_port_owner(fRequestPort, thread);
	set_port_owner(fReplyPort, thread);

	result = resume_thread(thread);
	if (result != B_OK) {
		kill_thread(thread);
		delete_port(fReplyPort);
		delete_port(fRequestPort);
		return result;
	}

	return B_OK;
}

