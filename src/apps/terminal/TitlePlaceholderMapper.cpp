/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TitlePlaceholderMapper.h"


// #pragma mark - TitlePlaceholderMapper


TitlePlaceholderMapper::TitlePlaceholderMapper(
	const ActiveProcessInfo& processInfo)
	:
	fProcessInfo(processInfo)
{
}


#include <stdio.h>
bool
TitlePlaceholderMapper::MapPlaceholder(char placeholder, int64 number,
	bool numberGiven, BString& _string)
{
	switch (placeholder) {
		case 'd':
		{
			// current working directory

			// If a number is given, extract the respective number of rightmost
			// components.
			BString directory(fProcessInfo.CurrentDirectory());
			if (numberGiven && number > 0) {
				int32 index = directory.Length();
				while (number > 0 && index > 0) {
					 index = directory.FindLast('/', index - 1);
					 number--;
				}

				if (number == 0 && index >= 0 && index + 1 < directory.Length())
					directory.Remove(0, index + 1);
			}

			_string = directory;
			return true;
		}

		case 'p':
			// process name
			_string = fProcessInfo.Name();
			return true;
	}

	return false;
}


// #pragma mark - WindowTitlePlaceholderMapper


WindowTitlePlaceholderMapper::WindowTitlePlaceholderMapper(
	const ActiveProcessInfo& processInfo, int32 windowIndex,
	const BString& tabTitle)
	:
	TitlePlaceholderMapper(processInfo),
	fWindowIndex(windowIndex),
	fTabTitle(tabTitle)
{
}


bool
WindowTitlePlaceholderMapper::MapPlaceholder(char placeholder, int64 number,
	bool numberGiven, BString& _string)
{
	switch (placeholder) {
		case 'i':
			// window index
			_string.Truncate(0);
			_string << fWindowIndex;
			return true;

		case 't':
			// the tab title
			_string = fTabTitle;
			return true;
	}

	return TitlePlaceholderMapper::MapPlaceholder(placeholder, number,
		numberGiven, _string);
}


// #pragma mark - TabTitlePlaceholderMapper


TabTitlePlaceholderMapper::TabTitlePlaceholderMapper(
	const ActiveProcessInfo& processInfo, int32 tabIndex)
	:
	TitlePlaceholderMapper(processInfo),
	fTabIndex(tabIndex)
{
}


bool
TabTitlePlaceholderMapper::MapPlaceholder(char placeholder, int64 number,
	bool numberGiven, BString& _string)
{
	switch (placeholder) {
		case 'i':
			// tab index
			_string.Truncate(0);
			_string << fTabIndex;
			return true;
	}

	return TitlePlaceholderMapper::MapPlaceholder(placeholder, number,
		numberGiven, _string);
}
