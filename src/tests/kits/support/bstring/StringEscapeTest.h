#ifndef StringEscapeTest_H
#define StringEscapeTest_H

#include "TestCase.h"
#include <String.h>

	
class StringEscapeTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringEscapeTest(std::string name = "");
	virtual ~StringEscapeTest();
	};
	
#endif
