#ifndef _udf_allocator_test_h_
#define _udf_allocator_test_h_

#include <ThreadedTestCase.h>
#include <Locker.h>

class AllocatorTest : public BTestCase {
public:
	AllocatorTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void BlockSizeTest();
	void PartitionFullTest();
};

#endif // _udf_allocator_test_h_
