#ifndef DEFS_H_
#define DEFS_H_

#include <bluetooth/LocalDevice.h>

#include <bluetoothserver_p.h>


#define APPLY_SETTINGS 'aply'
#define REVERT_SETTINGS 'rvrt'
#define DEFAULT_SETTINGS 'dflt'
#define TRY_SETTINGS 'trys'

#define ATTRIBUTE_CHOSEN 'atch'
#define UPDATE_COLOR 'upcl'
#define DECORATOR_CHOSEN 'dcch'
#define UPDATE_DECORATOR 'updc'
#define UPDATE_COLOR_SET 'upcs'

#define SET_VISIBLE 		'sVis'
#define SET_DISCOVERABLE 	'sDis'
#define SET_AUTHENTICATION 	'sAth'

#define SET_UI_COLORS 'suic'
#define PREFS_CHOSEN 'prch'

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

static const uint32 kMsgAddToRemoteList = 'aDdL';
static const uint32 kMsgRefresh = 'rFLd';

extern LocalDevice* ActiveLocalDevice;

#endif
