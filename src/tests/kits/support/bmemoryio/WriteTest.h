#ifndef WriteTest_H
#define WriteTest_H

#include "TestCase.h"
#include <DataIO.h>

	
class WriteTest : public BTestCase
{
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	WriteTest(std::string name = "");
	virtual ~WriteTest();
	};
	
#endif
