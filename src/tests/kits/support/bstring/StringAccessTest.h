#ifndef StringAccessTest_H
#define StringAccessTest_H

#include "TestCase.h"
#include <String.h>

	
class StringAccessTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringAccessTest(std::string name = "");
	virtual ~StringAccessTest();
	};
	
#endif
