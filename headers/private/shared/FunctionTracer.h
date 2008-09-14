/*
 * Copyright © 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT/X11 license.
 */
#ifndef FUNCTION_TRACER_H
#define FUNCTION_TRACER_H

#include <stdio.h>

#include <String.h>

namespace BPrivate {

class FunctionTracer {
public:
	FunctionTracer(const char* className, const char* functionName,
			int32& depth)
		: fFunctionName(),
		  fPrepend(),
		  fFunctionDepth(depth)
	{
		fFunctionDepth++;
		fPrepend.Append(' ', fFunctionDepth * 2);
		fFunctionName << className << "::" << functionName << "()";

		printf("%s%s {\n", fPrepend.String(), fFunctionName.String());
	}

	 ~FunctionTracer()
	{
//		printf("%s - leave\n", fFunctionName.String());
		printf("%s}\n", fPrepend.String());
		fFunctionDepth--;
	}

private:
	BString	fFunctionName;
	BString	fPrepend;
	int32&	fFunctionDepth;
};

}	// namespace BPrivate

using BPrivate::FunctionTracer;

#endif // FUNCTION_TRACER_H
