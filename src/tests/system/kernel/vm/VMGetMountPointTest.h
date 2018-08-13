/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _VM_GET_MOUNT_POINT_TEST_H_
#define _VM_GET_MOUNT_POINT_TEST_H_

#include <TestCase.h>


class VMGetMountPointTest : public BTestCase {
	public:
		VMGetMountPointTest(std::string name = "");

		static CppUnit::Test *Suite();

		void TestNullMountPointReturnsBadValue();
		void TestPartitionWithoutFilesystemReturnsBadValue();
		void TestPartitionContentNameUsedFirst();
		void TestPartitionNameUsedSecond();
		void TestPartitionWithoutAnyNameIsNotRoot();
		void TestPartitionNameWithSlashesRemoved();
		void TestPartitionMountPointExists();
};

#endif	/* _VM_GET_MOUNT_POINT_TEST_H_ */
