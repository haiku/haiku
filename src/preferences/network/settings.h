/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
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
		virtual ~Settings();

		void SetName(BString name);
		void SetIP(BString ip);
		void SetGateway(BString ip);
		void SetNetmask(BString ip);
		void SetAutoConfigure(bool t);
		
		const char* GetIP()  {return fIP.String(); }
		const char* GetGateway()  {return fGateway.String(); }		
		const char* GetNetmask()  {return fNetmask.String(); }
		const char* GetName()  {return fName.String(); }
		bool GetAutoConfigure() const {return fAuto; }
		BObjectList<BString> fNameservers;

		void SaveProfile(BString profileName);
		void LoadProfile(BString profileName);
		void SaveConfiguration();
		void ReadConfiguration();
		
	private:
		bool _PrepareRequest(struct ifreq& request);
		BString fIP;
		BString fGateway;
		BString fNetmask;
		BString fName;
		int fSocket;
		bool fAuto;
};

#endif /* SETTINGS_H */
