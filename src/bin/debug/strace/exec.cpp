/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <AutoDeleter.h>

#include "strace.h"
#include "Context.h"
#include "MemoryReader.h"
#include "TypeHandler.h"


using BPrivate::AutoDeleter;


class FlatArgsTypeHandler : public TypeHandler {
public:
	string GetParameterValue(Context &context, Parameter *param,
		const void *address)
	{
		size_t flatArgsSize;
		int32 argCount, envCount;
		char *flatArgs, *flatArgsEnd;
		int32 bytesRead;
		status_t err;
		ArrayDeleter<char> flatArgsDeleter;
		string r;

		if (!context.GetContents(Context::COMPLEX_STRUCTS))
			goto fallback;

		param = context.GetNextSibling(param);
		if (param == NULL)
			goto fallback;
		flatArgsSize = context.ReadValue<size_t>(param);

		param = context.GetNextSibling(param);
		if (param == NULL)
			goto fallback;
		argCount = context.ReadValue<int32>(param);

		param = context.GetNextSibling(param);
		if (param == NULL)
			goto fallback;
		envCount = context.ReadValue<int32>(param);

		flatArgs = new (std::nothrow) char[flatArgsSize + 1];
		if (flatArgs == NULL)
			goto fallback;

		flatArgsDeleter.SetTo(flatArgs);

		// Guard with a null byte to prevent faulty buffers.
		flatArgsEnd = flatArgs + flatArgsSize;
		*flatArgsEnd = '\0';

		err = context.Reader().Read(*(void **)address, flatArgs, flatArgsSize,
			bytesRead);
		if (err != B_OK)
			goto fallback;

		flatArgs += sizeof(void *) * (argCount + envCount + 2);

		r = "{args = [";

		for (int32 i = 0; i < argCount && flatArgs < flatArgsEnd; i++) {
			if (i > 0)
				r += ", ";
			size_t currentLen = strlen(flatArgs);
			r += "\"";
			r += flatArgs;
			r += "\"";
			flatArgs += currentLen + 1;
		}

		r += "], env = [";

		for (int32 i = 0; i < envCount && flatArgs < flatArgsEnd; i++) {
			if (i > 0)
				r += ", ";
			size_t currentLen = strlen(flatArgs);
			r += "\"";
			r += flatArgs;
			r += "\"";
			flatArgs += currentLen + 1;
		}

		r += "]}";

		return r;
	fallback:
		return context.FormatPointer(address);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return context.FormatPointer((void *)value);
	}
};


void
patch_exec()
{
	Syscall *exec = get_syscall("_kern_exec");
	exec->GetParameter("flatArgs")->SetHandler(new FlatArgsTypeHandler());

	Syscall *load_image = get_syscall("_kern_load_image");
	load_image->GetParameter("flatArgs")->SetHandler(new FlatArgsTypeHandler());
}
