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


// BasicThread.h
// based on ThreadPrimitive from the Be Newsletter
//
// HISTORY
//   e.moon		12jul99: now in cortex namespace
//   - The stop mechanism has changed a bit; it's now up to the
//     run() implementation to indicate that it has completed by
//     calling done().
//                    
//   e.moon		11may99: brought into the beDub support kit

#ifndef __BasicThread_H__
#define __BasicThread_H__

#include <Debug.h>
#include <KernelKit.h>
#include <String.h>


#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class BasicThread {

protected:				// hooks
	virtual void run()=0;

public:					// ctor/dtor
	virtual ~BasicThread() {

		if(m_thread > 0) {
			kill();
		}
	}
	
	BasicThread(
		int32 priority=B_LOW_PRIORITY,
		const char* name=0,
		bool killOnDelete=true) :
		
		m_thread(-1),
		m_priority(priority),
		m_name(name ? name : "BasicThread [?]"),
		m_running(false),
		m_stopping(false) {}
	
	bool running() const { return m_running; }
	bool stopping() const { return m_stopping; }
	
	thread_id thread() const { return m_thread; }
	const char* name() const { return m_name.String(); }
		
public:					// operations
	status_t start() {

		if(running())
			return B_ERROR;
			
		m_thread = spawn_thread(
			&BasicThread::Run,
			m_name.String(),
			m_priority,
			this);

		status_t err = resume_thread(m_thread);
		if(err == B_OK)
			m_running = true;
		return err;
	}

	// reworked 12jul99 e.moon
	status_t stop(bool wait=true) {

		if(!running())
			return B_ERROR;

		thread_id thread = m_thread; // +++++ is this safe?
		if(thread <= 0)
			return B_OK;

		// signal stop
		m_stopping = true;
		
		if(wait) {
			status_t ret;
			while(wait_for_thread(thread, &ret) == B_INTERRUPTED) {
				PRINT(("stopping thread %ld: B_INTERRUPTED\n", thread));
			}
		}
		
		return B_OK;
	}

protected:					// internal methods	

	// Blunt force trauma.
	// added 12jul99 e.moon
	status_t kill() {
		if(m_thread <= 0)
			return B_ERROR;

		kill_thread(m_thread);
		
		m_thread = -1;
		m_running = false;
		m_stopping = false;
		return B_OK;
	}

	// Call this method at the end of your run() implementation.
	// added 12jul99 e.moon
	void done() {
		m_running = false;
		m_stopping = false;
		m_thread = -1;
	}

private:						// static impl. methods
	static status_t Run(void* user) {

		BasicThread* instance = static_cast<BasicThread*>(user);
		instance->run();
		return B_OK;
	}
	
private:						// impl members
	thread_id		m_thread;
	int32				m_priority;
	BString			m_name;

	bool					m_running;	
	bool					m_stopping;
};

__END_CORTEX_NAMESPACE
#endif /* __BasicThread_H__ */
