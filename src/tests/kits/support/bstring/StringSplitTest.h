#ifndef StringSplitTest_H
#define StringSplitTest_H

#include "TestCase.h"
#include <String.h>


class StringSplitTest : public BTestCase
{

private:

protected:

public:
	static Test *suite(void);
	void PerformTest(void);
	StringSplitTest(std::string name = "");
	virtual ~StringSplitTest();
	};

#endif
