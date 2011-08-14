/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// InfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   A base class for displaying info in a list of fields,
//   where each field has a label and the actual content text.
//   This class has to be subclassed for providing info on
//   specific objects, by adding fields to the view in the
//   constructor of the subclass. InfoView takes care of all
//   the display details, dynamically rearranging text on resize
//   if necessary.
//
// * TODO
//   Maybe add more field types, e.g. for options (checkboxes) or
//   dropdown-menus ?
//
// * HISTORY
//   c.lenz		5nov99		Begun
//

#ifndef __InfoView_H__
#define __InfoView_H__

// Interface Kit
#include <View.h>
// Support Kit
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class InfoView :
	public BView {

public:					// *** constants

	// the default frame for an InfoView. Is not actually
	// needed right now, because the frame is set to an
	// 'ideal' size when the view is attached to the window
	static const BRect	M_DEFAULT_FRAME;

	// defines how much space is used to separate objects
	// horizontally
	static const float	M_H_MARGIN;
	
	// defines how much space is used to separate objects
	// vertically
	static const float	M_V_MARGIN;

public:					// *** ctor/dtor

	// creates a view containing only the title and subtitle
	// and the icon (if provided). No fields are defined here.
						InfoView(
							BString title,
							BString subTitle,
							BBitmap *icon);

	virtual				~InfoView();

public:					// *** BView impl.

	// resizes the view to an 'ideal' size and inits linewrapping
	// for each field
	virtual void		AttachedToWindow();

	// updates every fields linewrapping
	virtual void		FrameResized(
							float width,
							float height);

	// returns the ideal size needed to display all text without
	// wrapping lines
	virtual void		GetPreferredSize(
							float *width,
							float *height);

	// draws the title, subtitle, sidebar & icon as well as
	// every field
	virtual void		Draw(
							BRect updateRect);
	
public:					// *** accessors

	// adjusts the sidebars' width
	void				setSideBarWidth(
							float width)
						{ m_sideBarWidth = width; }

	// returns the sidebars' width
	float				getSideBarWidth() const
						{ return m_sideBarWidth; }					

	// set the title (also used for window title)
	void				setTitle(
							BString title)
						{ m_title = title; }

	// set the string which will be displayed just below
	// the title
	void				setSubTitle(
							BString subTitle)
						{ m_subTitle = subTitle; }

protected:				// *** operations

	// add a field with the given label and 'content'-text.
	// fields are displayed in the order they are added!
	// as there is no way to update the fields (yet), these
	// should always be added in the constructor of the
	// subclass!
	void				addField(
							BString label,
							BString text);
	
private:				// *** data members

	// the objects title, which will appear at top of the view
	// and in the windows titlebar
	BString				m_title;

	// a string to be displayed right beneath the title, using a
	// smaller font
	BString				m_subTitle;

	// an icon representation of the object
	BBitmap			   *m_icon;
	
	// a list of the InfoTextField objects registered thru addField()
	BList			   *m_fields;

	// the width of the sidebar holding label strings
	float				m_sideBarWidth;
	
	// cached frame rect for proper redrawing of the sidebar
	BRect				m_oldFrame;
};

__END_CORTEX_NAMESPACE
#endif /* __InfoView_H__ */
