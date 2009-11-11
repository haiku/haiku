/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <ObjectList.h>
#include <String.h>


class Settings {
public:
								Settings(const char* name);
	virtual						~Settings();

			void				SetIP(BString ip) { fIP = ip; }
			void				SetGateway(BString ip) { fGateway = ip; }
			void				SetNetmask(BString ip) { fNetmask = ip; }
			void				SetAutoConfigure(bool autoConfigure)
									{ fAuto = autoConfigure; }

			const char*			GetIP()  { return fIP.String(); }
			const char*			GetGateway()  { return fGateway.String(); }
			const char*			GetNetmask()  { return fNetmask.String(); }
			const char*			GetName()  { return fName.String(); }
			bool				AutoConfigure() { return fAuto; }

			BObjectList<BString>& NameServers() { return fNameServers; }

			void				ReadConfiguration();

private:
			bool				_PrepareRequest(struct ifreq& request);

			BString				fIP;
			BString				fGateway;
			BString				fNetmask;
			BString				fName;
			int					fSocket;
			bool				fAuto;
			BObjectList<BString> fNameServers;
};

#endif /* SETTINGS_H */
