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


// DormantNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   An InfoView variant for dormant MediaNodes. Accepts
//   a dormant_node_info struct in the constructor and 
//   tries to aquire the corresponding dormant_flavor_info
//   from the BMediaRoster.
//
// * HISTORY
//   c.lenz		5nov99		Begun
//

#ifndef __DormantNodeInfoView_H__
#define __DormantNodeInfoView_H__

#include "InfoView.h"

#include "cortex_defs.h"


struct dormant_node_info;

__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class DormantNodeInfoView :
	public InfoView {

public:					// *** ctor/dtor

	// add fields relevant for dormant MediaNodes (like 
	// AddOn-ID, input/output formats etc.)
						DormantNodeInfoView(
							const dormant_node_info &info);

	virtual				~DormantNodeInfoView();

public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** data members

	int32				m_addOnID;

	int32				m_flavorID;
};

__END_CORTEX_NAMESPACE
#endif /* __DormantNodeInfoView_H__ */
