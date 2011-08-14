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


// DormantNodeListItem.h
// e.moon 2jun99
//
// * PURPOSE
// - represent a single dormant (add-on) media node in a
//   list view.
//
// * HISTORY
//   22oct99	c.lenz		complete rewrite
//   27oct99	c.lenz		added tooltip support

#ifndef __DormantNodeListItem_H__
#define __DormantNodeListItem_H__

// Interface Kit
#include <Bitmap.h>
#include <Font.h>
#include <ListItem.h>
// Media Kit
#include <MediaAddOn.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DormantNodeView;
class MediaIcon;

int compareName(const void *lValue, const void *rValue);
int compareAddOnID(const void *lValue, const void *rValue);


class DormantNodeListItem :
	public	BListItem {
	typedef	BListItem _inherited;
	
public:					// *** constants

	// [e.moon 26oct99] moved definition to DormantNodeListItem.cpp
	// to appease the Metrowerks compiler.
	static const float M_ICON_H_MARGIN;
	static const float M_ICON_V_MARGIN;

public:					// *** ctor/dtor

						DormantNodeListItem(
							const dormant_node_info &nodeInfo);

	virtual				~DormantNodeListItem();

public:					// *** accessors

	const dormant_node_info &info() const 
						{ return m_info; }

public:					// *** operations

	void				MouseOver(
							BView *owner,
							BPoint point,
							uint32 transit);

	BRect				getRealFrame(
							const BFont *font) const;

	BBitmap			   *getDragBitmap();
	
	void				showContextMenu(
							BPoint point,
							BView *owner);
	
public:					// *** BListItem impl.

	virtual void		DrawItem(
							BView *owner,
							BRect frame,
							bool drawEverything = false);

	virtual void		Update(
							BView *owner,
							const BFont *fFont);
		
protected:				// *** compare functions

	friend int			compareName(
							const void *lValue,
							const void *rValue);

	friend int			compareAddOnID(
							const void *lValue,
							const void *rValue);

private:				// *** data

	dormant_node_info	m_info;

	BRect				m_frame;

	font_height			m_fontHeight;
	
	MediaIcon		   *m_icon;
};

__END_CORTEX_NAMESPACE
#endif /*__DormantNodeListItem_H__*/
