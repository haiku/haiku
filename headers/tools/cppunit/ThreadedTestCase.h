#ifndef _beos_threaded_test_case_h_
#define _beos_threaded_test_case_h_

#include <Locker.h>
#include <OS.h>
#include <TestCase.h>
#include <map>
#include <string>
#include <vector>

//! Base class for single threaded unit tests
class CPPUNIT_API BThreadedTestCase : public BTestCase {
public:
	BThreadedTestCase(string Name = "", string progressSeparator = ".");
	virtual ~BThreadedTestCase();
	
	/*! \brief Displays the next sub test progress indicator for the
		thread in which it's called (i.e. [A.0][B.0][A.1][A.2][B.1]...). */
	virtual void NextSubTest();
	
	/*! \brief Prints to standard out just like printf, except shell verbosity
		settings are honored, and output from threads other than the main thread
		happens before the test is over.
		
		\note Currently your output is limited to a length of 1024 characters. If
		you really need to print a single string that's long than that, fix the
		function yourself :-).
	*/
	virtual void Outputf(const char *str, ...);
	
	//! Saves the location of the current working directory. 
	void SaveCWD();
	
	//! Restores the current working directory to last directory saved by a	call to SaveCWD().	
	void RestoreCWD(const char *alternate = NULL);
	void InitThreadInfo(thread_id id, string threadName);
	bool RegisterForUse();
	void UnregisterForUse();
	
	vector<string>& AcquireUpdateList();
	void ReleaseUpdateList();
protected:
	bool fInUse;

//	friend class ThreadManager<BThreadedTestCase>;
	string fProgressSeparator;

	struct ThreadSubTestInfo {
		string name;
		int32 subTestNum;	
	};
	map<thread_id, ThreadSubTestInfo*> fNumberMap;
	vector<string> fUpdateList;
	BLocker *fUpdateLock;

};

#endif // _beos_threaded_test_case_h_
