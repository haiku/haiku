#ifndef MallocWriteTest_H
#define MallocWriteTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class MallocWriteTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	MallocWriteTest(std::string name = "");
	virtual ~MallocWriteTest();
	};
	
#endif
