/*
 * Copyright 2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KPATH_TEST_H
#define KPATH_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class KPathTest : public CppUnit::TestCase {
public:
								KPathTest();
	virtual						~KPathTest();

			void				TestSetToAndPath();
			void				TestLeaf();
			void				TestReplaceLeaf();
			void				TestRemoveLeaf();
			void				TestAdopt();
			void				TestDetachBuffer();
			void				TestNormalize();
			void				TestAssign();
			void				TestEquals();
			void				TestNotEquals();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// KPATH_TEST_H
