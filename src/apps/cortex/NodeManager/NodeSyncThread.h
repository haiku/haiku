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


// NodeSyncThread.h [rewrite 14oct99]
// * PURPOSE
//   Provide continuous synchronization notices on
//   a particular BMediaNode.  Notification is sent
//   to a provided BMessenger.
//
//   As long as a NodeSyncThread object exists its thread
//   is running.  The thread blocks indefinitely, waiting
//   for a message to show up on an internal port.
//
//   Sync-notice requests (via the +++++ sync() operation)
//   trigger a message sent to that port.  The thread wakes
//   up, then calls BMediaRoster::SyncToNode() to wait
//   until a particular performace time arrives for that node.
//
//   If SyncToNode() times out, an M_TIMED_OUT message is sent;
//   otherwise, an M_SYNC_COMPLETE message is sent.
//
// * HISTORY
//   e.moon		14oct99		Rewrite begun.

#ifndef __NodeSyncThread_H__
#define __NodeSyncThread_H__

#include <MediaNode.h>
#include <Messenger.h>
#include <OS.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeSyncThread {
public:													// *** messages
	enum message_t {
		// 'nodeID' (int32)         media_node_id value
		// 'perfTime' (int64)				the performance time
		// 'position' (int64)       corresponding (caller-provided) position
		M_SYNC_COMPLETE							=NodeSyncThread_message_base,
		
		// 'nodeID' (int32)         media_node_id value
		// 'error' (int32)					status_t value from SyncToNode()
		// 'perfTime' (int64)				the performance time
		// 'position' (int64)       corresponding (caller-provided) position
		M_SYNC_FAILED
	};
	
public:													// *** dtor/ctors
	virtual ~NodeSyncThread();

	NodeSyncThread(
		const media_node&						node,
		BMessenger*									messenger);

public:													// *** operations
	// trigger a sync operation: when 'perfTime' arrives
	// for the node, a M_SYNC_COMPLETE message with the given
	// position value will be sent,  unless the sync operation
	// times out, in which case M_TIMED_OUT will be sent.
	status_t sync(
		bigtime_t										perfTime,
		bigtime_t										position,
		bigtime_t										timeout);

private:

	// the node to watch
	media_node										m_node;
	
	// the messenger to inform
	BMessenger*										m_messenger;

	// if the thread is running, it has exclusive write access
	// to this flag.
	volatile bool									m_syncInProgress;

	// thread guts
	thread_id											m_thread;
	port_id												m_port;
	char*													m_portBuffer;
	ssize_t												m_portBufferSize;

	enum _op_codes {
		M_TRIGGER
	};

	struct _sync_op {
		bigtime_t 	targetTime;
		bigtime_t		position;
		bigtime_t		timeout;
	};
	
	static status_t _Sync(
		void*												cookie);
	void _sync();
};

__END_CORTEX_NAMESPACE
#endif /*__NodeSyncThread_H__*/
