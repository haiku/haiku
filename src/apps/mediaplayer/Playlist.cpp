/*
 * Playlist.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "Playlist.h"

#include <debugger.h>
#include <new.h>
#include <stdio.h>
#include <string.h>

#include <Path.h>

using std::nothrow;

// TODO: using BList for objects is bad, replace it with a template

Playlist::Listener::Listener() {}
Playlist::Listener::~Listener() {}
void Playlist::Listener::RefAdded(const entry_ref& ref, int32 index) {}
void Playlist::Listener::RefRemoved(int32 index) {}
void Playlist::Listener::RefsSorted() {}
void Playlist::Listener::CurrentRefChanged(int32 newIndex) {}


// #pragma mark -


Playlist::Playlist()
 :	fRefs()
 ,	fCurrentIndex(-1)
{
}


Playlist::~Playlist()
{
	MakeEmpty();

	if (fListeners.CountItems() > 0)
		debugger("Playlist::~Playlist() - there are still listeners attached!");
}


void
Playlist::MakeEmpty()
{
	int32 count = fRefs.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		entry_ref* ref = (entry_ref*)fRefs.RemoveItem(i);
		_NotifyRefRemoved(i);
		delete ref;
	}
	SetCurrentRefIndex(-1);
}


int32
Playlist::CountItems() const
{
	return fRefs.CountItems();
}


void
Playlist::Sort()
{
	fRefs.SortItems(playlist_cmp);
	_NotifyRefsSorted();
}


bool
Playlist::AddRef(const entry_ref &ref)
{
	return AddRef(ref, CountItems());
}


bool
Playlist::AddRef(const entry_ref &ref, int32 index)
{
	entry_ref* copy = new (nothrow) entry_ref(ref);
	if (!copy)
		return false;
	if (!fRefs.AddItem(copy, index)) {
		delete copy;
		return false;
	}
	_NotifyRefAdded(ref, index);
	return true;
}


bool
Playlist::AdoptPlaylist(Playlist& other)
{
	return AdoptPlaylist(other, CountItems());
}


bool
Playlist::AdoptPlaylist(Playlist& other, int32 index)
{
	if (&other == this)
		return false;
	// NOTE: this is not intended to merge two "equal" playlists
	// the given playlist is assumed to be a temporary "dummy"
	if (fRefs.AddList(&other.fRefs, index)) {
		// take care of the notifications
		int32 count = other.fRefs.CountItems();
		for (int32 i = index; i < index + count; i++) {
			entry_ref* ref = (entry_ref*)fRefs.ItemAtFast(i);
			_NotifyRefAdded(*ref, i);
		}
		// empty the other list, so that the entry_refs are no ours
		other.fRefs.MakeEmpty();
		return true;
	}
	return false;
}


entry_ref
Playlist::RemoveRef(int32 index)
{
	entry_ref _ref;
	entry_ref* ref = (entry_ref*)fRefs.RemoveItem(index);
	if (!ref)
		return _ref;
	_NotifyRefRemoved(index);
	_ref = *ref;
	delete ref;
	return _ref;
}


//int32
//Playlist::IndexOf(const entry_ref& _ref) const
//{
//	int32 count = CountItems();
//	for (int32 i = 0; i < count; i++) {
//		entry_ref* ref = (entry_ref*)fRefs.ItemAtFast(i);
//		if (*ref == _ref)
//			return i;
//	}
//	return -1;
//}


status_t
Playlist::GetRefAt(int32 index, entry_ref* _ref) const
{
	if (!_ref)
		return B_BAD_VALUE;
	entry_ref* ref = (entry_ref*)fRefs.ItemAt(index);
	if (!ref)
		return B_BAD_INDEX;
	*_ref = *ref;

	return B_OK;
}


//bool
//Playlist::HasRef(const entry_ref& ref) const
//{
//	return IndexOf(ref) >= 0;
//}



// #pragma mark -


void
Playlist::SetCurrentRefIndex(int32 index)
{
	if (index == fCurrentIndex)
		return;

	fCurrentIndex = index;
	_NotifyCurrentRefChanged(fCurrentIndex);
}


int32
Playlist::CurrentRefIndex() const
{
	return fCurrentIndex;
}


void
Playlist::GetSkipInfo(bool* canSkipPrevious, bool* canSkipNext) const
{
	if (canSkipPrevious)
		*canSkipPrevious = fCurrentIndex > 0;
	if (canSkipNext)
		*canSkipNext = fCurrentIndex < CountItems() - 1;
}


// pragma mark -


bool
Playlist::AddListener(Listener* listener)
{
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


void
Playlist::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}


// #pragma mark -


int
Playlist::playlist_cmp(const void *p1, const void *p2)
{
	// compare complete path

	BEntry a(*(const entry_ref **)p1, false);
	BEntry b(*(const entry_ref **)p2, false);

	BPath aPath(&a);
	BPath bPath(&b);

	return strcmp(aPath.Path(), bPath.Path());
}


void
Playlist::_NotifyRefAdded(const entry_ref& ref, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->RefAdded(ref, index);
	}
}


void
Playlist::_NotifyRefRemoved(int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->RefRemoved(index);
	}
}


void
Playlist::_NotifyRefsSorted() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->RefsSorted();
	}
}


void
Playlist::_NotifyCurrentRefChanged(int32 newIndex) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->CurrentRefChanged(newIndex);
	}
}

