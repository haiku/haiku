/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "UiUtils.h"

#include <stdio.h>

#include <DateTime.h>
#include <Path.h>
#include <String.h>
#include <Variant.h>

#include "FunctionInstance.h"
#include "Image.h"
#include "StackFrame.h"
#include "Team.h"
#include "Thread.h"
#include "Type.h"
#include "Value.h"
#include "ValueNode.h"


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


/*static*/ void
UiUtils::PrintValueNodeGraph(BString& _output, StackFrame* frame,
	ValueNodeChild* child, int32 indentLevel, int32 maxDepth)
{
	_output.Append('\t', indentLevel);
	_output << child->Name();

	ValueNode* node = child->Node();
	if (node == NULL) {
		_output << ": Unavailable\n";
		return;
	}

	if (node->GetType()->Kind() != TYPE_COMPOUND) {
		_output << ": ";
		status_t resolutionState = node->LocationAndValueResolutionState();
		if (resolutionState == VALUE_NODE_UNRESOLVED)
			_output << "Unresolved";
		else if (resolutionState == B_OK) {
			Value* value = node->GetValue();
			if (value != NULL) {
				BString valueData;
				value->ToString(valueData);
				_output << valueData;
			} else
				_output << "Unavailable";
		} else
			_output << strerror(resolutionState);
	}

	if (maxDepth == 0 || node->CountChildren() == 0) {
		_output << "\n";
		return;
	}

	if (node->CountChildren() == 1
		&& node->GetType()->Kind() == TYPE_ADDRESS
		&& node->ChildAt(0)->GetType()->Kind() == TYPE_COMPOUND) {
		// for the case of a pointer to a compound type,
		// we want to hide the intervening compound node and print
		// the children directly.
		node = node->ChildAt(0)->Node();
	}

	if (node != NULL) {
		_output << " {\n";

		for (int32 i = 0; i < node->CountChildren(); i++) {
			// don't dump compound nodes if our depth limit won't allow
			// us to traverse into their children anyways, and the top
			// level node contains no data of intereest.
			if (node->ChildAt(i)->GetType()->Kind() != TYPE_COMPOUND
				|| maxDepth > 1) {
				PrintValueNodeGraph(_output, frame, node->ChildAt(i),
					indentLevel + 1, maxDepth - 1);
			}
		}
		_output.Append('\t', indentLevel);
		_output << "}\n";
	}

	return;
}
