/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT/X11 license.
 */
#ifndef FUNCTION_TRACER_H
#define FUNCTION_TRACER_H


#include <stdio.h>

#include <String.h>


namespace BPrivate {

class FunctionTracer {
public:
	typedef int (*PrintFunction)(const char * fmt, ...);

	FunctionTracer(PrintFunction printFunction, const void* pointer, const char* functionName,
		int32& depth)
		: fFunctionName(functionName),
		  fPrepend(),
		  fFunctionDepth(depth),
		  fPrintFunction(printFunction)
	{
		fFunctionDepth++;
		if (pointer != NULL)
			fPrepend.SetToFormat("%p ->", pointer);
		fPrepend.Append(' ', fFunctionDepth * 2);

		fPrintFunction("%s%s {\n", fPrepend.String(), fFunctionName.String());
	}

	 ~FunctionTracer()
	{
		fPrintFunction("%s}\n", fPrepend.String());
		fFunctionDepth--;
	}

private:
	BString	fFunctionName;
	BString	fPrepend;
	int32&	fFunctionDepth;
#if __GNUC__ < 3
	// gcc2 doesn't support function attributes on function pointers
	PrintFunction fPrintFunction;
#else
	PrintFunction _PRINTFLIKE(1, 2) fPrintFunction;
#endif
};

}	// namespace BPrivate

using BPrivate::FunctionTracer;

#endif // FUNCTION_TRACER_H
