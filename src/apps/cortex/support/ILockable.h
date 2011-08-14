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


// ILockable.h (Cortex)
// * PURPOSE
//   Simple interface by which an object's locking capabilites
//   are declared/exposed.  Includes support for multiple lock
//   modes (read/write) which objects can choose to support if
//   they wish.
//
// * IMPLEMENTATION/USAGE
//   Implementations are expected to follow the rules described
//   in the MultiLock documentation (Be Developer Newsletter
//   Vol.2 #36, Sep '98).  In brief: write locks can be nested
//   (a thread holding the write lock may nest as many read or
//   or write locks as it likes) but read locks may not (only one
//   per thread.)
//
//   If using a simple BLocker to implement, ignore the mode.
//
// * HISTORY
//   e.moon		26jul99		Moved Autolock into this file to
//											resolve header-naming conflict.
//   e.moon		7jul99		Reworked slightly for Cortex.
//   e.moon		13apr99		Begun (beDub)

#ifndef __ILockable_H__
#define __ILockable_H__

#include <Debug.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// the locking interface

class ILockable {
public:
	ILockable() { }
	virtual ~ILockable() { }
public:						// constants
	enum lock_t {
		WRITE,
		READ
	};

public:						// interface
	virtual bool lock(
		lock_t type=WRITE,
		bigtime_t timeout=B_INFINITE_TIMEOUT)=0;
	
	virtual bool unlock(
		lock_t type=WRITE)=0;
	
	virtual bool isLocked(
		lock_t type=WRITE) const=0;
};



// a simple hands-free lock helper

class Autolock {
public:				// *** ctor/dtor
	virtual ~Autolock() {
		ASSERT(m_target);
		if(m_locked)
			m_target->unlock();
	}

	Autolock(ILockable& target) : m_target(&target) { init(); }
	Autolock(ILockable* target) : m_target( target) { init(); }

	Autolock(const ILockable& target) :
		m_target(const_cast<ILockable*>(&target)) { init(); }		
	Autolock(const ILockable* target) :
		m_target(const_cast<ILockable*>( target)) { init(); }
	
private:
	void init() {
		ASSERT(m_target);
		m_locked = m_target->lock();
	}
	
	ILockable*			m_target;
	bool						m_locked;
};

__END_CORTEX_NAMESPACE
#endif /*__ILockable_H__*/
