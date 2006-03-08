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
