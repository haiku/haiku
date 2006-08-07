/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StyleContainer.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "Style.h"

#ifdef ICON_O_MATIC
StyleContainerListener::StyleContainerListener() {}
StyleContainerListener::~StyleContainerListener() {}
#endif

// constructor
StyleContainer::StyleContainer()
#ifdef ICON_O_MATIC
	: fStyles(32),
	  fListeners(2)
#else
	: fStyles(32)
#endif
{
}

// destructor
StyleContainer::~StyleContainer()
{
#ifdef ICON_O_MATIC
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~StyleContainer() - there are still"
				 "listeners attached\n");
	}
#endif // ICON_O_MATIC
	_MakeEmpty();
}

// #pragma mark -

// AddStyle
bool
StyleContainer::AddStyle(Style* style)
{
	return AddStyle(style, CountStyles());
}

// AddStyle
bool
StyleContainer::AddStyle(Style* style, int32 index)
{
	if (!style)
		return false;

	// prevent adding the same style twice
	if (HasStyle(style))
		return false;

	if (fStyles.AddItem((void*)style, index)) {
#ifdef ICON_O_MATIC
		_NotifyStyleAdded(style, index);
#endif
		return true;
	}

	fprintf(stderr, "StyleContainer::AddStyle() - out of memory!\n");
	return false;
}

// RemoveStyle
bool
StyleContainer::RemoveStyle(Style* style)
{
	if (fStyles.RemoveItem((void*)style)) {
#ifdef ICON_O_MATIC
		_NotifyStyleRemoved(style);
#endif
		return true;
	}

	return false;
}

// RemoveStyle
Style*
StyleContainer::RemoveStyle(int32 index)
{
	Style* style = (Style*)fStyles.RemoveItem(index);
	if (style) {
#ifdef ICON_O_MATIC
		_NotifyStyleRemoved(style);
#endif
	}

	return style;
}

// MakeEmpty
void
StyleContainer::MakeEmpty()
{
	_MakeEmpty();
}

// #pragma mark -

// CountStyles
int32
StyleContainer::CountStyles() const
{
	return fStyles.CountItems();
}

// HasStyle
bool
StyleContainer::HasStyle(Style* style) const
{
	return fStyles.HasItem((void*)style);
}

// IndexOf
int32
StyleContainer::IndexOf(Style* style) const
{
	return fStyles.IndexOf((void*)style);
}

// StyleAt
Style*
StyleContainer::StyleAt(int32 index) const
{
	return (Style*)fStyles.ItemAt(index);
}

// StyleAtFast
Style*
StyleContainer::StyleAtFast(int32 index) const
{
	return (Style*)fStyles.ItemAtFast(index);
}

// #pragma mark -

#ifdef ICON_O_MATIC

// AddListener
bool
StyleContainer::AddListener(StyleContainerListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem(listener);
	return false;
}

// RemoveListener
bool
StyleContainer::RemoveListener(StyleContainerListener* listener)
{
	return fListeners.RemoveItem(listener);
}

#endif // ICON_O_MATIC

// #pragma mark -

// _MakeEmpty
void
StyleContainer::_MakeEmpty()
{
	int32 count = CountStyles();
	for (int32 i = 0; i < count; i++) {
		Style* style = StyleAtFast(i);
#ifdef ICON_O_MATIC
		_NotifyStyleRemoved(style);
		style->Release();
#else
		delete style;
#endif
	}
	fStyles.MakeEmpty();
}

// #pragma mark -

#ifdef ICON_O_MATIC

// _NotifyStyleAdded
void
StyleContainer::_NotifyStyleAdded(Style* style, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleContainerListener* listener
			= (StyleContainerListener*)listeners.ItemAtFast(i);
		listener->StyleAdded(style, index);
	}
}

// _NotifyStyleRemoved
void
StyleContainer::_NotifyStyleRemoved(Style* style) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		StyleContainerListener* listener
			= (StyleContainerListener*)listeners.ItemAtFast(i);
		listener->StyleRemoved(style);
	}
}

#endif // ICON_O_MATIC

