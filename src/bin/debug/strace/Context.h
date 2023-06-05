/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Hugo Santos <hugosantos@gmail.com>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef STRACE_CONTEXT_H
#define STRACE_CONTEXT_H

#include "Syscall.h"

class Context {
public:
	enum {
		STRINGS		= 1 << 0,
		ENUMERATIONS	= 1 << 1,
		SIMPLE_STRUCTS	= 1 << 2,
		COMPLEX_STRUCTS	= 1 << 3,
		POINTER_VALUES	= 1 << 4,
		INPUT_VALUES	= 1 << 5,
		OUTPUT_VALUES	= 1 << 6,
		ALL		= 0xffffff9f
	};

	Context(Syscall *sc, char *data, MemoryReader &reader,
		uint32 flags, bool decimal, uint64 returnValue = 0)
		: fSyscall(sc), fData(data), fReader(reader),
		  fFlags(flags), fDecimal(decimal), fReturnValue(returnValue) {}

	Parameter *GetSibling(int32 index) const {
		return fSyscall->ParameterAt(index);
	}

	Parameter *GetNextSibling(Parameter *param) const;

	const void *GetValue(Parameter *param) const {
		return fData + param->Offset();
	}

	template<typename value_t>
	value_t ReadValue(Parameter *param) const {
		const void *address = GetValue(param);
		if (sizeof(align_t) > sizeof(value_t))
			return value_t(*(align_t*)address);
		else
			return *(value_t*)address;
	}

	uint64 GetReturnValue() const {
		return fReturnValue;
	}

	MemoryReader &Reader() { return fReader; }
	bool GetContents(uint32 what) const { return fFlags & what; }

	string FormatSigned(int64 value, int bytes = 8) const;
	string FormatUnsigned(uint64 value) const;
	string FormatFlags(uint64 value) const;

	string FormatPointer(const void *address) const;

private:
	Syscall *fSyscall;
	char *fData;
	MemoryReader &fReader;
	uint32 fFlags;
	bool fDecimal;
	uint64 fReturnValue;
};

#endif	// STRACE_CONTEXT_H
