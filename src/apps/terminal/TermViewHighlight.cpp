/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TermViewHighlight.h"


TermViewHighlighter::~TermViewHighlighter()
{
}


uint32
TermViewHighlighter::AdjustTextAttributes(uint32 attributes)
{
	return attributes;
}
