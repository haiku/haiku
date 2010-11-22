/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TITLE_PLACEHOLDER_MAPPER_H
#define TITLE_PLACEHOLDER_MAPPER_H


#include "ActiveProcessInfo.h"
#include "PatternEvaluator.h"
#include "ShellInfo.h"


/*! Class mapping the placeholders common for window and tab titles.
 */
class TitlePlaceholderMapper : public PatternEvaluator::PlaceholderMapper {
public:
								TitlePlaceholderMapper(
									const ShellInfo& shellInfo,
									const ActiveProcessInfo& processInfo);

	virtual	bool				MapPlaceholder(char placeholder,
									int64 number, bool numberGiven,
									BString& _string);

private:
			ShellInfo			fShellInfo;
			ActiveProcessInfo	fProcessInfo;
};


class WindowTitlePlaceholderMapper : public TitlePlaceholderMapper {
public:
								WindowTitlePlaceholderMapper(
									const ShellInfo& shellInfo,
									const ActiveProcessInfo& processInfo,
									int32 windowIndex, const BString& tabTitle);

	virtual	bool				MapPlaceholder(char placeholder,
									int64 number, bool numberGiven,
									BString& _string);

private:
			int32				fWindowIndex;
			BString				fTabTitle;
};


class TabTitlePlaceholderMapper : public TitlePlaceholderMapper {
public:
								TabTitlePlaceholderMapper(
									const ShellInfo& shellInfo,
									const ActiveProcessInfo& processInfo,
									int32 tabIndex);

	virtual	bool				MapPlaceholder(char placeholder,
									int64 number, bool numberGiven,
									BString& _string);

private:
			int32				fTabIndex;
};


#endif	// TITLE_PLACEHOLDER_MAPPER_H
