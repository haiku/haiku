#ifndef MallocBufferLengthTest_H
#define MallocBufferLengthTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class MallocBufferLengthTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	MallocBufferLengthTest(std::string name = "");
	virtual ~MallocBufferLengthTest();
	};
	
#endif
