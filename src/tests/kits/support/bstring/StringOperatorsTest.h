#ifndef StringOperatorsTest_H
#define StringOperatorsTest_H

#include "TestCase.h"
#include <String.h>

	
class StringOperatorsTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringOperatorsTest(std::string name = "");
	virtual ~StringOperatorsTest();
};
	
#endif
