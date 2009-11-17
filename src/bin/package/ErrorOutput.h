/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ERROR_OUTPUT_H
#define ERROR_OUTPUT_H


#include <stdarg.h>

#include <SupportDefs.h>


class ErrorOutput {
public:
	virtual						~ErrorOutput();

	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args) = 0;
			void				PrintError(const char* format, ...);
};


#endif	// ERROR_OUTPUT_H
