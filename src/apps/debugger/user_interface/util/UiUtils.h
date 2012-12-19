/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <size_t.h>

#include <image.h>

#include "Types.h"


class BString;
class BVariant;
class StackFrame;
class Team;
class TeamMemoryBlock;
class ValueNodeChild;


class UiUtils {
public:
	static	const char*			ThreadStateToString(int state,
									int stoppedReason);

	static	const char*			VariantToString(const BVariant& value,
									char* buffer, size_t bufferSize);
	static	const char*			FunctionNameForFrame(StackFrame* frame,
									char* buffer, size_t bufferSize);
	static	const char*			ImageTypeToString(image_type type,
									char* buffer, size_t bufferSize);

	static	const char*			ReportNameForTeam(::Team* team,
									char* buffer, size_t bufferSize);

	// this function assumes the value nodes have already been resolved
	// (if possible).
	static	void				PrintValueNodeGraph(BString& _output,
									ValueNodeChild* child,
									int32 indentLevel, int32 maxDepth);

	static	void				DumpMemory(BString& _output,
									int32 indentLevel,
									TeamMemoryBlock* block,
									target_addr_t address, int32 itemSize,
									int32 displayWidth, int32 count);
};


#endif	// UI_UTILS_H
