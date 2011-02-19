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


typedef std::map<int, BNetworkAddress> AddressMap;
typedef std::map<int, bool> BoolMap;
typedef std::map<int, int> SocketMap;


class NetworkSettings {
public:
								NetworkSettings(const char* name);
	virtual						~NetworkSettings();

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


			const char*			Name()  { return fName.String(); }
			const char*			Domain() { return fDomain.String(); }
			bool				IsDisabled() { return fDisabled; }
			const BString&		WirelessNetwork() { return fWirelessNetwork; }

			BObjectList<BString>& NameServers() { return fNameServers; }

			void				ReadConfiguration();

private:
			SocketMap			fSocket;
			BNetworkInterface	fNetworkInterface;

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
};


#endif /* SETTINGS_H */
