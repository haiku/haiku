#ifndef ConstTest_H
#define ConstTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class ConstTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	ConstTest(std::string name = "");
	virtual ~ConstTest();
	};
	
#endif
