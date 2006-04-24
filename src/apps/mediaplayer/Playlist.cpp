/*
 * Playlist.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
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

// TODO: using BList for objects is bad, replace it with a template

Playlist::Playlist()
 :	fList()
 ,	fCurrentIndex(-1)
{
}


Playlist::~Playlist()
{
	MakeEmpty();
}


void
Playlist::MakeEmpty()
{
	int count = fList.CountItems();
	for (int i = count - 1; i >= 0; i--) {
		entry_ref *ref = (entry_ref *)fList.RemoveItem(i);
		delete ref;
	}
	fCurrentIndex = -1;
}


int
Playlist::CountItems()
{
	return fList.CountItems();
}


int 
Playlist::playlist_cmp(const void *p1, const void *p2)
{
	return strcmp((*(const entry_ref **)p1)->name, (*(const entry_ref **)p2)->name);
}


void
Playlist::Sort()
{
	fList.SortItems(playlist_cmp);
}


void
Playlist::AddRef(const entry_ref &ref)
{
	fList.AddItem(new entry_ref(ref));
}


status_t
Playlist::NextRef(entry_ref *ref)
{
	int count = fList.CountItems();
	if (fCurrentIndex + 2 > count)
		return B_ERROR;
	*ref = *(entry_ref *)fList.ItemAt(++fCurrentIndex);
	return B_OK;
}


status_t
Playlist::PrevRef(entry_ref *ref)
{
	int count = fList.CountItems();
	if (count == 0 || fCurrentIndex <= 0)
		return B_ERROR;
	*ref = *(entry_ref *)fList.ItemAt(--fCurrentIndex);
	return B_OK;
}

