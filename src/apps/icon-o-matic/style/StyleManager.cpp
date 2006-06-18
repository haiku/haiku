/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StyleManager.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "Style.h"

// constructor
StyleManagerListener::StyleManagerListener()
{
}

// destructor
StyleManagerListener::~StyleManagerListener()
{
}


// init global StyleManager instance
StyleManager
StyleManager::fDefaultInstance;


// constructor
StyleManager::StyleManager()
	: RWLocker(),
	  fStyles(32),
	  fListeners(2)
{
}

// destructor
StyleManager::~StyleManager()
{
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~StyleManager() - there are still"
				 "listeners attached\n");
	}
	_MakeEmpty();
}

// #pragma mark -

// AddStyle
bool
StyleManager::AddStyle(Style* style)
{
	if (!style)
		return false;

	// prevent adding the same style twice
	if (HasStyle(style))
		return false;

	if (fStyles.AddItem((void*)style)) {
		_NotifyStyleAdded(style);
		_StateChanged();
		return true;
	}

	fprintf(stderr, "StyleManager::AddStyle() - out of memory!\n");
	return false;
}

// RemoveStyle
bool
StyleManager::RemoveStyle(Style* style)
{
	if (fStyles.RemoveItem((void*)style)) {
		_NotifyStyleRemoved(style);
		_StateChanged();
		return true;
	}

	return false;
}

// RemoveStyle
Style*
StyleManager::RemoveStyle(int32 index)
{
	Style* style = (Style*)fStyles.RemoveItem(index);
	if (style) {
		_NotifyStyleRemoved(style);
		_StateChanged();
	}

	return style;
}

// MakeEmpty
void
StyleManager::MakeEmpty()
{
	_MakeEmpty();
	_StateChanged();
}

// #pragma mark -

// CountStyles
int32
StyleManager::CountStyles() const
{
	return fStyles.CountItems();
}

// HasStyle
bool
StyleManager::HasStyle(Style* style) const
{
	return fStyles.HasItem((void*)style);
}

// StyleAt
Style*
StyleManager::StyleAt(int32 index) const
{
	return (Style*)fStyles.ItemAt(index);
}

// StyleAtFast
Style*
StyleManager::StyleAtFast(int32 index) const
{
	return (Style*)fStyles.ItemAtFast(index);
}

// StateChanged
void
StyleManager::StateChanged()
{
	// empty
}

// #pragma mark -

// AddListener
bool
StyleManager::AddListener(StyleManagerListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem(listener);
	return false;
}

// RemoveListener
bool
StyleManager::RemoveListener(StyleManagerListener* listener)
{
	return fListeners.RemoveItem(listener);
}

StyleManager*
StyleManager::Default()
{
	return &fDefaultInstance;
}

// #pragma mark -

// _StateChanged
void
StyleManager::_StateChanged()
{
	StateChanged();
}

// _MakeEmpty
void
StyleManager::_MakeEmpty()
{
	int32 count = CountStyles();
	for (int32 i = 0; i < count; i++) {
		Style* style = StyleAtFast(i);
		_NotifyStyleRemoved(style);
		style->Release();
	}
	fStyles.MakeEmpty();
}

// #pragma mark -

// _NotifyStyleAdded
void
StyleManager::_NotifyStyleAdded(Style* style) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleManagerListener* listener
			= (StyleManagerListener*)listeners.ItemAtFast(i);
		listener->StyleAdded(style);
	}
}

// _NotifyStyleRemoved
void
StyleManager::_NotifyStyleRemoved(Style* style) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleManagerListener* listener
			= (StyleManagerListener*)listeners.ItemAtFast(i);
		listener->StyleRemoved(style);
	}
}

