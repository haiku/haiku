/*
 * Copyright 2004-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Vegard Wærp, vegarwa@online.no
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <ObjectList.h>
#include <String.h>

#include <map>


#define MAX_PROTOCOLS	7
	// maximum number of protocols that could be configured


typedef std::map<int, BNetworkAddress> AddressMap;
typedef std::map<int, BNetworkInterfaceAddress> InterfaceAddressMap;
typedef std::map<int, bool> BoolMap;

typedef struct _protocols {
	const char* name;
		// Eg. "IPv4", used for user descriptions
	bool present;
		// Does the OS have the proper addon?
	int inet_id;
		// Eg. AF_INET
	int socket_id;
		// Socket ID used for ioctls to this proto
} protocols;


class NetworkSettings {
public:
								NetworkSettings(const char* name);
	virtual						~NetworkSettings();

			protocols*			ProtocolVersions()
									{ return fProtocols; }

			void				SetIP(int family, const char* ip)
									{ fAddress[family].SetTo(ip); }
			void				SetNetmask(int family, const char* mask)
									{ fNetmask[family].SetTo(mask); }
			void				SetGateway(int family, const char* ip)
									{ fGateway[family].SetTo(ip); }
			void				SetAutoConfigure(int family, bool autoConf)
									{ fAutoConfigure[family] = autoConf; }

			void				SetDisabled(bool disabled)
									{ fDisabled = disabled; }

//			void				SetWirelessNetwork(const char* name)
//									{ fWirelessNetwork.SetTo(name); }
//			void				SetDomain(const BString& domain)
//									{ fDomain = domain; }


			bool				AutoConfigure(int family)
									{ return fAutoConfigure[family]; }
			BNetworkAddress		IPAddr(int family)
									{ return fAddress[family]; }
			const char*			IP(int family)
									{ return fAddress[family].ToString(); }
			const char*			Netmask(int family)
									{ return fNetmask[family].ToString(); }
			const char*			Gateway(int family)
									{ return fGateway[family].ToString(); }
			int32				PrefixLen(int family)
									{ return fNetmask[family].PrefixLength(); }

			status_t			RenegotiateAddresses();

			const char*			Name()  { return fName.String(); }
			const char*			Domain() { return fDomain.String(); }
			bool				IsDisabled() { return fDisabled; }

			bool				IsWireless() {
									return fNetworkDevice->IsWireless(); }
			bool				IsEthernet() {
									return fNetworkDevice->IsEthernet(); }
			bool				HasLink() {
									return fNetworkDevice->HasLink(); }

			const BString&		WirelessNetwork() { return fWirelessNetwork; }

			BObjectList<BString>& NameServers() { return fNameServers; }

			/*	Grab and write interface settings directly from interface */
			void				ReadConfiguration();
			void				SetConfiguration();

			/*	Grab and write interface settings from interface configuration
				file and let NetServer sort it out */
			void				LoadConfiguration();
			void				WriteConfiguration();

private:
			status_t			_DetectProtocols();

			// Stored network addresses and paramaters
			BoolMap				fAutoConfigure;
			AddressMap			fAddress;
			AddressMap			fNetmask;
			AddressMap			fGateway;

			BString				fName;
			BString				fDomain;
			bool				fDisabled;
			BObjectList<BString> fNameServers;
			BString				fWirelessNetwork;

			protocols			fProtocols[MAX_PROTOCOLS];

			BNetworkInterface*	fNetworkInterface;
			BNetworkDevice*		fNetworkDevice;

			InterfaceAddressMap	fInterfaceAddressMap;
};


#endif /* SETTINGS_H */
