#ifndef StringCountCharTest_H
#define StringCountCharTest_H

#include "TestCase.h"
#include <String.h>

	
class StringCountCharTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringCountCharTest(std::string name = "");
	virtual ~StringCountCharTest();
	};
	
#endif
