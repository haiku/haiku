/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef _PTP_SETTINGS__H
#define _PTP_SETTINGS__H

#include <Message.h>
#include <String.h>


class GeneralAddon;


class PTPSettings {
	public:
		PTPSettings();
		~PTPSettings();
		
		BMessage& Addons()
			{ return fAddons; }
		const BString& CurrentInterface() const
			{ return fCurrent; }
		const char *SessionPassword() const;
		
		bool LoadSettings(const char *interfaceName, bool isNew);
		void IsModified(bool *settings);
		bool SaveSettings(BMessage *settings);
		bool SaveSettingsToFile();
		
		void LoadAddons();
			// must be called manually

	private:
		BMessage fAddons, fSettings;
		BString fCurrent;
		GeneralAddon *fGeneralAddon;
};


#endif
