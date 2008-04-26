/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FAKE_AUTHENTICATION_MANAGER_H
#define FAKE_AUTHENTICATION_MANAGER_H


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
	class UserDB;
	class GroupDB;

private:
			port_id				fRequestPort;
			thread_id			fRequestThread;
			UserDB*				fUserDB;
			GroupDB*			fGroupDB;
			BPrivate::KMessage*	fPasswdDBReply;
			BPrivate::KMessage*	fGroupDBReply;
			BPrivate::KMessage*	fShadowPwdDBReply;
};


#endif	// FAKE_AUTHENTICATION_MANAGER_H
