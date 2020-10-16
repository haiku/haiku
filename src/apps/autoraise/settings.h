#ifndef SETTINGS_H
#define SETTINGS_H


/***************************************************
	settings.h
	Mechanisms for managing the settings setting/retireval for AutoRaise

	2002 mmu_man
	from Deskscope:
	2000 Shamyl Zakariya
***************************************************/
#include "common.h"

/****************************************
	AutoRaiseSettings
	Simple class for getting and setting prefs.
	Instantiating will open up settings file
	Destroying will write settings plus any changes
	back into the file.

	Settings file won't be updated until AutoRaiseSettings
	destructor is called. Doens't matter if it's allocated off
	heap or stack. I recommend stack, though, to keep likelyhood
	of race conditions down.
	
	File is defined in common.h as SETTINGS_FILE

****************************************/

// make adding configuration fields easier
#define CONF_ADDPROP(_type, _name) \
protected:\
		_type _conf##_name;\
public:\
		void Set##_name(_type value);\
		_type _name();

class AutoRaiseSettings
{
	protected:
		BFile _settingsFile;
		BMessage _settingsMessage;
		

			
		BMessage openSettingsFile();
		void closeSettingsFile();
	
	public:
		AutoRaiseSettings();
		~AutoRaiseSettings();

CONF_ADDPROP(bool, Active)
CONF_ADDPROP(bigtime_t, Delay)
CONF_ADDPROP(int32, Mode)

};

#undef CONF_ADDPROP

#endif
