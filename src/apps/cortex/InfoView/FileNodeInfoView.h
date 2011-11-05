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


// FileNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   In addition to the fields defined by its base class
//   LiveNodeInfoView, this class defines media-file specific
//   fields, line number of tracks, codecs, formats, etc.
//
// * HISTORY
//   c.lenz		13nov99		Begun
//

#ifndef __FileNodeInfoView_H__
#define __FileNodeInfoView_H__

#include "LiveNodeInfoView.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class FileNodeInfoView : public LiveNodeInfoView
{

public:					// *** ctor/dtor

	// add file related fields to the view by creating
	// and querying an instance of BMediaFile/BMediaTrack
						FileNodeInfoView(
							const NodeRef *ref);

	virtual				~FileNodeInfoView();
};

__END_CORTEX_NAMESPACE
#endif /* __FileNodeInfoView_H__ */
