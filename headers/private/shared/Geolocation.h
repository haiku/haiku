/*
 * Copyright 2014-2019, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GEOLOCATION_H
#define _GEOLOCATION_H


#include <Country.h>
#include <Url.h>


namespace BPrivate {


class BGeolocation {
public:
				BGeolocation();
				BGeolocation(const BUrl& geolocationService,
					const BUrl& geocodingService);

	status_t	LocateSelf(float& latitude, float& longitude);
	status_t	Locate(const BString placeName, float& latitude,
					float& longitude);

	status_t	Name(const float latitude, const float longitude,
					BString& name);
	status_t	Country(const float latitude, const float longitude,
					BCountry& country);

private:
					BUrl	fGeolocationService;
					BUrl	fGeocodingService;

	static const	char*	kDefaultGeolocationService;
	static const	char*	kDefaultGeocodingService;
};


}	// namespace BPrivate


#endif
