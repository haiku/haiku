#ifndef StringSearchTest_H
#define StringSearchTest_H

#include "TestCase.h"
#include <String.h>

	
class StringSearchTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	StringSearchTest(std::string name = "");
	virtual ~StringSearchTest();
	};
	
#endif
