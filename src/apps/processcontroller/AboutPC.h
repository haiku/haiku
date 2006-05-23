/*

	AboutPC.h

	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
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

#ifndef _ABOUT_PC_H_
#define _ABOUT_PC_H_

class BBitmap;

#include <Window.h>
#include <View.h>
#include <Font.h>

class AboutPC : public BWindow {

public:
				AboutPC(BRect r);
};

class AboutView : public BView {

public:
				AboutView(BRect frame);
				~AboutView();
virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);

private:
		BFont		fFont;
		BBitmap		*fIcon;
};

#define CenterRect(r, W, H) BRect(r.left+(r.Width()-W)/2, r.top+(r.Height()-H)/3, r.right-(r.Width()-W)/2, r.bottom-2*(r.Height()-H)/3)

#endif // _ABOUT_PC_H_
