/*

	URLView.h

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

#ifndef _URLVIEW_H_
#define _URLVIEW_H_

#include <View.h>

class URLView : public BView {

	public:
					URLView (BRect frame, const char* url);
					~URLView ();
	virtual	void	AttachedToWindow ();
	virtual	void	Draw (BRect updateRect);
	virtual	void	MouseDown (BPoint where);
	virtual void	OpenURL ();

	protected:
		char*		fUrl;
		BPoint		fLocation;
};

class mailtoView : public URLView {
	public:
					mailtoView (BRect frame, const char* email, const char* subject = NULL, const char* texte = NULL);
					~mailtoView ();
	virtual void	OpenURL ();
	
	protected:
		char*		fSubject;
		char*		fTexte;
};

#endif // _URLVIEW_H_
