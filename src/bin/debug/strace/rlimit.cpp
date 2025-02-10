/*
 * Copyright 2025, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Copyright 2025, Jérôme Duval, jerome.duval@gmail.com
 */


#include <sys/resource.h>

#include "strace.h"
#include "Syscall.h"
#include "Context.h"
#include "MemoryReader.h"
#include "TypeHandler.h"



static EnumTypeHandler::EnumMap kResourceNames;

struct enum_info {
	unsigned int index;
	const char *name;
};

#define ENUM_INFO_ENTRY(name) \
	{ name, #name }

static const enum_info kResources[] = {
	ENUM_INFO_ENTRY(RLIMIT_CORE),
	ENUM_INFO_ENTRY(RLIMIT_CPU),
	ENUM_INFO_ENTRY(RLIMIT_DATA),
	ENUM_INFO_ENTRY(RLIMIT_FSIZE),
	ENUM_INFO_ENTRY(RLIMIT_NOFILE),
	ENUM_INFO_ENTRY(RLIMIT_STACK),
	ENUM_INFO_ENTRY(RLIMIT_AS),
	ENUM_INFO_ENTRY(RLIMIT_NOVMON),

	{ 0, NULL }
};

static EnumTypeHandler::EnumMap kResourcesMap;


static string
format_number(rlim_t value)
{
	char tmp[32];
	if (value == RLIM_INFINITY) {
		strlcpy(tmp, "RLIM_INFINITY", sizeof(tmp));
	} else if ((value % 1024) == 0) {
		snprintf(tmp, sizeof(tmp), "%lu*1024", value / 1024);
	} else {
		snprintf(tmp, sizeof(tmp), "%lu", value);
	}
	return tmp;
}


static string
format_pointer(Context &context, struct rlimit *addr)
{
	string r;

	r += "rlim_cur=" + format_number(addr->rlim_cur) + ", ";
	r += "rlim_max=" + format_number(addr->rlim_max);

	return r;
}

static string
read_rlimit(Context &context, Parameter *param, void *address)
{
	struct rlimit data;

	if (param->Out()) {
		int status = context.GetReturnValue();
		if (status < 0)
			return string();
	}

	int32 bytesRead;
	status_t err = context.Reader().Read(address, &data, sizeof(struct rlimit), bytesRead);
	if (err != B_OK)
		return context.FormatPointer(address);

	return "{" + format_pointer(context, &data) + "}";
}


template<>
string
TypeHandlerImpl<struct rlimit *>::GetParameterValue(Context &context, Parameter *param,
	const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_rlimit(context, param, data);
	return context.FormatPointer(data);
}


template<>
string
TypeHandlerImpl<struct rlimit *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


DEFINE_TYPE(rlimit_ptr, rlimit *)


void
patch_rlimit()
{
	for (int i = 0; kResources[i].name != NULL; i++) {
		kResourcesMap[kResources[i].index] = kResources[i].name;
	}

	Syscall *getrlimit = get_syscall("_kern_getrlimit");
	getrlimit->GetParameter("resource")->SetHandler(new EnumTypeHandler(kResourcesMap));
	getrlimit->ParameterAt(1)->SetOut(true);
	Syscall *setrlimit = get_syscall("_kern_setrlimit");
	setrlimit->GetParameter("resource")->SetHandler(new EnumTypeHandler(kResourcesMap));
}

