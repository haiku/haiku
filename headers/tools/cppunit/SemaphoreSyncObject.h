#ifndef _beos_semaphore_sync_object_h_
#define _beos_semaphore_sync_object_h_

#include <cppunit/SynchronizedObject.h>
#include <OS.h>

//! Semaphore based implementation of CppUnit::SynchronizedObject::SynchronizationObject
/*!	This class is used to serialize access to a TestResult object. You should
	not need to explicitly use it anywhere in your testing code.
*/
class CPPUNIT_API SemaphoreSyncObject : public CppUnit::SynchronizedObject::SynchronizationObject {
public:
	SemaphoreSyncObject();
	virtual ~SemaphoreSyncObject();

	virtual void lock(); 
	virtual void unlock();

protected:
	sem_id fSemId;

private:
  //! Prevents the use of the copy constructor.
  SemaphoreSyncObject( const SemaphoreSyncObject &copy );

  //! Prevents the use of the copy operator.
  void operator =( const SemaphoreSyncObject &copy );
  
};

#endif  // _beos_synchronization_object_h_
