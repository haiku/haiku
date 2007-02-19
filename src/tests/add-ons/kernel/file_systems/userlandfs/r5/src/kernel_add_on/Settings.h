// Settings.h

#ifndef USERLAND_FS_SETTINGS_H
#define USERLAND_FS_SETTINGS_H

#include <SupportDefs.h>

struct driver_settings;
struct driver_parameter;
struct IOCtlInfo;

// Settings
class Settings {
public:
								Settings();
								~Settings();

			status_t			SetTo(const char *fsName);
			void				Unset();

			const IOCtlInfo*	GetIOCtlInfo(int command) const;

			void				Dump() const;

private:
			status_t			_Init(const driver_settings *settings,
									const driver_parameter *fsParams);

private:
			struct IOCtlInfoMap;

			IOCtlInfoMap*		fIOCtlInfos;
};

#endif	// USERLAND_FS_SETTINGS_H
