#include <ThreadedTestCaller.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <iostream>
#include <kernel/OS.h>
#include <TestUtils.h>

#include "AllocatorTest.h"

#include "Allocator.h"
#include "PhysicalPartitionAllocator.h"

AllocatorTest::AllocatorTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
AllocatorTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Yo");
	
	// And our tests
	suite->addTest(new CppUnit::TestCaller<AllocatorTest>("Udf::Allocator::BlockSize Test", &AllocatorTest::BlockSizeTest));
	suite->addTest(new CppUnit::TestCaller<AllocatorTest>("Udf::Allocator::Partition Full Test", &AllocatorTest::PartitionFullTest));
	
	return suite;
}

void
AllocatorTest::BlockSizeTest() {
	{
		Allocator allocator(0);
		CHK(allocator.InitCheck() != B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(1);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(2);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(3);
		CHK(allocator.InitCheck() != B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(256);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(512);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(1024);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(1025);
		CHK(allocator.InitCheck() != B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(2048);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
	{
		Allocator allocator(4096);
		CHK(allocator.InitCheck() == B_OK);
		NextSubTest();
	}
}

void
AllocatorTest::PartitionFullTest() {
	{
		Udf::extent_address extent;
		const uint32 blockSize = 2048;
		Allocator allocator(blockSize);
		CHK(allocator.InitCheck() == B_OK);
		CHK(allocator.GetNextExtent(blockSize*12, true, extent) == B_OK); // 0-11
		extent.set_location(14);
		extent.set_length(1);
		CHK(allocator.GetExtent(extent) == B_OK); // 14
		CHK(allocator.GetNextExtent(blockSize*3, true, extent) == B_OK); // 15-17
		CHK(allocator.GetNextExtent(1, true, extent) == B_OK); // 12
		CHK(allocator.GetBlock(12) != B_OK);
		CHK(allocator.GetBlock(13) == B_OK);
    }
	NextSubTest();
	{
		Udf::extent_address extent;
		Allocator allocator(2048);
		CHK(allocator.InitCheck() == B_OK);
		CHK(allocator.GetNextExtent(ULONG_MAX-1, true, extent) == B_OK);
		CHK(allocator.GetNextExtent(1, true, extent) != B_OK);
		extent.set_location(256);
		extent.set_length(1);
		CHK(allocator.GetExtent(extent) != B_OK);
		CHK(allocator.GetBlock(13) != B_OK);
    }
	NextSubTest();
	{
		Udf::extent_address extent;
		const uint32 blockSize = 2048;
		Allocator allocator(blockSize);
		CHK(allocator.InitCheck() == B_OK);
		CHK(allocator.GetNextExtent(ULONG_MAX-blockSize*2, true, extent) == B_OK);
		CHK(allocator.GetNextExtent(1, true, extent) == B_OK);
		extent.set_location(256);
		extent.set_length(1);
		CHK(allocator.GetExtent(extent) != B_OK);
		CHK(allocator.GetBlock(13) != B_OK);
		PhysicalPartitionAllocator partition(0, 0, allocator);
		std::list<Udf::long_address> extents;
    	std::list<Udf::extent_address> physicalExtents;
    	CHK(partition.GetNextExtents(1, extents, physicalExtents) == B_OK);
    }
	NextSubTest();
	{
		Udf::extent_address extent;
		const uint32 blockSize = 2048;
		Allocator allocator(blockSize);
		CHK(allocator.InitCheck() == B_OK);
		CHK(allocator.GetNextExtent(ULONG_MAX, true, extent) == B_OK);
		PhysicalPartitionAllocator partition(0, 0, allocator);
		std::list<Udf::long_address> extents;
    	std::list<Udf::extent_address> physicalExtents;
    	CHK(partition.GetNextExtents(1, extents, physicalExtents) != B_OK);
    }
	NextSubTest();
}

