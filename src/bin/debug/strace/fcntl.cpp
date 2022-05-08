/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Copyright 2022, Jérôme Duval, jerome.duval@gmail.com
 */


#include <fcntl.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"


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
	FCNTL_INFO_ENTRY_TYPE(F_SETFL, int),
	FCNTL_INFO_ENTRY_TYPE(F_GETLK, struct flock*),
	FCNTL_INFO_ENTRY_TYPE(F_SETLK, struct flock*),
	FCNTL_INFO_ENTRY_TYPE(F_SETLKW, struct flock*),
	{ 0, NULL, NULL }
};

static EnumTypeHandler::EnumMap kFcntlNames;
static TypeHandlerSelector::SelectMap kFcntlTypeHandlers;

void
patch_fcntl()
{
	for (int i = 0; kFcntls[i].name != NULL; i++) {
		kFcntlNames[kFcntls[i].index] = kFcntls[i].name;
		if (kFcntls[i].handler != NULL)
			kFcntlTypeHandlers[kFcntls[i].index] = kFcntls[i].handler;
	}

	Syscall *fcntl = get_syscall("_kern_fcntl");

	fcntl->GetParameter("op")->SetHandler(
			new EnumTypeHandler(kFcntlNames));
	fcntl->GetParameter("argument")->SetHandler(
			new TypeHandlerSelector(kFcntlTypeHandlers,
					1, TypeHandlerFactory<void *>::Create()));
}

