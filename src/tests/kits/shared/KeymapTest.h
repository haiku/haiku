/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Simon South, simon@simonsouth.net
 */
#ifndef KEYMAP_TEST_H
#define KEYMAP_TEST_H


#include <Keymap.h>

#include <TestCase.h>
#include <TestSuite.h>


class KeymapTest : public CppUnit::TestCase {
public:
								KeymapTest();
	virtual					   ~KeymapTest();

			void				TestEquals();
			void				TestAssignment();
			void				TestGetChars();

	static	void				AddTests(BTestSuite& suite);

private:
			BKeymap				fCurrentKeymap;
			BKeymap				fDefaultKeymap;
};


#endif	// KEYMAP_TEST
