#ifndef StringConstructionTest_H
#define StringConstructionTest_H

#include "TestCase.h"
#include <String.h>

	
class StringConstructionTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringConstructionTest(std::string name = "");
	virtual ~StringConstructionTest();
	};
	
#endif
