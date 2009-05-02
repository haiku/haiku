#ifndef _beos_threaded_test_caller_h_
#define _beos_threaded_test_caller_h_

//#include <memory>
#include <cppunit/TestCase.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestCaller.h>
#include <TestShell.h>
#include <ThreadManager.h>
#include <map>
#include <vector>
#include <stdio.h>

class TestResult;

template <class TestClass, class ExpectedException = CppUnit::NoExceptionExpected>
class CPPUNIT_API BThreadedTestCaller : public CppUnit::TestCase {
public:
	/*! \brief Pointer to a test function in the given class.
		Each ThreadMethod added with addThread() is run in its own thread.
	*/
	typedef void (TestClass::*ThreadMethod)();

	BThreadedTestCaller(std::string name);
	BThreadedTestCaller(std::string name, TestClass &object);
	BThreadedTestCaller(std::string name, TestClass *object);
	virtual ~BThreadedTestCaller();

    virtual CppUnit::TestResult *run();
    virtual void run(CppUnit::TestResult *result);

	//! Adds a thread to the test. \c threadName must be unique to this BThreadedTestCaller.
    void addThread(std::string threadName, ThreadMethod method);

protected:
	virtual void setUp();
	virtual void tearDown();
	virtual std::string toString() const;

	typedef std::map<std::string, BThreadManager<TestClass, ExpectedException> *> ThreadManagerMap;

	bool fOwnObject;
	TestClass *fObject;
	ThreadManagerMap fThreads;

	sem_id fThreadSem;

};


template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name)
	: TestCase(name)
	, fOwnObject(true)
	, fObject(new TestClass())
	, fThreadSem(-1)
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name, TestClass &object)
	: TestCase(name)
	, fOwnObject(false)
	, fObject(&object)
	, fThreadSem(-1)
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name, TestClass *object)
	: TestCase(name)
	, fOwnObject(true)
	, fObject(object)
	, fThreadSem(-1)
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::~BThreadedTestCaller() {
	if (fOwnObject)
		delete fObject;
	for (typename ThreadManagerMap::iterator it = fThreads.begin(); it != fThreads.end (); ++it) {
		delete it->second;
	}
}


template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::addThread(std::string threadName, ThreadMethod method) {
	if (fThreads.find(threadName) == fThreads.end()) {
		// Unused name, go ahead and add
		fThreads[threadName] = new BThreadManager<TestClass, ExpectedException>(threadName, fObject, method, fThreadSem);
	} else {
		// Duplicate name, throw an exception
		throw CppUnit::Exception("BThreadedTestCaller::addThread() - Attempt to add thread under duplicated name ('"
			+ threadName + "')");
	}
}

template <class TestClass, class ExpectedException>
CppUnit::TestResult *
BThreadedTestCaller<TestClass, ExpectedException>::run() {
	CppUnit::TestResult *result = new CppUnit::TestResult;
	run(result);
	return result;
}

