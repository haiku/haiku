#ifndef StringCharAccessTest_H
#define StringCharAccessTest_H

#include "TestCase.h"
#include <String.h>

	
class StringCharAccessTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringCharAccessTest(std::string name = "");
	virtual ~StringCharAccessTest();
	};
	
#endif
