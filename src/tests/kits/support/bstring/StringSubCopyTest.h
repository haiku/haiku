#ifndef StringSubCopyTest_H
#define StringSubCopyTest_H

#include "TestCase.h"
#include <String.h>

	
class StringSubCopyTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringSubCopyTest(std::string name = "");
	virtual ~StringSubCopyTest();
	};
	
#endif
