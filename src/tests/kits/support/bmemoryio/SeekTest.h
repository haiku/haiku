#ifndef SeekTest_H
#define SeekTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class SeekTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	SeekTest(std::string name = "");
	virtual ~SeekTest();
	};
	
#endif
