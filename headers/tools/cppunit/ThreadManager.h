#ifndef _beos_thread_manager_h_
#define _beos_thread_manager_h_

#include <cppunit/Exception.h>
#include <cppunit/TestResult.h>
#include <OS.h>
#include <signal.h>
#include <string>

// Pointer to a function that takes no parameters and
// returns no result. All threads must be implemented
// in a function of this type.

// Helper class to handle thread management
template <class TestClass, class ExpectedException>
class CPPUNIT_API BThreadManager {
public:
	typedef void (TestClass::*ThreadMethod)();
	
	BThreadManager(std::string threadName, TestClass *object, ThreadMethod method, sem_id &threadSem);
	~BThreadManager();
	
    status_t LaunchThread(CppUnit::TestResult *result);
    
	// Thread management methods
	int32 Stop();
	int32 WaitForThread();
	bool IsRunning();

	std::string getName() const { return fName; }

protected:
	std::string fName;
	TestClass *fObject;
	ThreadMethod fMethod;
	thread_id fID;
	CppUnit::TestResult *fTestResult;
	sem_id &fThreadSem;
	
	static long EntryFunction(BThreadManager<TestClass, ExpectedException>* manager);
	void Run();	

};

template <class TestClass, class ExpectedException>
BThreadManager<TestClass, ExpectedException>::BThreadManager(
	std::string threadName,
	TestClass *object,
	ThreadMethod method,
	sem_id &threadSem
)
	: fName(threadName)
	, fObject(object)
	, fMethod(method)
	, fID(0)
	, fTestResult(NULL)
	, fThreadSem(threadSem)
{
}
	
	
template <class TestClass, class ExpectedException>
BThreadManager<TestClass, ExpectedException>::~BThreadManager() {
	Stop();
}
	
	
template <class TestClass, class ExpectedException>
int32
BThreadManager<TestClass, ExpectedException>::WaitForThread() {
	int32 result = 0;
	if (find_thread(NULL) != fID) 
		wait_for_thread(fID, &result);
	return result;
}
		
template <class TestClass, class ExpectedException>
int32
BThreadManager<TestClass, ExpectedException>::Stop() {
	int32 result = 0;
	if (find_thread(NULL) != fID) {
		while (IsRunning()) {
			kill(fID, SIGINT);
			snooze(1000000);
		}
		result = WaitForThread();
	}	
	return result;
}
	
	
template <class TestClass, class ExpectedException>
bool
BThreadManager<TestClass, ExpectedException>::IsRunning(void) {
	if (fID != 0) {
		thread_info info;
		if (get_thread_info(fID, &info) == B_OK)
			return true;
		else
			fID = 0;
	}
	return false;
}
	
	
template <class TestClass, class ExpectedException>
status_t
BThreadManager<TestClass, ExpectedException>::LaunchThread(CppUnit::TestResult *result) {
	if (IsRunning())
		return B_ALREADY_RUNNING;

	fTestResult = result;
	fID = spawn_thread((thread_entry)(BThreadManager::EntryFunction),
		fName.c_str(), B_NORMAL_PRIORITY, this);
					
	status_t err;
	if (fID == B_NO_MORE_THREADS || fID == B_NO_MEMORY) {
		err = fID;
		fID = 0;
	} else {
		// Aquire the semaphore, then start the thread.
		if (acquire_sem(fThreadSem) != B_OK)
			throw CppUnit::Exception("BThreadManager::LaunchThread() -- Error acquiring thread semaphore");
		err = resume_thread(fID);
	}
	return err;
}
	
	
template <class TestClass, class ExpectedException>
long
BThreadManager<TestClass, ExpectedException>::EntryFunction(BThreadManager<TestClass, ExpectedException> *manager) {
	manager->Run();
	return 0;
}
	
template <class TestClass, class ExpectedException>
void
BThreadManager<TestClass, ExpectedException>::Run(void) {
	// These outer try/catch blocks handle unexpected exceptions.
	// Said exceptions are caught and noted in the TestResult
	// class, but not allowed to escape and disrupt the other
	// threads that are assumingly running concurrently.
	try {
	
		// Our parent ThreadedTestCaller should check fObject to be non-NULL,
		// but we'll do it here too just to be sure.
		if (!fObject)
			throw CppUnit::Exception("BThreadManager::Run() -- NULL fObject pointer");
		
		// Before running, we need to add this thread's name to
		// the object's id->(name,subtestnum) map.
		fObject->InitThreadInfo(fID, fName);

		// This inner try/catch block is for expected exceptions.
		// If we get through it without an exception, we have to
		// raise a different exception that makes note of the fact
		// that the exception we were expecting didn't arrive. If
		// no exception is expected, then nothing is done (see
		// "cppunit/TestCaller.h" for detail on the classes used
		// to handle this behaviour).
		try {
			(fObject->*fMethod)();		
	    } catch ( ExpectedException & ) {
			return;
		}		
	  	CppUnit::ExpectedExceptionTraits<ExpectedException>::expectedException();
	  	
	} catch ( CppUnit::Exception &e ) {
		// Add on the thread name, then note the exception
        CppUnit::Exception *threadException = new CppUnit::Exception(
        	std::string(e.what()) + " (thread: " + fName + ")",
        	e.sourceLine()
        );        	        	
		fTestResult->addFailure( fObject, threadException );
	}
	catch ( std::exception &e ) {
		// Add on the thread name, then note the exception
        CppUnit::Exception *threadException = new CppUnit::Exception(
        	std::string(e.what()) + " (thread: " + fName + ")"
        );        	        	
		fTestResult->addError( fObject, threadException );
	}
	catch (...) {
		// Add on the thread name, then note the exception
		CppUnit::Exception *threadException = new CppUnit::Exception(
			"caught unknown exception (thread: " + fName + ")"
		);
		fTestResult->addError( fObject, threadException );
	}
	
	// Release the semaphore we acquired earlier
	release_sem(fThreadSem);  	
}
	

#endif
