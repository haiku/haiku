/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef _PTP_SETTINGS__H
#define _PTP_SETTINGS__H

#include <Message.h>
#include <String.h>


class PTPSettings {
	public:
		PTPSettings();
		~PTPSettings();
		
		BMessage& Addons()
			{ return fAddons; }
		const BString& CurrentInterface() const
			{ return fCurrent; }
		
		bool SetDefaultInterface(const char *name);
		const char *DefaultInterface() const
			{ return fDefaultInterface; }
		
		bool GetPTPDirectories(BDirectory *settingsDirectory,
			BDirectory *profileDirectory) const;
		
		bool LoadSettings(const char *interfaceName, bool isNew);
		void IsModified(bool *settings, bool *profile);
		bool SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary);
		bool SaveSettingsToFile();
		
		void LoadAddons(bool loadGeneralAddon = true);
			// must be called manually

	private:
		BMessage fAddons, fSettings, fProfile;
		BString fCurrent;
		char *fDefaultInterface;
};


#endif
