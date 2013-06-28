/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "SyscallInfo.h"

#include <string.h>


SyscallInfo::SyscallInfo()
	:
	fStartTime(0),
	fEndTime(0),
	fReturnValue(0),
	fSyscall(0)
{
	memset(fArguments, 0, sizeof(fArguments));
}


SyscallInfo::SyscallInfo(const SyscallInfo& other)
	:
	fStartTime(other.fStartTime),
	fEndTime(other.fEndTime),
	fReturnValue(other.fReturnValue),
	fSyscall(other.fSyscall)
{
	memcpy(fArguments, other.fArguments, sizeof(fArguments));
}


SyscallInfo::SyscallInfo(bigtime_t startTime, bigtime_t endTime,
	uint64 returnValue, uint32 syscall, const uint8* args)
	:
	fStartTime(startTime),
	fEndTime(endTime),
	fReturnValue(returnValue),
	fSyscall(syscall)
{
	memcpy(fArguments, args, sizeof(fArguments));
}


void
SyscallInfo::SetTo(bigtime_t startTime, bigtime_t endTime, uint64 returnValue,
	uint32 syscall, const uint8* args)
{
	fStartTime = startTime;
	fEndTime = endTime;
	fReturnValue = returnValue;
	fSyscall = syscall;
	memcpy(fArguments, args, sizeof(fArguments));
}
