#ifndef _beos_synchronization_object_h_
#define _beos_synchronization_object_h_

#include <cppunit/SynchronizedObject.h>
#include <Locker.h>

/*! \brief BLocker based implementation of CppUnit::SynchronizedObject::SynchronizationObject
*/
class LockerSyncObject : public CppUnit::SynchronizedObject::SynchronizationObject {
public:
/*	LockerSyncObject() : fLock(new BLocker()) {}
	virtual ~LockerSyncObject() { cout << 1 << endl; delete fLock; cout << 2 << endl; }

	virtual void lock() { fLock->Lock(); }
	virtual void unlock() { fLock->Unlock(); }
protected:
	BLocker *fLock;
*/
	LockerSyncObject() {}
	virtual ~LockerSyncObject() {}

	virtual void lock() { fLock.Lock(); }
	virtual void unlock() { fLock.Unlock(); }
protected:
	BLocker fLock;
};

#endif  // _beos_synchronization_object_h_
