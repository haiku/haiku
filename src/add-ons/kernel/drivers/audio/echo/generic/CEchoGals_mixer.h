// ****************************************************************************
//
//		CEchoGals_mixer.h
//
//		Set editor tabs to 3 for your viewing pleasure.
//
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
//
//   Copyright Echo Digital Audio Corporation (c) 1998 - 2004
//   All rights reserved
//   www.echoaudio.com
//   
//   This file is part of Echo Digital Audio's generic driver library.
//   
//   Echo Digital Audio's generic driver library is free software; 
//   you can redistribute it and/or modify it under the terms of 
//   the GNU General Public License as published by the Free Software Foundation.
//   
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
//   MA  02111-1307, USA.
//
// ****************************************************************************

#ifndef _CEchoGals_mixer_h_
#define _CEchoGals_mixer_h_

//===========================================================================
//
// Mixer client stuff
//
//===========================================================================

//
//	Max number of notifies stored per card
//
#define MAX_MIXER_NOTIFIES		2048

//
//	Structure describing a mixer client.  The notifies are stored in a circular
// buffer
// 
typedef struct tECHO_MIXER_CLIENT
{
	NUINT				Cookie;		//	Unique ID for this client

	DWORD				dwCount;
	DWORD				dwHead;
	DWORD				dwTail;
	
	MIXER_NOTIFY	Notifies[MAX_MIXER_NOTIFIES];
	
	struct tECHO_MIXER_CLIENT *pNext;
	
} ECHO_MIXER_CLIENT;

#endif // _CEchoGals_mixer_h_
