#ifndef StringAppendTest_H
#define StringAppendTest_H

#include "TestCase.h"
#include <String.h>

	
class StringAppendTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringAppendTest(std::string name = "");
	virtual ~StringAppendTest();
	};
	
#endif
