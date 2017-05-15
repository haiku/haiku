/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include "JsonWriter.h"

#include <stdio.h>
#include <stdlib.h>

#include <UnicodeChar.h>


BJsonWriter::BJsonWriter()
	:
	fErrorStatus(B_OK)
{
}


BJsonWriter::~BJsonWriter()
{
}


void
BJsonWriter::HandleError(status_t status, int32 line,
	const char* message)
{
	if(fErrorStatus == B_OK) {
		if (message == NULL)
			message = "?";
		fErrorStatus = status;
		fprintf(stderr, "! json err @line %" B_PRIi32 " - %s : %s\n", line,
			strerror(status), message);
	}
}


status_t
BJsonWriter::ErrorStatus()
{
	return fErrorStatus;
}


status_t
BJsonWriter::WriteBoolean(bool value)
{
	if (value)
		return WriteTrue();

	return WriteFalse();
}


status_t
BJsonWriter::WriteTrue()
{
	Handle(BJsonEvent(B_JSON_TRUE));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteFalse()
{
	Handle(BJsonEvent(B_JSON_FALSE));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteNull()
{
	Handle(BJsonEvent(B_JSON_NULL));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteInteger(int64 value)
{
	Handle(BJsonEvent(value));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteDouble(double value)
{
	Handle(BJsonEvent(value));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteString(const char* value)
{
	Handle(BJsonEvent(value));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteObjectStart()
{
	Handle(BJsonEvent(B_JSON_OBJECT_START));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteObjectName(const char* value)
{
	Handle(BJsonEvent(B_JSON_OBJECT_NAME, value));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteObjectEnd()
{
	Handle(BJsonEvent(B_JSON_OBJECT_END));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteArrayStart()
{
	Handle(BJsonEvent(B_JSON_ARRAY_START));
	return fErrorStatus;
}


status_t
BJsonWriter::WriteArrayEnd()
{
	Handle(BJsonEvent(B_JSON_ARRAY_END));
	return fErrorStatus;
}

