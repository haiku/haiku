/*
 * Copyright 2023-2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <poll.h>

#include "strace.h"
#include "Syscall.h"
#include "Context.h"
#include "MemoryReader.h"
#include "TypeHandler.h"


// #pragma mark - enums & flags handlers


struct enum_info {
	unsigned int index;
	const char *name;
};

#define ENUM_INFO_ENTRY(name) \
	{ name, #name }


#define FLAG_INFO_ENTRY(name) \
	{ name, #name }


static const FlagsTypeHandler::FlagInfo kPollFlagInfos[] = {
	FLAG_INFO_ENTRY(POLLIN),
	FLAG_INFO_ENTRY(POLLOUT),
	FLAG_INFO_ENTRY(POLLRDBAND),
	FLAG_INFO_ENTRY(POLLWRBAND),
	FLAG_INFO_ENTRY(POLLPRI),

	FLAG_INFO_ENTRY(POLLERR),
	FLAG_INFO_ENTRY(POLLHUP),
	FLAG_INFO_ENTRY(POLLNVAL),

	{ 0, NULL }
};


static FlagsTypeHandler::FlagsList kPollFlags;
static FlagsTypeHandler sPollFlagsHandler(kPollFlags);


// #pragma mark - specialized type handlers


static string
read_fdset(Context &context, void *data)
{
	// default FD_SETSIZE is 1024
	unsigned long tmp[1024 / (sizeof(unsigned long) * 8)];
	int32 bytesRead;

	status_t err = context.Reader().Read(data, &tmp, sizeof(tmp), bytesRead);
	if (err != B_OK)
		return context.FormatPointer(data);

	/* implicitly align to unsigned long lower boundary */
	int count = bytesRead / sizeof(unsigned long);
	int added = 0;

	string r;
	r.reserve(16);

	r = "[";

	for (int i = 0; i < count && added < 8; i++) {
		for (int j = 0;
			 j < (int)(sizeof(unsigned long) * 8) && added < 8; j++) {
			if (tmp[i] & (1UL << j)) {
				if (added > 0)
					r += " ";
				unsigned int fd = i * sizeof(unsigned long) * 8 + j;
				r += context.FormatUnsigned(fd);
				added++;
			}
		}
	}

	if (added >= 8)
		r += " ...";

	r += "]";

	return r;
}


template<>
string
TypeHandlerImpl<fd_set *>::GetParameterValue(Context &context, Parameter *,
	const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_fdset(context, data);
	return context.FormatPointer(data);
}


template<>
string
TypeHandlerImpl<fd_set *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


static string
read_pollfd(Context &context, void *data)
{
	nfds_t numfds = context.ReadValue<nfds_t>(context.GetSibling(1));
	if ((int64)numfds <= 0)
		return string();

	pollfd tmp[numfds];
	int32 bytesRead;

	status_t err = context.Reader().Read(data, &tmp, sizeof(tmp), bytesRead);
	if (err != B_OK)
		return context.FormatPointer(data);

	string r;
	r.reserve(16);

	r = "[";

	int added = 0;
	for (nfds_t i = 0; i < numfds && added < 8; i++) {
		if ((tmp[i].fd == -1 || tmp[i].revents == 0)
				&& context.GetContents(Context::OUTPUT_VALUES)) {
			continue;
		}
		if (added > 0)
			r += ", ";
		r += "{fd=" + context.FormatSigned(tmp[i].fd);
		if (tmp[i].fd != -1 && context.GetContents(Context::INPUT_VALUES)) {
			r += ", events=";
			r += sPollFlagsHandler.RenderValue(context, tmp[i].events);
		}
		if (context.GetContents(Context::OUTPUT_VALUES)) {
			r += ", revents=";
			r += sPollFlagsHandler.RenderValue(context, tmp[i].revents);
		}
		added++;
		r += "}";
	}

	if (added >= 8)
		r += " ...";

	r += "]";
	return r;
}


template<>
string
TypeHandlerImpl<pollfd *>::GetParameterValue(Context &context, Parameter *,
	const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_pollfd(context, data);
	return context.FormatPointer(data);
}


template<>
string
TypeHandlerImpl<pollfd *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


DEFINE_TYPE(fdset_ptr, fd_set *)
DEFINE_TYPE(pollfd_ptr, pollfd *)


// #pragma mark - patch function


void
patch_events()
{
	for (int i = 0; kPollFlagInfos[i].name != NULL; i++)
		kPollFlags.push_back(kPollFlagInfos[i]);

	Syscall *poll = get_syscall("_kern_poll");
	poll->ParameterAt(0)->SetInOut(true);

	Syscall *select = get_syscall("_kern_select");
	select->ParameterAt(1)->SetInOut(true);
	select->ParameterAt(2)->SetInOut(true);
	select->ParameterAt(3)->SetInOut(true);

	Syscall *wait = get_syscall("_kern_wait_for_child");
	wait->ParameterAt(2)->SetOut(true);
	wait->ParameterAt(3)->SetOut(true);
}
