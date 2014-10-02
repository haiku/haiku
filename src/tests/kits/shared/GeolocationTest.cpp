/*
 * Copyright 2014, Haiku, inc.
 * Distributed under the terms of the MIT licence
 */


#include "GeolocationTest.h"

#include <Geolocation.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


using namespace BPrivate;


GeolocationTest::GeolocationTest()
{
}


void
GeolocationTest::TestLocateSelf()
{
	BGeolocation locator(BUrl(
		"https://location.services.mozilla.com/v1/geolocate?key=test"));
	float latitude, longitude;
	status_t result = locator.LocateSelf(latitude, longitude);

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	printf("Your position is: %f %f\n", latitude, longitude);
}


/* static */ void
GeolocationTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("GeolocationTest");

	suite.addTest(new CppUnit::TestCaller<GeolocationTest>(
		"GeolocationTest::LocateSelf", &GeolocationTest::TestLocateSelf));

	parent.addTest("GeolocationTest", &suite);
};
