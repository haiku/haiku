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

static FlagsTypeHandler::FlagsList kMutexFlags;


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
    for (int i = 0; kMutexFlagsInfo[i].name != NULL; i++) {
        kMutexFlags.push_back(kMutexFlagsInfo[i]);
    }

    Syscall *mutex_lock = get_syscall("_kern_mutex_lock");
    mutex_lock->GetParameter("mutex")->SetHandler(new MutexTypeHandler());

    Syscall *mutex_unlock = get_syscall("_kern_mutex_unblock");
    mutex_unlock->GetParameter("mutex")->SetHandler(new MutexTypeHandler());

    Syscall *mutex_switch_lock = get_syscall("_kern_mutex_switch_lock");
    mutex_switch_lock->GetParameter("fromMutex")->SetHandler(new MutexTypeHandler());
    mutex_switch_lock->GetParameter("toMutex")->SetHandler(new MutexTypeHandler());
}
