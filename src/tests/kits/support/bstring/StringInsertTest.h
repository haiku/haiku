#ifndef StringInsertTest_H
#define StringInsertTest_H

#include "TestCase.h"
#include <String.h>

	
class StringInsertTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringInsertTest(std::string name = "");
	virtual ~StringInsertTest();
	};
	
#endif
