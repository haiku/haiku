/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Simon South, simon@simonsouth.net
 */


#include <Keymap.h>

#include <stdio.h>
#include <string.h>

#include <map>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class KeymapTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(KeymapTest);
	CPPUNIT_TEST(TestEquals);
	CPPUNIT_TEST(TestAssignment);
	CPPUNIT_TEST(TestGetChars);
	CPPUNIT_TEST_SUITE_END();

	BKeymap				fCurrentKeymap;
	BKeymap				fDefaultKeymap;
public:
	void
	setUp()
	{
		fCurrentKeymap.SetToCurrent();
		fDefaultKeymap.SetToDefault();
	}

	void TestGetChars()
	{
		// Get a copy of the currently loaded keymap
		key_map* keymap;
		char* charArray;

		get_key_map(&keymap, &charArray);
		CPPUNIT_ASSERT(keymap != NULL);

		// Test each of the keymap's character tables
		typedef std::map<uint32, int32(*)[128]> table_map_t;
		table_map_t tables;
		tables[0] = &keymap->normal_map;
		tables[B_SHIFT_KEY] = &keymap->shift_map;
		tables[B_CAPS_LOCK] = &keymap->caps_map;
		tables[B_CAPS_LOCK | B_SHIFT_KEY] = &keymap->caps_shift_map;
		tables[B_CONTROL_KEY] = &keymap->control_map;
		tables[B_OPTION_KEY] = &keymap->option_map;
		tables[B_OPTION_KEY | B_SHIFT_KEY] = &keymap->option_shift_map;
		tables[B_OPTION_KEY | B_CAPS_LOCK] = &keymap->option_caps_map;
		tables[B_OPTION_KEY | B_SHIFT_KEY | B_CAPS_LOCK] =
			&keymap->option_caps_shift_map;

		for (table_map_t::const_iterator p = tables.begin();
			 p != tables.end(); p++) {
			const uint32 modifiers = (*p).first;
			const int32(*table)[128] = (*p).second;

			// Test, for every keycode, that the result from BKeymap::GetChars()
			// matches what we find in our our own copy of the keymap
			for (uint32 keycode = 0; keycode < 128; keycode++) {
				char* mapChars = &charArray[(*table)[keycode]];

				// If the keycode isn't mapped, try again without the Option key
				if (*mapChars <= 0 && (modifiers & B_OPTION_KEY) != 0) {
					int newOffset = (*tables[modifiers & ~B_OPTION_KEY])[keycode];
					if (newOffset >= 0)
						mapChars = &charArray[newOffset];
				}

				char* chars;
				int32 numBytes;
				fCurrentKeymap.GetChars(keycode, modifiers, 0, &chars, &numBytes);

				CPPUNIT_ASSERT(*mapChars <= 0 || chars != NULL);
				CPPUNIT_ASSERT_EQUAL((int32)*mapChars, numBytes);
				if (*mapChars > 0)
					CPPUNIT_ASSERT(strncmp(chars, mapChars + 1, numBytes) == 0);
			}
		}

		delete keymap;
		delete[] charArray;
	}

	void TestAssignment()
	{
		BKeymap keymap;

		keymap = fCurrentKeymap;
		CPPUNIT_ASSERT(keymap == fCurrentKeymap);

		keymap = fDefaultKeymap;
		CPPUNIT_ASSERT(keymap == fDefaultKeymap);
	}

	void TestEquals()
	{
		CPPUNIT_ASSERT(fCurrentKeymap == fCurrentKeymap);
		CPPUNIT_ASSERT(fDefaultKeymap == fDefaultKeymap);

		BKeymap keymap;

		keymap.SetToCurrent();
		CPPUNIT_ASSERT(keymap == fCurrentKeymap);

		keymap.SetToDefault();
		CPPUNIT_ASSERT(keymap == fDefaultKeymap);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(KeymapTest, getTestSuiteName());
