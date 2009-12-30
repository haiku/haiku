/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_OUTPUT_H
#define TEST_OUTPUT_H


#include <stdarg.h>

#include <KernelExport.h>


class TestOutput {
public:
								TestOutput();
	virtual						~TestOutput();

	virtual	int					PrintArgs(const char* format, va_list args) = 0;
	inline	int					Print(const char* format,...);
};


class DebugTestOutput : public TestOutput {
public:
								DebugTestOutput();
	virtual						~DebugTestOutput();

	virtual	int					PrintArgs(const char* format, va_list args);

private:
			spinlock			fLock;
			char				fBuffer[1024];
};


int
TestOutput::Print(const char* format,...)
{
	va_list args;
	va_start(args, format);
	int result = PrintArgs(format, args);
	va_end(args);

	return result;
}



#endif	// TEST_OUTPUT_H
