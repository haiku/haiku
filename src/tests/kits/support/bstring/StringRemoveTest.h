#ifndef StringRemoveTest_H
#define StringRemoveTest_H

#include "TestCase.h"
#include <String.h>

	
class StringRemoveTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringRemoveTest(std::string name = "");
	virtual ~StringRemoveTest();
	};
	
#endif
