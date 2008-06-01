/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_OUTPUT_FILTER_H
#define _KERNEL_DEBUG_OUTPUT_FILTER_H

#include <stdarg.h>

#include <SupportDefs.h>


class DebugOutputFilter {
public:
	DebugOutputFilter();
	virtual ~DebugOutputFilter();

	virtual void PrintString(const char* string);
	virtual void Print(const char* format, va_list args);
};

class DefaultDebugOutputFilter : public DebugOutputFilter {
public:
	virtual void PrintString(const char* string);
	virtual void Print(const char* format, va_list args);
};


extern DefaultDebugOutputFilter gDefaultDebugOutputFilter;


DebugOutputFilter* set_debug_output_filter(DebugOutputFilter* filter);


#endif	// _KERNEL_DEBUG_OUTPUT_FILTER_H
