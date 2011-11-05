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


// SimpleLockable.h
// * PURPOSE
//   Basic BLocker implementation of ILockable.
//
// * HISTORY
//   e.moon		7jul99		Cortex rework.
//   e.moon		13apr99		Begun (beDub)

#ifndef __SimpleLockable_H__
#define __SimpleLockable_H__

#include "ILockable.h"
#include <Locker.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class SimpleLockable :
	public ILockable {

public:						// *** ctor
	virtual ~SimpleLockable() {}
	SimpleLockable() {}
	SimpleLockable(const char* name) : m_lock(name) {}

public:						// *** ILockable impl.
	virtual bool lock(
		lock_t type=WRITE,
		bigtime_t timeout=B_INFINITE_TIMEOUT) {

		return m_lock.LockWithTimeout(timeout) == B_OK;
	}
	
	virtual bool unlock(
		lock_t type=WRITE) {
		m_lock.Unlock();
		return true;
	}
	
	virtual bool isLocked(
		lock_t type=WRITE) const {
		return m_lock.IsLocked();
	}

protected:					// members
	mutable BLocker		m_lock;
};

__END_CORTEX_NAMESPACE
#endif /*__SimpleLockable_H__*/
