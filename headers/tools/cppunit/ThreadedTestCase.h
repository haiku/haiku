#ifndef _beos_threaded_test_case_h_
#define _beos_threaded_test_case_h_

#include <Locker.h>
#include <kernel/OS.h>
#include <TestCase.h>
#include <map>
#include <string>

//! Base class for single threaded unit tests
class BThreadedTestCase : public BTestCase {
public:
	BThreadedTestCase(std::string Name = "", std::string progressSeparator = ".");
	virtual ~BThreadedTestCase();
	
	/*! \brief Displays the next sub test progress indicator for the
		thread in which it's called (i.e. [A.0][B.0][A.1][A.2][B.1]...). */
	virtual void NextSubTest();
	
	//! Saves the location of the current working directory. 
	void SaveCWD();
	
	//! Restores the current working directory to last directory saved by a	call to SaveCWD().	
	void RestoreCWD(const char *alternate = NULL);
	void InitThreadInfo(thread_id id, std::string threadName);
protected:
//	friend class ThreadManager<BThreadedTestCase>;
	std::string fProgressSeparator;

	struct ThreadSubTestInfo {
		std::string name;
		int32 subTestNum;	
	};
	std::map<thread_id, ThreadSubTestInfo*> fNumberMap;
	BLocker *fNumberMapLock;
};

#endif // _beos_threaded_test_case_h_
