/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef GEOLOCATION_TEST_H
#define GEOLOCAITON_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class GeolocationTest: public BTestCase {
public:
					GeolocationTest();

			void	TestLocateSelf();
			void	TestCountry();

	static	void	AddTests(BTestSuite& suite);
};


#endif
