#ifndef SetSizeTest_H
#define SetSizeTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class SetSizeTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	SetSizeTest(std::string name = "");
	virtual ~SetSizeTest();
	};
	
#endif
