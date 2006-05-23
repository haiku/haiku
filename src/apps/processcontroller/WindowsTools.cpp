/*
	
	WindowsTools.cpp
	
	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
*/

#include "WindowsTools.h"
#include <Screen.h>
#include <Window.h>

_EXPORT void visible_window (BWindow* window, bool mayResize)
{
	uint32		flags = window->Flags ();
	BRect		screen = BScreen (window).Frame ();
	BRect		frame = window->Frame ();
	screen.InsetBy (4, 8);
	screen.OffsetBy (0, 8);
	if (mayResize)
	{
		float width = frame.Width ();
		float height = frame.Height ();
		if (screen.Width () < width && !(flags & B_NOT_H_RESIZABLE))
			width = screen.Width ();
		if (screen.Height () < height && !(flags & B_NOT_V_RESIZABLE))
			height = screen.Height ();
		if (width != frame.Width () || height != frame.Height ())
		{
			window->ResizeTo (width, height);
			frame.right = frame.left + width;
			frame.bottom = frame.top + height;
		}
	}
	if (frame.right > screen.right)
		window->MoveBy (screen.right-frame.right, 0);
	if (frame.bottom > screen.bottom)
		window->MoveBy (0, screen.bottom-frame.bottom);
	if (frame.left < screen.left)
		window->MoveTo (screen.left, frame.top);
	if (frame.top < screen.top)
		window->MoveBy (0, screen.top-frame.top);
}
