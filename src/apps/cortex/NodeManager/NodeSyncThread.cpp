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


// NodeSyncThread.cpp

#include "NodeSyncThread.h"

#include <cstring>

#include <Debug.h>
#include <MediaRoster.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** dtor/ctors
// -------------------------------------------------------- //

NodeSyncThread::~NodeSyncThread() {
	status_t err;

	// clean up
	if(m_thread) {
		err = kill_thread(m_thread);
		if(err < B_OK)
			PRINT((
				"! ~NodeSyncThread(): kill_thread(%ld):\n"
				"  %s\n",
				m_thread,
				strerror(err)));
				
		// +++++ is a wait_for_thread() necessary?
	}
	
	if(m_port) {
		err = delete_port(m_port);
		if(err < B_OK)
			PRINT((
				"! ~NodeSyncThread(): delete_port(%ld):\n"
				"  %s\n",
				m_port,
				strerror(err)));
	}
	
	if(m_portBuffer)
		delete [] m_portBuffer;
	if(m_messenger)
		delete m_messenger;
}

NodeSyncThread::NodeSyncThread(
	const media_node&						node,
	BMessenger*									messenger) :
	
	m_node(node),
	m_messenger(messenger),
	m_syncInProgress(false),
	m_thread(0),
	m_port(0),
	m_portBuffer(0),
	m_portBufferSize(sizeof(_sync_op)) {		
	
	ASSERT(m_messenger);

	m_portBuffer = new char[m_portBufferSize];

	m_port = create_port(
		m_portBufferSize * 16,
		"NodeSyncThread___port");
	ASSERT(m_port >= B_OK);

	m_thread = spawn_thread(
		&_Sync,
		"NodeSyncThread",
		B_DISPLAY_PRIORITY,
		this);
	ASSERT(m_thread >= B_OK);
	resume_thread(m_thread);
}	

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// trigger a sync operation: when 'perfTime' arrives
// for the node, a M_SYNC_COMPLETE message with the given
// position value will be sent,  unless the sync operation
// times out, in which case M_TIMED_OUT will be sent.

status_t NodeSyncThread::sync(
	bigtime_t										perfTime,
	bigtime_t										position,
	bigtime_t										timeout) {

	status_t err;
	
	if(m_syncInProgress)
		return B_NOT_ALLOWED;	

	_sync_op op = {perfTime, position, timeout};
	err = write_port(
		m_port,
		M_TRIGGER,
		&op,
		sizeof(_sync_op));
	
	return err;
}

// -------------------------------------------------------- //
// *** guts
// -------------------------------------------------------- //

/*static*/
status_t NodeSyncThread::_Sync(
	void*												cookie) {
	((NodeSyncThread*)cookie)->_sync();
	return B_OK;
}

// THREAD BODY
//
void NodeSyncThread::_sync() {
	ASSERT(m_port >= B_OK);
	ASSERT(m_messenger);

	bool done = false;
	while(!done) {
	
		// WAIT FOR A REQUEST
		int32 code;
		ssize_t readCount = read_port(	
			m_port,
			&code,
			m_portBuffer,
			m_portBufferSize);
			
		if(readCount < B_OK) {
			PRINT((
				"! NodeSyncThread::_sync(): read_port():\n"
				"  %s\n",
				strerror(readCount)));
			continue;
		}
		
		if(code != M_TRIGGER) {
			PRINT((
				"! NodeSyncThread::sync(): unknown message code %ld\n",
				code));
			continue;
		}
		
		// SERVICE THE REQUEST
		const _sync_op& op = *(_sync_op*)m_portBuffer;
		
		// pre-fill the message
		BMessage m(M_SYNC_COMPLETE);
		m.AddInt32("nodeID", m_node.node);
		m.AddInt64("perfTime", op.targetTime);
		m.AddInt64("position", op.position);

		// sync
		status_t err = BMediaRoster::Roster()->SyncToNode(
			m_node,
			op.targetTime,
			op.timeout);

		// deliver reply
		if(err < B_OK) {
			m.what = M_SYNC_FAILED;
			m.AddInt32("error", err);
		}
		
		err = m_messenger->SendMessage(&m);
		if(err < B_OK) {
			PRINT((
				"! NodeSyncThread::_sync(): m_messenger->SendMessage():\n"
				"  %s\n",
				strerror(err)));
		}
	}
}


// END -- NodeSyncThread.cpp --
