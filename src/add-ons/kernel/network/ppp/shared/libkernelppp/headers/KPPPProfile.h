//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_PROFILE__H
#define _K_PPP_PROFILE__H

#include <KPPPDefs.h>

class KPPPInterface;


class KPPPProfile {
		friend class KPPPInterface;

	private:
		KPPPProfile(KPPPInterface& interface);
		~KPPPProfile();
		void LoadSettings(const driver_settings *profile,
			driver_settings *interfaceSettings);

	public:
		//!	Returns interface that owns this profile.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the loaded profile settings.
		const driver_settings *Settings() const
			{ return fSettings; }
		const driver_parameter *SettingsFor(const char *type, const char *name) const;
			// name may be NULL
		
//		size_t Request(void *out, size_t outSize, void *in, size_t inSize) const;

	private:
		KPPPInterface& fInterface;
		driver_settings *fSettings;
};


#endif
