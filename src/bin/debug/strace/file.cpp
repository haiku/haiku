/*
 * Copyright 2025, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/stat.h>

#include <NodeMonitor.h>

#include "Context.h"
#include "MemoryReader.h"
#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"
#include "util.h"


static string
format_mode(Context &context, mode_t mode)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
#define MODE_TYPE(mode) \
		case mode: \
			return #mode

		switch (mode) {
			MODE_TYPE(S_IFSOCK);
			MODE_TYPE(S_IFLNK);
			MODE_TYPE(S_IFREG);
			MODE_TYPE(S_IFBLK);
			MODE_TYPE(S_IFDIR);
			MODE_TYPE(S_IFIFO);
			MODE_TYPE(S_IFCHR);
		}
	}

	return context.FormatSigned(mode);
}


static string
read_stat(Context &context, Parameter *param, void *data)
{
	int statMask = 0xffffffff;
	if (param->Out()) {
		if ((status_t)context.GetReturnValue() < 0)
			return "";
	} else {
		Parameter* nextParam = context.GetNextSibling(param);
		if (nextParam != NULL) {
			nextParam = context.GetNextSibling(nextParam);
			if (nextParam != NULL && nextParam->Name() == "statMask")
				statMask = context.ReadValue<int>(nextParam);
		}
	}

	struct stat s;
	int32 bytesRead;

	status_t err = context.Reader().Read(data, &s, sizeof(s), bytesRead);
	if (err != B_OK)
		return context.FormatPointer(data);

	string r;
	if ((statMask & 0xffffffff) == 0xffffffff) {
		char mode[12];
		r += ", st_dev = " + format_unsigned(s.st_dev);
		r += ", st_ino = " + format_unsigned(s.st_ino);
		snprintf(mode, sizeof(mode), "%03" B_PRIo32,
			(uint32)(s.st_mode & ~(S_IFMT | S_ISUID | S_ISGID | S_ISVTX)));
		r += ", st_mode = " + format_mode(context, s.st_mode & S_IFMT) + "|";
		r += mode;
		r += ", st_nlink = " + format_unsigned(s.st_nlink);
	}
	if ((statMask & B_STAT_UID) != 0)
		r += ", st_uid = " + format_unsigned(s.st_uid);
	if ((statMask & B_STAT_GID) != 0)
		r += ", st_gid = " + format_unsigned(s.st_gid);
	if ((statMask & B_STAT_SIZE) != 0)
		r += ", st_size = " + format_unsigned64(s.st_size);
	if ((statMask & 0xffffffff) == 0xffffffff)
		r += ", st_blksize = " + format_unsigned(s.st_blksize);
	if ((statMask & B_STAT_ACCESS_TIME) != 0)
		r += ", st_atim = " + format_timespec(context, s.st_atim);
	if ((statMask & B_STAT_MODIFICATION_TIME) != 0)
		r += ", st_mtim = " + format_timespec(context, s.st_mtim);
	if ((statMask & B_STAT_CHANGE_TIME) != 0)
		r += ", st_ctim = " + format_timespec(context, s.st_ctim);
	if ((statMask & B_STAT_CREATION_TIME) != 0)
		r += ", st_crtim = " + format_timespec(context, s.st_crtim);
	if ((statMask & 0xffffffff) == 0xffffffff) {
		r += ", st_type = " + format_unsigned(s.st_type);
		r += ", st_blocks = " + format_unsigned(s.st_blocks);
	}
	return "{" + r.substr(2) + "}";
}


template<>
string
TypeHandlerImpl<struct stat *>::GetParameterValue(Context &context, Parameter *param,
	const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_stat(context, param, data);
	return context.FormatPointer(data);
}


template<>
string
TypeHandlerImpl<struct stat *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


DEFINE_TYPE(stat_ptr, struct stat*);


void
patch_file()
{
	Syscall *readStat = get_syscall("_kern_read_stat");
	readStat->GetParameter("stat")->SetOut(true);
	Syscall *readIndexStat = get_syscall("_kern_read_index_stat");
	readIndexStat->GetParameter("stat")->SetOut(true);
}
