#ifndef StringAssignTest_H
#define StringAssignTest_H

#include "TestCase.h"
#include <String.h>

	
class StringAssignTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringAssignTest(std::string name = "");
	virtual ~StringAssignTest();
};
	
#endif
