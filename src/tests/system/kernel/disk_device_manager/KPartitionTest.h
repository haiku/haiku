/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KPARTITION_TEST_H_
#define _KPARTITION_TEST_H_

#include <TestCase.h>


class KPartitionGetMountPointTest : public BTestCase {
	public:
		KPartitionGetMountPointTest(std::string name = "");

		static CppUnit::Test *Suite();

		void TestPartitionWithoutFilesystemReturnsBadValue();
		void TestPartitionContentNameUsedFirst();
		void TestPartitionNameUsedSecond();
		void TestPartitionWithoutAnyNameIsNotRoot();
		void TestPartitionNameWithSlashesRemoved();
		void TestPartitionMountPointExists();
};

#endif	/* _KPARTITION_TEST_H_ */
