/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <user_mutex_defs.h>

#include "strace.h"
#include "Context.h"
#include "MemoryReader.h"
#include "TypeHandler.h"


#define FLAG_INFO_ENTRY(name) \
	{ name, #name }

static const FlagsTypeHandler::FlagInfo kMutexFlagsInfo[] = {
	FLAG_INFO_ENTRY(B_USER_MUTEX_LOCKED),
	FLAG_INFO_ENTRY(B_USER_MUTEX_WAITING),
	FLAG_INFO_ENTRY(B_USER_MUTEX_DISABLED),

	{ 0, NULL }
};

static const FlagsTypeHandler::FlagInfo kMutexOptionFlagsInfo[] = {
	FLAG_INFO_ENTRY(B_RELATIVE_TIMEOUT),
	FLAG_INFO_ENTRY(B_ABSOLUTE_TIMEOUT),
	FLAG_INFO_ENTRY(B_TIMEOUT_REAL_TIME_BASE),

	FLAG_INFO_ENTRY(B_USER_MUTEX_SHARED),
	FLAG_INFO_ENTRY(B_USER_MUTEX_UNBLOCK_ALL),

	{ 0, NULL }
};

static FlagsTypeHandler::FlagsList kMutexFlags;
static FlagsTypeHandler::FlagsList kMutexOptionFlags;


class MutexTypeHandler : public FlagsTypeHandler {
public:
	MutexTypeHandler()
		:
		FlagsTypeHandler(kMutexFlags)
	{
	}

	string GetParameterValue(Context &context, Parameter *param,
		const void *address)
	{
		void *data = *(void **)address;

		if (context.GetContents(Context::POINTER_VALUES)) {
			int32 value, bytesRead;
			status_t err = context.Reader().Read(data, &value, sizeof(value), bytesRead);
			if (err != B_OK)
				return context.FormatPointer(data);

			string r = context.FormatPointer(data);
			r += " [";
			r += RenderValue(context, (unsigned int)value);
			r += "]";
			return r;
		}

		return context.FormatPointer(data);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return context.FormatPointer((void *)value);
	}
};


void
patch_mutex()
{
	for (int i = 0; kMutexFlagsInfo[i].name != NULL; i++)
		kMutexFlags.push_back(kMutexFlagsInfo[i]);
	for (int i = 0; kMutexOptionFlagsInfo[i].name != NULL; i++)
        kMutexOptionFlags.push_back(kMutexOptionFlagsInfo[i]);

	Syscall *mutex_lock = get_syscall("_kern_mutex_lock");
	mutex_lock->GetParameter("mutex")->SetHandler(new MutexTypeHandler());
	mutex_lock->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kMutexOptionFlags));

	Syscall *mutex_unlock = get_syscall("_kern_mutex_unblock");
	mutex_unlock->GetParameter("mutex")->SetHandler(new MutexTypeHandler());
	mutex_unlock->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kMutexOptionFlags));

	Syscall *mutex_switch_lock = get_syscall("_kern_mutex_switch_lock");
	mutex_switch_lock->GetParameter("fromMutex")->SetHandler(new MutexTypeHandler());
	mutex_switch_lock->GetParameter("fromFlags")->SetHandler(
		new FlagsTypeHandler(kMutexOptionFlags));
	mutex_switch_lock->GetParameter("toMutex")->SetHandler(new MutexTypeHandler());
	mutex_switch_lock->GetParameter("toFlags")->SetHandler(
		new FlagsTypeHandler(kMutexOptionFlags));

	Syscall *mutex_sem_acquire = get_syscall("_kern_mutex_sem_acquire");
	mutex_sem_acquire->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kMutexOptionFlags));

	Syscall *mutex_sem_release = get_syscall("_kern_mutex_sem_release");
	mutex_sem_release->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kMutexOptionFlags));
}
