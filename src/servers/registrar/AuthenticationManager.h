/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H


#include <OS.h>


namespace BPrivate {
	class KMessage;
}


class AuthenticationManager {
public:
								AuthenticationManager();
								~AuthenticationManager();

			status_t			Init();

private:
	class FlatStore;
	class User;
	class Group;
	class UserDB;
	class GroupDB;

	static	status_t			_RequestThreadEntry(void* data);
			status_t			_RequestThread();

			status_t			_InitPasswdDB();
			status_t			_InitGroupDB();
			status_t			_InitShadowPwdDB();

private:
			port_id				fRequestPort;
			thread_id			fRequestThread;
			UserDB*				fUserDB;
			GroupDB*			fGroupDB;
			BPrivate::KMessage*	fPasswdDBReply;
			BPrivate::KMessage*	fGroupDBReply;
			BPrivate::KMessage*	fShadowPwdDBReply;
};


#endif	// AUTHENTICATION_MANAGER_H
