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


// EndPointInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   Display input/output related info in the InfoWindow.
//   Very similar to ConnectionInfoView, only that this
//   views is prepared to handle wildcard values in the
//   media_format (a connection should never have to!)
//
// * HISTORY
//   c.lenz		14nov99		Begun
//

#ifndef __EndPointInfoView_H__
#define __EndPointInfoView_H__

#include "InfoView.h"

// Media Kit
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class EndPointInfoView : 
	public InfoView {

public:					// *** ctor/dtor

	// creates an InfoView for a media_input. adds source
	// and destination fields, then calls _addFormatFields
						EndPointInfoView(
							const media_input &input);

	// creates an InfoView for a media_output. adds source
	// and destination fields, then calls _addFormatFields
						EndPointInfoView(
							const media_output &output);

	virtual				~EndPointInfoView();
	
public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** internal operations

	// adds media_format related fields to the view
	// (wildcard-aware)
	void				_addFormatFields(
							const media_format &format);

private:				// *** data members

	// true if media_output
	bool				m_output;

	int32				m_port;

	int32				m_id;
};

__END_CORTEX_NAMESPACE
#endif /* __EndPointInfoView_H__ */
