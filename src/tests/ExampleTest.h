#ifndef _example_test_h_
#define _example_test_h_

#include <ThreadedTestCase.h>
#include <Locker.h>

class ExampleTest : public BThreadedTestCase {
public:
	ExampleTest(std::string name = "");
	virtual ~ExampleTest() { delete fLocker; }

	static Test* Suite();
	
	void TestFunc1();	// num += 10
	void TestFunc2();	// num *= 2
	void TestFunc3();	// num -= 5
protected:
	BLocker *fLocker;
	int fNum;
};

#endif // _example_test_h_
