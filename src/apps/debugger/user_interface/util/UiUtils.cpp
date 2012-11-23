/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "UiUtils.h"

#include <stdio.h>

#include <DateTime.h>
#include <Path.h>
#include <Variant.h>

#include "FunctionInstance.h"
#include "Image.h"
#include "StackFrame.h"
#include "Team.h"
#include "Thread.h"


/*static*/ const char*
UiUtils::ThreadStateToString(int state, int stoppedReason)
{
	switch (state) {
		case THREAD_STATE_RUNNING:
			return "Running";
		case THREAD_STATE_STOPPED:
			break;
		case THREAD_STATE_UNKNOWN:
		default:
			return "?";
	}

	// thread is stopped -- get the reason
	switch (stoppedReason) {
		case THREAD_STOPPED_DEBUGGER_CALL:
			return "Call";
		case THREAD_STOPPED_EXCEPTION:
			return "Exception";
		case THREAD_STOPPED_BREAKPOINT:
		case THREAD_STOPPED_WATCHPOINT:
		case THREAD_STOPPED_SINGLE_STEP:
		case THREAD_STOPPED_DEBUGGED:
		case THREAD_STOPPED_UNKNOWN:
		default:
			return "Debugged";
	}
}


/*static*/ const char*
UiUtils::VariantToString(const BVariant& value, char* buffer,
	size_t bufferSize)
{
	if (!value.IsNumber())
		return value.ToString();

	switch (value.Type()) {
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			snprintf(buffer, bufferSize, "%g", value.ToDouble());
			break;
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			snprintf(buffer, bufferSize, "0x%02x", value.ToUInt8());
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			snprintf(buffer, bufferSize, "0x%04x", value.ToUInt16());
			break;
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			snprintf(buffer, bufferSize, "0x%08" B_PRIx32,
				value.ToUInt32());
			break;
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		default:
			snprintf(buffer, bufferSize, "0x%016" B_PRIx64,
				value.ToUInt64());
			break;
	}

	return buffer;
}


/*static*/ const char*
UiUtils::FunctionNameForFrame(StackFrame* frame, char* buffer,
	size_t bufferSize)
{
	Image* image = frame->GetImage();
	FunctionInstance* function = frame->Function();
	if (image == NULL && function == NULL) {
		snprintf(buffer, bufferSize, "?");
		return buffer;
	}

	BString name;
	target_addr_t baseAddress;
	if (function != NULL) {
		name = function->PrettyName();
		baseAddress = function->Address();
	} else {
		name = image->Name();
		baseAddress = image->Info().TextBase();
	}

	snprintf(buffer, bufferSize, "%s + %#" B_PRIx64,
		name.String(), frame->InstructionPointer() - baseAddress);

	return buffer;
}


/*static*/ const char*
UiUtils::ImageTypeToString(image_type type, char* buffer, size_t bufferSize)
{
	switch (type) {
		case B_APP_IMAGE:
			snprintf(buffer, bufferSize, "Application");
			break;
		case B_LIBRARY_IMAGE:
			snprintf(buffer, bufferSize, "Library");
			break;
		case B_ADD_ON_IMAGE:
			snprintf(buffer, bufferSize, "Add-on");
			break;
		case B_SYSTEM_IMAGE:
			snprintf(buffer, bufferSize, "System");
			break;
		default:
			snprintf(buffer, bufferSize, "Unknown");
			break;
	}

	return buffer;
}


/*static*/ const char*
UiUtils::ReportNameForTeam(::Team* team, char* buffer, size_t bufferSize)
{
	BPath teamPath(team->Name());
	BDateTime currentTime;
	currentTime.SetTime_t(time(NULL));
	snprintf(buffer, bufferSize, "%s-%" B_PRId32 "-debug-%02" B_PRId32 "-%02"
		B_PRId32 "-%02" B_PRId32 "-%02" B_PRId32 "-%02" B_PRId32 "-%02"
		B_PRId32 ".report", teamPath.Leaf(), team->ID(),
		currentTime.Date().Day(), currentTime.Date().Month(),
		currentTime.Date().Year(), currentTime.Time().Hour(),
		currentTime.Time().Minute(), currentTime.Time().Second());

	return buffer;

}
