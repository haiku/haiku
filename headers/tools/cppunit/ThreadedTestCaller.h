#ifndef _beos_threaded_test_caller_h_
#define _beos_threaded_test_caller_h_

//#include <memory>
#include <cppunit/TestCase.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestCaller.h>
#include <ThreadManager.h>
#include <map>

class TestResult;

template <class TestClass, class ExpectedException = CppUnit::NoExceptionExpected>
class BThreadedTestCaller : public CppUnit::TestCase { 
public:
	/*! \brief Pointer to a test function in the given class.
		Each ThreadMethod added with addThread() is run in its own thread.
	*/
	typedef void (TestClass::*ThreadMethod)();

	BThreadedTestCaller(std::string name);	
	BThreadedTestCaller(std::string name, TestClass &object);	
	BThreadedTestCaller(std::string name, TestClass *object);	
	virtual ~BThreadedTestCaller();
	
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
};


template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name)
	: TestCase(name)
	, fOwnObject(true)
	, fObject(new TestClass())
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name, TestClass &object)
	: TestCase(name)
	, fOwnObject(false)
	, fObject(&object)
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::BThreadedTestCaller(std::string name, TestClass *object)
	: TestCase(name)
	, fOwnObject(true)
	, fObject(object)
{
}

template <class TestClass, class ExpectedException>
BThreadedTestCaller<TestClass, ExpectedException>::~BThreadedTestCaller() {
	if (fOwnObject)
		delete fObject;		
	for (ThreadManagerMap::iterator it = fThreads.begin(); it != fThreads.end (); ++it) {
		delete it->second;
	}
}

template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::addThread(std::string threadName, ThreadMethod method) {
	if (fThreads.find(threadName) == fThreads.end()) {
		// Unused name, go ahead and add
		fThreads[threadName] = new BThreadManager<TestClass, ExpectedException>(threadName, fObject, method);		
	} else {
		// Duplicate name, throw an exception
		throw CppUnit::Exception("BThreadedTestCaller::addThread() - Attempt to add thread under duplicated name ('"
			+ threadName + "')");
	}
}

template <class TestClass, class ExpectedException>
void
BThreadedTestCaller<TestClass, ExpectedException>::run(CppUnit::TestResult *result) {
	result->startTest(this);

	try {    
		setUp();

		// This try/catch block should never actually have to catch
		// anything (unless some bonehead passes in a NULL pointer to
		// the constructor). Each BThreadManager object catches and
		// handles exceptions for its respective thread, so as not
		// to disrupt the others.
		try {

			// Verify we have a valid object first.
			if (!fObject)
				throw CppUnit::Exception("BThreadedTestCaller::runTest() -- NULL fObject pointer");

			// Launch all the threads.
			for (ThreadManagerMap::iterator i = fThreads.begin();
				   i != fThreads.end ();
	    	         ++i)
	    	{
	    		status_t err = i->second->LaunchThread(result);
				if (err != B_OK)
					result->addError(this, new CppUnit::Exception("Error launching thread '" + i->second->getName() + "'"));
//				printf("Launch(%s)\n", i->second->getName().c_str());
			}
			
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
	return "BThreadedTestCaller for " + getName(); 
}

#endif // _beos_threaded_test_caller_h_
