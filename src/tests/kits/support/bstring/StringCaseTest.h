#ifndef StringCaseTest_H
#define StringCaseTest_H

#include "TestCase.h"
#include <String.h>

	
class StringCaseTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringCaseTest(std::string name = "");
	virtual ~StringCaseTest();
	};
	
#endif