template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::run(CppUnit::TestResult *result) {
	result->startTest(this);

	if (fThreads.size() <= 0)
		throw CppUnit::Exception("BThreadedTestCaller::run() -- No threads added to BThreadedTestCaller()");

	try {
		setUp();

		// This try/catch block should never actually have to catch
		// anything (unless some bonehead passes in a NULL pointer to
		// the constructor). Each BThreadManager object catches and
		// handles exceptions for its respective thread, so as not
		// to disrupt the others.
		try {
			// Create our thread semaphore. This semaphore is used to
			// determine when all the threads have finished executing,
			// while still allowing *this* thread to handle printing
			// out NextSubTest() info (since other threads don't appear
			// to be able to output text while the main thread is
			// blocked; their output appears later...).
			//
			// Each thread will acquire the semaphore once when launched,
			// thus the initial thread count is equal the number of threads.
			fThreadSem = create_sem(fThreads.size(), "ThreadSem");
			if (fThreadSem < B_OK)
				throw CppUnit::Exception("BThreadedTestCaller::run() -- Error creating fThreadSem");

			// Launch all the threads.
			for (typename ThreadManagerMap::iterator i = fThreads.begin();
				   i != fThreads.end ();
	    	         ++i)
	    	{
	    		status_t err = i->second->LaunchThread(result);
				if (err != B_OK)
					result->addError(this, new CppUnit::Exception("Error launching thread '" + i->second->getName() + "'"));
//				printf("Launch(%s)\n", i->second->getName().c_str());
			}

			// Now we loop. Before you faint, there is a reason for this:
			// Calls to NextSubTest() from other threads don't actually
			// print anything while the main thread is blocked waiting
			// for another thread. Thus, we have NextSubTest() add the
			// information to be printed into a queue. The main thread
			// (this code right here), blocks on a semaphore that it
			// can only acquire after all the test threads have terminated.
			// If it times out, it checks the NextSubTest() queue, prints
			// any pending updates, and tries to acquire the semaphore
			// again. When it finally manages to acquire it, all the
			// test threads have terminated, and it's safe to clean up.

			status_t err;
			do {
				// Try to acquire the semaphore
				err = acquire_sem_etc(fThreadSem, fThreads.size(), B_RELATIVE_TIMEOUT,	500000);

				// Empty the UpdateList
				std::vector<std::string> &list = fObject->AcquireUpdateList();
				for (std::vector<std::string>::iterator i = list.begin();
					   i != list.end();
					     i++)
				{
					// Only print to standard out if the current global shell
					// lets us (or if no global shell is designated).
					if (BTestShell::GlobalBeVerbose()) {
						printf("%s", (*i).c_str());
						fflush(stdout);
					}
				}
				list.clear();
				fObject->ReleaseUpdateList();

			} while (err != B_OK);

			// If we get this far, we actually managed to acquire the semaphore,
			// so we should release it now.
			release_sem_etc(fThreadSem, fThreads.size(), 0);

			// Print out a newline for asthetics :-)
			printf("\n");

/*


			// Wait for them all to finish, then clean up
			for (ThreadManagerMap::iterator i = fThreads.begin();
				  i != fThreads.end ();
	    	        ++i)
			{
//				printf("Wait(%s)...", i->second->getName().c_str());
				fflush(stdout);
				i->second->WaitForThread();
//				printf("done\n");
				delete i->second;
			}
*/

			fThreads.clear();

		} catch ( CppUnit::Exception &e ) {
			// Add on the a note that this exception was caught by the
			// thread caller (which is a bad thing), then note the exception
	        CppUnit::Exception *threadException = new CppUnit::Exception(
	        	std::string(e.what()) + " (NOTE: caught by BThreadedTestCaller)",
	        	e.sourceLine()
	        );
			result->addFailure( fObject, threadException );
		}
		catch ( std::exception &e ) {
			// Add on the thread name, then note the exception
	        CppUnit::Exception *threadException = new CppUnit::Exception(
	        	std::string(e.what()) + " (NOTE: caught by BThreadedTestCaller)"
	        );
			result->addError( fObject, threadException );
		}
		catch (...) {
			// Add on the thread name, then note the exception
			CppUnit::Exception *threadException = new CppUnit::Exception(
				"caught unknown exception (NOTE: caught by BThreadedTestCaller)"
			);
			result->addError( fObject, threadException );
		}

		snooze(50000);

		try {
		    tearDown();
		} catch (...) {
			result->addError(this, new CppUnit::Exception("tearDown() failed"));
		}
	} catch (...) {
		result->addError(this, new CppUnit::Exception("setUp() failed"));
	}	// setUp() try/catch block

	result->endTest(this);
}

template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::setUp() {
	// Verify we have a valid object that's not currently in use first.
	if (!fObject)
		throw CppUnit::Exception("BThreadedTestCaller::runTest() -- NULL fObject pointer");
	if (!fObject->RegisterForUse())
		throw CppUnit::Exception("BThreadedTestCaller::runTest() -- Attempt to reuse ThreadedTestCase object already in use");

	fObject->setUp();
}

template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::tearDown() {
	fObject->tearDown();
}

template <class TestClass, class ExpectedException>
std::string
BThreadedTestCaller<TestClass, ExpectedException>::toString() const {
	return std::string("BThreadedTestCaller for ") + getName();
}

#endif // _beos_threaded_test_caller_h_
