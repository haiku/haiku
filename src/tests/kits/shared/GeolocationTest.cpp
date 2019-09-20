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
		"https://location.services.mozilla.com/v1/geolocate?key=test"), BUrl());
	float latitude, longitude;
	status_t result = locator.LocateSelf(latitude, longitude);

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	printf("Your position is: %f %f\n", latitude, longitude);
}


void
GeolocationTest::TestCountry()
{
	BGeolocation geocoder(BUrl(""), BUrl("https://secure.geonames.org/?username=demo"));
	BCountry country;
	status_t result = geocoder.Country(47.03f, 10.2f, country);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(B_OK, country.InitCheck());
	CPPUNIT_ASSERT_EQUAL(BString("AT"), BString(country.Code()));
}


/* static */ void
GeolocationTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("GeolocationTest");

	suite.addTest(new CppUnit::TestCaller<GeolocationTest>(
		"GeolocationTest::LocateSelf", &GeolocationTest::TestLocateSelf));
	suite.addTest(new CppUnit::TestCaller<GeolocationTest>(
		"GeolocationTest::Country", &GeolocationTest::TestCountry));

	parent.addTest("GeolocationTest", &suite);
};
