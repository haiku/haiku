#ifndef ReadTest_H
#define ReadTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class ReadTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	ReadTest(std::string name = "");
	virtual ~ReadTest();
	};
	
#endif
