#ifndef _beos_locker_sync_object_h_
#define _beos_locker_sync_object_h_

#include <cppunit/SynchronizedObject.h>
#include <Locker.h>

//! BLocker based implementation of CppUnit::SynchronizedObject::SynchronizationObject
/*!	This class is used to serialize access to a TestResult object. You should
	not need to explicitly use it anywhere in your testing code.
*/
class CPPUNIT_API LockerSyncObject : public CppUnit::SynchronizedObject::SynchronizationObject {
public:
	LockerSyncObject() {}
	virtual ~LockerSyncObject() {}

	virtual void lock() { fLock.Lock(); }
	virtual void unlock() { fLock.Unlock(); }

protected:
	BLocker fLock;

private:
  //! Prevents the use of the copy constructor.
  LockerSyncObject( const LockerSyncObject &copy );

  //! Prevents the use of the copy operator.
  void operator =( const LockerSyncObject &copy );
  
};

#endif  // _beos_synchronization_object_h_
