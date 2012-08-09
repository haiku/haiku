/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef IDMAP_H
#define IDMAP_H


#include <lock.h>
#include <port.h>
#include <SupportDefs.h>


class IdMap {
public:
					IdMap();
					~IdMap();

		uid_t		GetUserId(const char* owner);
		gid_t		GetGroupId(const char* ownerGroup);

		char*		GetOwner(uid_t user);
		char*		GetOwnerGroup(gid_t group);

private:
		status_t	_Repair();

		template<typename T>
		void*		_GetBuffer(T value, int32 code);

		template<typename T>
		T			_GetValue(const char* buffer, int32 code);

		mutex		fLock;

		port_id		fRequestPort;
		port_id		fReplyPort;
};

extern IdMap*	gIdMapper;
extern mutex	gIdMapperLock;


#endif	// IDMAP_H

