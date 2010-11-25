/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLICK_TARGET_H
#define CLICK_TARGET_H


#include <TokenSpace.h>


/*!	\brief Identifies a mouse click target in the app server.

	Used to discriminate between different targets in order to filter
	multi-clicks. A click on a different target resets the click count.
*/
struct ClickTarget {
public:
	enum Type {
		TYPE_INVALID,
		TYPE_WINDOW_CONTENTS,
		TYPE_WINDOW_DECORATOR
	};

public:
	ClickTarget()
		:
		fType(TYPE_INVALID),
		fWindow(B_NULL_TOKEN),
		fWindowElement(0)
	{
	}

	ClickTarget(Type type, int32 window, int32 windowElement)
		:
		fType(type),
		fWindow(window),
		fWindowElement(windowElement)
	{
	}

	bool IsValid() const
	{
		return fType != TYPE_INVALID;
	}

	Type GetType() const
	{
		return fType;
	}

	int32 WindowToken() const
	{
		return fWindow;
	}

	int32 WindowElement() const
	{
		return fWindowElement;
	}

	bool operator==(const ClickTarget& other) const
	{
		return fType == other.fType && fWindow == other.fWindow
			&& fWindowElement == other.fWindowElement;
	}

	bool operator!=(const ClickTarget& other) const
	{
		return !(*this == other);
	}

private:
	Type	fType;
	int32	fWindow;
	int32	fWindowElement;
};


#endif	// CLICK_TARGET_H
