#ifndef StringPrependTest_H
#define StringPrependTest_H

#include "TestCase.h"
#include <String.h>

	
class StringPrependTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringPrependTest(std::string name = "");
	virtual ~StringPrependTest();
	};
	
#endif
