/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSCALL_INFO_H
#define SYSCALL_INFO_H

#include "Types.h"


class SyscallInfo {
public:
								SyscallInfo();
								SyscallInfo(const SyscallInfo& other);
								SyscallInfo(bigtime_t startTime,
									bigtime_t endTime,
									uint64 returnValue,
									uint32 syscall,
									const uint32* args);

			void				SetTo(bigtime_t startTime,
									bigtime_t endTime,
									uint64 returnValue,
									uint32 syscall,
									const uint32* args);

			bigtime_t			StartTime() const	{ return fStartTime; }
			bigtime_t			EndTime() const	{ return fEndTime; }
			uint64				ReturnValue() const	{ return fReturnValue; }
			uint32				Syscall() const	{ return fSyscall; }

			const uint32*		Arguments() const	{ return fArguments; }

private:
			bigtime_t 			fStartTime;
			bigtime_t 			fEndTime;
			uint64 				fReturnValue;
			uint32 				fSyscall;
			uint32 				fArguments[16];
};


#endif	// SYSCALL_INFO_H
