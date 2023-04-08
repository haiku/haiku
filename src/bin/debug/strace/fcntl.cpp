/*
 * Copyright 2022-2023, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Copyright 2022, Jérôme Duval, jerome.duval@gmail.com
 */


#include <fcntl.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"


#define FLAG_INFO_ENTRY(name) \
	{ name, #name }

static const FlagsTypeHandler::FlagInfo kOpenFlagInfos[] = {
	FLAG_INFO_ENTRY(O_WRONLY),
	FLAG_INFO_ENTRY(O_RDWR),

	FLAG_INFO_ENTRY(O_EXCL),
	FLAG_INFO_ENTRY(O_CREAT),
	FLAG_INFO_ENTRY(O_TRUNC),
	FLAG_INFO_ENTRY(O_NOCTTY),
	FLAG_INFO_ENTRY(O_NOTRAVERSE),

	FLAG_INFO_ENTRY(O_CLOEXEC),
	FLAG_INFO_ENTRY(O_NONBLOCK),
	FLAG_INFO_ENTRY(O_APPEND),
	FLAG_INFO_ENTRY(O_SYNC),
	FLAG_INFO_ENTRY(O_RSYNC),
	FLAG_INFO_ENTRY(O_DSYNC),
	FLAG_INFO_ENTRY(O_NOFOLLOW),
	FLAG_INFO_ENTRY(O_DIRECT),

	FLAG_INFO_ENTRY(O_DIRECTORY),

	{ 0, NULL }
};


struct fcntl_info {
	unsigned int index;
	const char *name;
	TypeHandler *handler;
};

#define FCNTL_INFO_ENTRY(name) \
	{ name, #name, NULL }

#define FCNTL_INFO_ENTRY_TYPE(name, type) \
	{ name, #name, TypeHandlerFactory<type>::Create() }

static const fcntl_info kFcntls[] = {
	FCNTL_INFO_ENTRY_TYPE(F_DUPFD, int),
	FCNTL_INFO_ENTRY(F_GETFD),
	FCNTL_INFO_ENTRY_TYPE(F_SETFD, int),
	FCNTL_INFO_ENTRY(F_GETFL),
	FCNTL_INFO_ENTRY(F_SETFL),
	FCNTL_INFO_ENTRY_TYPE(F_GETLK, struct flock*),
	FCNTL_INFO_ENTRY_TYPE(F_SETLK, struct flock*),
	FCNTL_INFO_ENTRY_TYPE(F_SETLKW, struct flock*),
	{ 0, NULL, NULL }
};

static FlagsTypeHandler::FlagsList kOpenFlags;
static EnumTypeHandler::EnumMap kFcntlNames;
static TypeHandlerSelector::SelectMap kFcntlTypeHandlers;

void
patch_fcntl()
{
	for (int i = 0; kOpenFlagInfos[i].name != NULL; i++) {
		kOpenFlags.push_back(kOpenFlagInfos[i]);
	}

	for (int i = 0; kFcntls[i].name != NULL; i++) {
		kFcntlNames[kFcntls[i].index] = kFcntls[i].name;
		if (kFcntls[i].handler != NULL)
			kFcntlTypeHandlers[kFcntls[i].index] = kFcntls[i].handler;
	}

	kFcntlTypeHandlers[F_SETFL] = new FlagsTypeHandler(kOpenFlags);

	Syscall *open = get_syscall("_kern_open");
	open->GetParameter("openMode")->SetHandler(new FlagsTypeHandler(kOpenFlags));

	Syscall *fcntl = get_syscall("_kern_fcntl");
	fcntl->GetParameter("op")->SetHandler(new EnumTypeHandler(kFcntlNames));
	fcntl->GetParameter("argument")->SetHandler(
		new TypeHandlerSelector(kFcntlTypeHandlers,
				1, TypeHandlerFactory<void *>::Create()));
}

