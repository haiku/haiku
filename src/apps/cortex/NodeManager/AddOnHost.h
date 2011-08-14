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


// cortex::NodeManager::AddOnHost.h
// * PURPOSE
//   Provides an interface to a separate BApplication whose
//   single responsibility is to launch nodes.
//   NodeManager-launched nodes now run in a separate team,
//   helping to lower the likelihood of a socially maladjusted
//   young node taking you out.
//
// * HISTORY
//   e.moon			6nov99

#ifndef __NodeManager_AddOnHost_H__
#define __NodeManager_AddOnHost_H__

#include <Application.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class AddOnHost {

public:												// *** static interface

	static status_t FindInstance(
		BMessenger*								outMessenger);

	static status_t Kill(
		bigtime_t									timeout=B_INFINITE_TIMEOUT);

	// returns B_NOT_ALLOWED if an instance has already been launched
	static status_t Launch(
		BMessenger*								outMessenger=0);
		
	static status_t InstantiateDormantNode(
		const dormant_node_info&	info,
		media_node*								outNode,
		bigtime_t									timeout=B_INFINITE_TIMEOUT);
		
	static status_t ReleaseInternalNode(
		const live_node_info&			info,
		bigtime_t									timeout=B_INFINITE_TIMEOUT);
		
	static BMessenger						s_messenger;
		
private:											// implementation
	friend class _AddOnHostApp;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeManager_AddOnHost_H__*/

