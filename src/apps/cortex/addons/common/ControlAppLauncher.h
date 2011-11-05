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


// ControlAppLauncher.h
// * PURPOSE
//   A ControlAppLauncher manages a control-panel application
//   for a given Media Kit node.  It takes responsibility for
//   launching the application, maintaining a BMessenger to
//   it for control purposes, and telling it to quit upon
//   destruction.
//
// * HISTORY
//   e.moon		17jun99		Begun.

#ifndef __ControlAppLauncher_H__
#define __ControlAppLauncher_H__

#include <kernel/image.h>
#include <MediaDefs.h>
#include <Messenger.h>

class ControlAppLauncher {
public:						// ctor/dtor
	virtual ~ControlAppLauncher(); //nyi
	
	ControlAppLauncher(
		media_node_id mediaNode,
		bool launchNow=true); //nyi
				
public:						// accessors
	bool launched() const { return m_launched; }
	status_t error() const { return m_error; }
	const BMessenger& messenger() const { return m_messenger; }

public:						// operations
	status_t launch(); //nyi
	
private:						// guts
	void quit();

	bool							m_launched;
	status_t				m_error;
	BMessenger			m_messenger;
};

#endif /*__ControlAppLauncher_H__*/
