/*Terminal: constants*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <GraphicsDefs.h>
#include <SupportDefs.h>

#define APP_SIGNATURE  "application/x-vnd.obos.terminal"

const float TEXT_INSET = 3.0;

// Application messages

const uint32 OPEN_TERMINAL              ='OTRM';

// Messages for menu commands

// Terminal
const uint32 TERMINAL_SWITCH_TERMINAL	='TSWT';
const uint32 TERMINAL_START_NEW_TERMINAL='TSNT';
const uint32 TERMINAL_LOG_TO_FILE		='TLTF';
// Edit
const uint32 EDIT_WRITE_SELECTION		='EWSL';
const uint32 EDIT_CLEAR_ALL				='ECLA';
const uint32 EDIT_FIND					='EFND';
const uint32 EDIT_FIND_BACKWARD			='EFBK';
const uint32 EDIT_FIND_FORWARD			='EFFD';
// Settings
const uint32 SETTINGS_WINDOW_SIZE		='SWSZ';
const uint32 SETTINGS_FONT				='SFNT';
const uint32 SETTINGS_FONT_SIZE			='SFSZ';
const uint32 SETTINGS_FONT_ENCODING		='SFEN';
const uint32 SETTINGS_TAB_WIDTH			='STBW';
const uint32 SETTINGS_COLOR				='SCLR';
const uint32 SETTINGS_SAVE_AS_DEFAULT	='SSAD';
const uint32 SETTINGS_SAVE_AS_SETTINGS	='SSAS';

// Edit menu toggle
const uint32 ENABLE_ITEMS				='EDON';
const uint32 DISABLE_ITEMS				='EDOF';

#endif // CONSTANTS_H
