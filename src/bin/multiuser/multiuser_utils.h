/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MULTIUSER_UTILS_H
#define MULTIUSER_UTILS_H

#include <pwd.h>
#include <shadow.h>
#include <stdio.h>

#include <SupportDefs.h>


status_t read_password(const char* prompt, char* password, size_t bufferSize,
			bool useStdio);

bool verify_password(passwd* passwd, spwd* spwd, const char* plainPassword);

status_t authenticate_user(const char* prompt, passwd* passwd, spwd* spwd,
			int maxTries, bool useStdio);
status_t authenticate_user(const char* prompt, const char* user,
			passwd** _passwd, spwd** _spwd, int maxTries, bool useStdio);

status_t setup_environment(struct passwd* passwd, bool preserveEnvironment,
			bool chngdir = true);


#endif	// MULTIUSER_UTILS_H
