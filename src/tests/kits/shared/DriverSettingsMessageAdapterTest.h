/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVER_SETTINGS_MESSAGE_ADAPTER_TEST_H
#define DRIVER_SETTINGS_MESSAGE_ADAPTER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class DriverSettingsMessageAdapterTest : public CppUnit::TestCase {
public:
								DriverSettingsMessageAdapterTest();
	virtual						~DriverSettingsMessageAdapterTest();

			void				TestPrimitivesToMessage();
			void				TestMessage();
			void				TestParent();
			void				TestConverter();
			void				TestWildcard();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// DRIVER_SETTINGS_MESSAGE_ADAPTER_TEST_H
