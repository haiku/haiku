#ifndef MallocSeekTest_H
#define MallocSeekTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class MallocSeekTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	MallocSeekTest(std::string name = "");
	virtual ~MallocSeekTest();
	};
	
#endif
