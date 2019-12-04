/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <Geolocation.h>

#include <HttpRequest.h>
#include <Json.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <String.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>


namespace BPrivate {


class GeolocationListener: public BUrlProtocolListener
{
	public:
		GeolocationListener()
		{
			pthread_cond_init(&fCompletion, NULL);
			pthread_mutex_init(&fLock, NULL);
		}

		virtual	~GeolocationListener() {
			pthread_cond_destroy(&fCompletion);
			pthread_mutex_destroy(&fLock);
		}

		void ConnectionOpened(BUrlRequest* caller)
		{
			pthread_mutex_lock(&fLock);
		}

		void DataReceived(BUrlRequest*, const char* data, off_t position,
					ssize_t size) {
			fResult.WriteAt(position, data, size);
		}

		void RequestCompleted(BUrlRequest* caller, bool success) {
			pthread_cond_signal(&fCompletion);
			pthread_mutex_unlock(&fLock);
		}

		BMallocIO fResult;
		pthread_cond_t fCompletion;
		pthread_mutex_t fLock;
};


BGeolocation::BGeolocation()
	: fGeolocationService(kDefaultGeolocationService),
	fGeocodingService(kDefaultGeocodingService)
{
}


BGeolocation::BGeolocation(const BUrl& geolocationService,
	const BUrl& geocodingService)
	: fGeolocationService(geolocationService),
	fGeocodingService(geocodingService)
{
	if (!fGeolocationService.IsValid())
		fGeolocationService.SetUrlString(kDefaultGeolocationService);
	if (!fGeocodingService.IsValid())
		fGeocodingService.SetUrlString(kDefaultGeocodingService);
}


status_t
BGeolocation::LocateSelf(float& latitude, float& longitude)
{
	// Enumerate wifi network and build JSON message
	BNetworkRoster& roster = BNetworkRoster::Default();
	uint32 interfaceCookie = 0;
	BNetworkInterface interface;

	BString query("{\n\t\"wifiAccessPoints\": [");
	int32 count = 0;

	while (roster.GetNextInterface(&interfaceCookie, interface) == B_OK) {
		uint32 networkCookie = 0;
		wireless_network network;

		BNetworkDevice device(interface.Name());
			// TODO is that the correct way to enumerate devices?

		while (device.GetNextNetwork(networkCookie, network) == B_OK) {
			if (count != 0)
				query += ',';

			count++;

			query += "\n\t\t{ \"macAddress\": \"";
			query += network.address.ToString().ToUpper();
			query += "\", \"signalStrength\": ";
			query << (int)network.signal_strength;
			query += ", \"signalToNoiseRatio\": ";
			query << (int)network.noise_level;
			query += " }";
		}

	}

	query += "\n\t]\n}\n";

	// Check that we have enough data (we need at least 2 networks)
	if (count < 2)
		return B_DEVICE_NOT_FOUND;

	GeolocationListener listener;

	// Send Request (POST JSON message)
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(fGeolocationService,
		&listener);
	if (request == NULL)
		return B_BAD_DATA;

	BHttpRequest* http = dynamic_cast<BHttpRequest*>(request);
	if (http == NULL) {
		delete request;
		return B_BAD_DATA;
	}

	http->SetMethod(B_HTTP_POST);
	BMemoryIO* io = new BMemoryIO(query.String(), query.Length());
	http->AdoptInputData(io, query.Length());

	status_t result = http->Run();
	if (result < 0) {
		delete http;
		return result;
	}

	pthread_mutex_lock(&listener.fLock);
	while (http->IsRunning())
		pthread_cond_wait(&listener.fCompletion, &listener.fLock);
	pthread_mutex_unlock(&listener.fLock);

	// Parse reply
	const BHttpResult& reply = (const BHttpResult&)http->Result();
	if (reply.StatusCode() != 200) {
		delete http;
		return B_ERROR;
	}

	BMessage data;
	result = BJson::Parse((char*)listener.fResult.Buffer(), data);
	delete http;
	if (result != B_OK) {
		return result;
	}

	BMessage location;
	result = data.FindMessage("location", &location);
	if (result != B_OK)
		return result;

	double lat, lon;
	result = location.FindDouble("lat", &lat);
	if (result != B_OK)
		return result;
	result = location.FindDouble("lng", &lon);
	if (result != B_OK)
		return result;

	latitude = lat;
	longitude = lon;

	return result;
}


status_t
BGeolocation::Country(const float latitude, const float longitude,
	BCountry& country)
{
	// Prepare the request URL
	BUrl url(fGeocodingService);
	BString requestString;
	requestString.SetToFormat("%s&lat=%f&lng=%f", url.Request().String(), latitude,
		longitude);
	url.SetPath("/countryCode");
	url.SetRequest(requestString);

	GeolocationListener listener;
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url,
		&listener);
	if (request == NULL)
		return B_BAD_DATA;

	BHttpRequest* http = dynamic_cast<BHttpRequest*>(request);
	if (http == NULL) {
		delete request;
		return B_BAD_DATA;
	}

	status_t result = http->Run();
	if (result < 0) {
		delete http;
		return result;
	}

	pthread_mutex_lock(&listener.fLock);
	while (http->IsRunning()) {
		pthread_cond_wait(&listener.fCompletion, &listener.fLock);
	}
	pthread_mutex_unlock(&listener.fLock);

	// Parse reply
	const BHttpResult& reply = (const BHttpResult&)http->Result();
	if (reply.StatusCode() != 200) {
		delete http;
		return B_ERROR;
	}

	off_t length = 0;
	listener.fResult.GetSize(&length);
	length -= 2; // Remove \r\n from response
	BString countryCode((char*)listener.fResult.Buffer(), (int32)length);
	return country.SetTo(countryCode);
}


#ifdef HAVE_DEFAULT_GEOLOCATION_SERVICE_KEY

#include "DefaultGeolocationServiceKey.h"

const char* BGeolocation::kDefaultGeolocationService
	= "https://location.services.mozilla.com/v1/geolocate?key="
		DEFAULT_GEOLOCATION_SERVICE_KEY;

const char* BGeolocation::kDefaultGeocodingService
	= "https://secure.geonames.org/?username="
		DEFAULT_GEOCODING_SERVICE_KEY;

#else

const char* BGeolocation::kDefaultGeolocationService = "";
const char* BGeolocation::kDefaultGeocodingService = "";

#endif


}	// namespace BPrivate
