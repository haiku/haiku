#ifndef StringCompareTest_H
#define StringCompareTest_H

#include "TestCase.h"
#include <String.h>

	
class StringCompareTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringCompareTest(std::string name = "");
	virtual ~StringCompareTest();
	};
	
#endif
