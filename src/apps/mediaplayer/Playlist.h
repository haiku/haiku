/*
 * Playlist.h - Media Player for the Haiku Operating System
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
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <Entry.h>
#include <List.h>

class Playlist
{
public:
					Playlist();
					~Playlist();
					
	void			MakeEmpty();
	int				CountItems();
	
	void			Sort();
	
	void			AddRef(const entry_ref &ref);

	status_t		NextRef(entry_ref *ref);
	status_t		PrevRef(entry_ref *ref);

private:
	static int		playlist_cmp(const void *p1, const void *p2);

private:
	BList			fList;
	int				fCurrentIndex;
};

#endif
