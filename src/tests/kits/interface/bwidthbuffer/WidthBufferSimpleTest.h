#ifndef __WidthBufferSIMPLETest_H
#define __WidthBufferSIMPLETest_H

#include "TestCase.h"
#include <WidthBuffer.h>

class WidthBufferSimpleTest : public BTestCase
{
public:
	static Test *suite(void);
	void PerformTest(void);
	WidthBufferSimpleTest(std::string name = "");
	virtual ~WidthBufferSimpleTest();

private:
	_BWidthBuffer_ *fWidthBuffer;
};
	
#endif
