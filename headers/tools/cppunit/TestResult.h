#ifndef _beos_test_result_h_
#define _beos_test_result_h_

#include <cppunit/TestResult.h>

/*! \brief CppUnit::TestResult class with BeOS synchronization implementation.

*/
//template <class SynchronizationObject>
class TestResult : public CppUnit::TestResult {
public:
  TestResult();  
protected:
};

#endif // _beos_test_result_h_


