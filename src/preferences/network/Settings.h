/*
 * Copyright 2004-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Vegard Wærp, vegarwa@online.no
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <ObjectList.h>
#include <String.h>


class Settings {
public:
								Settings(const char* name);
	virtual						~Settings();

			void				SetIP(const BString& ip)
									{ fIP = ip; }
			void				SetGateway(const BString& ip)
									{ fGateway = ip; }
			void				SetNetmask(const BString& ip)
									{ fNetmask = ip; }
			void				SetDomain(const BString& domain)
									{ fDomain = domain; }
			void				SetAutoConfigure(bool autoConfigure)
									{ fAuto = autoConfigure; }
			void				SetDisabled(bool disabled)
									{ fDisabled = disabled; }
			void				SetWirelessNetwork(const char* name)
									{ fWirelessNetwork.SetTo(name); }

			const char*			IP()  { return fIP.String(); }
			const char*			Gateway()  { return fGateway.String(); }
			const char*			Netmask()  { return fNetmask.String(); }
			const char*			Name()  { return fName.String(); }
			const char*			Domain() { return fDomain.String(); }
			bool				AutoConfigure() { return fAuto; }
			bool				IsDisabled() { return fDisabled; }
			const BString&		WirelessNetwork() { return fWirelessNetwork; }

			BObjectList<BString>& NameServers() { return fNameServers; }

			void				ReadConfiguration();

private:
			bool				_PrepareRequest(struct ifreq& request);

			BString				fIP;
			BString				fGateway;
			BString				fNetmask;
			BString				fName;
			BString				fDomain;
			int					fSocket;
			bool				fAuto;
			bool				fDisabled;
			BObjectList<BString> fNameServers;
			BString				fWirelessNetwork;
};


#endif /* SETTINGS_H */
