#ifndef StringReplaceTest_H
#define StringReplaceTest_H

#include "TestCase.h"
#include <String.h>

	
class StringReplaceTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringReplaceTest(std::string name = "");
	virtual ~StringReplaceTest();
	};
	
#endif
