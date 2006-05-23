/*

	URLView.cpp

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

#include "URLView.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Roster.h>
#include "Colors.h"

URLView::URLView(BRect frame, const char* url)
	: BView(frame, NULL, B_FOLLOW_NONE, B_WILL_DRAW)
{
	if (url == NULL)
		url = "http://www.be.com";
	fUrl = strdup (url);
}

URLView::~URLView()
{
	free (fUrl);
}

void URLView::AttachedToWindow()
{
	rgb_color back = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor (back);
	SetLowColor (back);
	SetHighColor (kBlue);
	SetFont (be_plain_font);
	font_height height;
	GetFontHeight (&height);
	float width = StringWidth (fUrl);
	float r = width + 8;
	BRect frame = Frame ();
	BRect nframe = frame;
	if (frame.right == frame.left)
		nframe.right = nframe.left + r - 4;
	else if (frame.right == 0)
	{
		nframe.left -= r/2;
		nframe.right = nframe.left + r;
	}
	if (frame.bottom == 0)
		nframe.bottom = frame.top + height.ascent + height.descent + 2;
	if (frame.left != nframe.left || frame.top != nframe.top)
		MoveTo (nframe.left, nframe.top);
	if (frame.Width () != nframe.Width () || frame.Height () != nframe.Height ())
		ResizeTo (nframe.Width (), nframe.Height ());
	fLocation.x = ceil ((nframe.Width () - width) / 2) + 1;
	fLocation.y = floor ((nframe.Height () + height.ascent - height.descent) / 2);
}

void URLView::Draw (BRect /*updateRect*/)
{
	DrawString (fUrl, fLocation);
}

void URLView::MouseDown (BPoint /*where*/)
{
	BPoint	pos;
	uint32	buttons;
	BRect	bounds = Bounds ();
	bool	inside = true;
	InvertRect (bounds);
	do
	{
		snooze (5000);
		GetMouse (&pos, &buttons);
		bool now = bounds.Contains (pos);
		if (now != inside)
		{
			InvertRect (bounds);
			Flush ();
			inside = now;
		}
	} while (buttons != 0);
	if (bounds.Contains (pos))
		OpenURL ();
	if (inside)
		InvertRect (bounds);
}

void URLView::OpenURL ()
{
	char*	argv[2];
	argv[0] = fUrl;
	argv[1] = 0;
	be_roster->Launch ("text/html", 1, argv);
}

mailtoView::mailtoView (BRect frame, const char* email, const char* subject, const char* texte)
	: URLView (frame, email)
{
	if (subject)
		fSubject = strdup (subject);
	else
		fSubject = NULL;
	if (texte)
		fTexte = strdup (texte);
	else
		fTexte = NULL;
}

mailtoView::~mailtoView()
{
	if (fSubject)
		free (fSubject);
	if (fTexte)
		free (fTexte);
}

void mailtoView::OpenURL ()
{
	int total = 7;
	char**	argv = new char*[total];
	memset (argv, 0, total * sizeof (char*));
	argv[0] = "email";
	int argc = 0;
	if (fUrl)
	{
		char email[256];
		strcpy (email, "mailto:");
		strcat (email, fUrl);
		argv[++argc] = email;
	}
	if (fSubject)
	{
		argv[++argc] = "-subject";
		argv[++argc] = fSubject;
	}
	if (fTexte)
	{
		argv[++argc] = "-body";
		argv[++argc] = fTexte;
	}
	argv[++argc] = 0;
	be_roster->Launch ("text/x-email", argc, argv);
}
