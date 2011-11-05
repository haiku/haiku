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


// DormantNodeView.h
// c.lenz 22oct99
//
// RESPONSIBILITIES
// - simple extension of BListView to support
//	 drag & drop
//
// HISTORY
//   c.lenz		22oct99		Begun
//   c.lenz     27oct99		Added ToolTip support

#ifndef __DormantNodeView_H__
#define __DormantNodeView_H__

// Interface Kit
#include <ListView.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DormantNodeView :
	public	BListView {
	typedef	BListView _inherited;
	
public:					// *** messages

	enum message_t {
		// OUTBOUND:
		// B_RAW_TYPE "which" dormant_node_info
		M_INSTANTIATE_NODE				= 'dNV0'
	};
	
public:					// *** ctor/dtor

						DormantNodeView(
							BRect frame,
							const char *name,
							uint32 resizeMode);

	virtual				~DormantNodeView();
		
public:					// *** BListView impl.

	virtual void		AttachedToWindow();
	
	virtual void		DetachedFromWindow();

	virtual void		GetPreferredSize(
							float* width,
							float* height);

	virtual void		MessageReceived(
							BMessage *message);

	virtual void		MouseDown(
							BPoint point);

	virtual void		MouseMoved(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	virtual bool		InitiateDrag(
							BPoint point,
							int32 index,
							bool wasSelected);

private:				// *** internal operations

	void				_populateList();

	void				_freeList();

	void				_updateList(
							int32 addOnID);

private:				// *** data

	BListItem		   *m_lastItemUnder;
};

__END_CORTEX_NAMESPACE
#endif /*__DormantNodeView_H__*/
