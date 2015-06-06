/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "StringValueFormatter.h"

#include <stdio.h>

#include <String.h>

#include "Value.h"


StringValueFormatter::StringValueFormatter()
	:
	ValueFormatter()
{
}


StringValueFormatter::~StringValueFormatter()
{
}


status_t
StringValueFormatter::FormatValue(Value* value, BString& _output)
{
	_output = "\"";
	BString tempString;
	if (!value->ToString(tempString))
		return B_BAD_VALUE;

	for (int32 i = 0; i < tempString.Length(); i++) {
		if (tempString[i] < 31) {
			switch (tempString[i]) {
				case '\0':
					_output << "\\0";
					break;
				case '\a':
					_output << "\\a";
					break;
				case '\b':
					_output << "\\b";
					break;
				case '\t':
					_output << "\\t";
					break;
				case '\r':
					_output << "\\r";
					break;
				case '\n':
					_output << "\\n";
					break;
				case '\f':
					_output << "\\f";
					break;
				default:
				{
					char buffer[5];
					snprintf(buffer, sizeof(buffer), "\\x%x",
						tempString.String()[i]);
					_output << buffer;
					break;
				}
			}
		} else if (tempString[i] == '\"')
			_output << "\\\"";
		else
			_output << tempString[i];
	}

	_output += "\"";

	return B_OK;
}
