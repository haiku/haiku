/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <size_t.h>

#include <image.h>


class BVariant;
class StackFrame;
class Team;


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
};


#endif	// UI_UTILS_H
