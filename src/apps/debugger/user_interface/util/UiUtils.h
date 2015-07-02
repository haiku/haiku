/*
 * Copyright 2014-2015, Rene Gollent, rene@gollent.com.
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
class RangeList;
class StackFrame;
class Team;
class TeamMemoryBlock;
class ValueNodeChild;


enum {
	SIMD_RENDER_FORMAT_INT8 = 0,
	SIMD_RENDER_FORMAT_INT16,
	SIMD_RENDER_FORMAT_INT32,
	SIMD_RENDER_FORMAT_INT64,
	SIMD_RENDER_FORMAT_FLOAT,
	SIMD_RENDER_FORMAT_DOUBLE
};


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
	static	const char*			AreaLockingFlagsToString(uint32 flags,
									char* buffer, size_t bufferSize);
	static	const BString&			AreaProtectionFlagsToString(uint32 protection,
									BString& _output);

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

	static	status_t			ParseRangeExpression(
									const BString& rangeString,
									int32 lowerBound, int32 upperBound,
									bool fixedRange,
									RangeList& _output);

	static	const char*			TypeCodeToString(type_code type);

	static	const BString&		FormatSIMDValue(const BVariant& value,
									uint32 bitSize, uint32 format,
									BString& _output);

	static	const char*			SignalNameToString(int32 signal,
									BString& _output);
	static	const char*			SignalDispositionToString(int disposition);
};


#endif	// UI_UTILS_H
