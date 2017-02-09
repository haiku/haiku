/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
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


BGeolocation::BGeolocation()
	: fService(kDefaultService)
{
}


BGeolocation::BGeolocation(const BUrl& service)
	: fService(service)
{
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

	class GeolocationListener: public BUrlProtocolListener
	{
		public:
			virtual	~GeolocationListener() {};

			void	DataReceived(BUrlRequest*, const char* data, off_t position,
						ssize_t size) {
				result.WriteAt(position, data, size);
			}
			BMallocIO result;
	};

	GeolocationListener listener;

	// Send Request (POST JSON message)
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(fService, &listener);
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

	while (http->IsRunning())
		snooze(10000);

	// Parse reply
	const BHttpResult& reply = (const BHttpResult&)http->Result();
	if (reply.StatusCode() != 200) {
		delete http;
		return B_ERROR;
	}

	BMessage data;
	result = BJson::Parse((char*)listener.result.Buffer(), data);
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
	if (result == B_OK)
		result = location.FindDouble("lng", &lon);

	latitude = lat;
	longitude = lon;

	return result;
}


#ifdef HAVE_DEFAULT_GEOLOCATION_SERVICE_KEY

#include "DefaultGeolocationServiceKey.h"

const char* BGeolocation::kDefaultService
	= "https://location.services.mozilla.com/v1/geolocate?key="
		DEFAULT_GEOLOCATION_SERVICE_KEY;

#else

const char* BGeolocation::kDefaultService = "";

#endif


}	// namespace BPrivate
