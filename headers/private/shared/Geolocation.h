/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GEOLOCATION_H
#define _GEOLOCATION_H


#include <Url.h>


namespace BPrivate {


class BGeolocation {
public:
				BGeolocation();
				BGeolocation(const BUrl& service);

	status_t	LocateSelf(float& latitude, float& longitude);
	status_t	Locate(const BString placeName, float& latitude,
					float& longitude);
	status_t	Name(const float latitude, const float longitude,
					BString& name);

private:
					BUrl	fService;
	static const	char*	kDefaultService;
};


}	// namespace BPrivate


#endif
