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


// cortex::NodeManager::AddOnHostApp.h
// * PURPOSE
//   Definition of (and provisions for communication with)
//   a separate BApplication whose single responsibility is
//   to launch nodes.  NodeManager-launched nodes run in
//   another team, helping to lower the likelihood of a
//   socially maladjusted young node taking you out.
//
// * HISTORY
//   e.moon			6nov99
#ifndef NODE_MANAGER_ADD_ON_HOST_APP_H
#define NODE_MANAGER_ADD_ON_HOST_APP_H


#include "cortex_defs.h"

#include <Application.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>


__BEGIN_CORTEX_NAMESPACE
namespace addon_host {

class App : public BApplication {
	typedef	BApplication _inherited;

	public:
		~App();
		App();

	public:
		bool QuitRequested();
		void MessageReceived(BMessage* message);

};

}	// addon_host
__END_CORTEX_NAMESPACE

#endif	// NODE_MANAGER_ADD_ON_HOST_APP_H
