#ifndef CONSTANTS_H
#define CONSTANTS_H

//Messages
static const uint32 WORKSPACE_CHECK_MSG = 'wchk';
static const uint32 BUTTON_DEFAULTS_MSG = 'bdef';
static const uint32 BUTTON_REVERT_MSG = 'brev';
static const uint32 BUTTON_APPLY_MSG = 'bapl';
static const uint32 BUTTON_DONE_MSG = 'bdon';
static const uint32 BUTTON_CANCEL_MSG = 'bcnc';
static const uint32 BUTTON_KEEP_MSG = 'bkep';
static const uint32 POP_WORKSPACE_CHANGED_MSG = 'pwsc';
static const uint32 POP_RESOLUTION_MSG = 'pres';
static const uint32 POP_COLORS_MSG = 'pclr';
static const uint32 POP_REFRESH_MSG = 'prfr';
static const uint32 POP_OTHER_REFRESH_MSG = 'porf';
static const uint32 UPDATE_DESKTOP_COLOR_MSG = 'udsc';
static const uint32 UPDATE_DESKTOP_MSG = 'udsk';
static const uint32 SLIDER_MODIFICATION_MSG = 'sldm';
static const uint32 SLIDER_INVOKE_MSG = 'sldi';
static const uint32 SET_INITIAL_MODE_MSG = 'sinm';
static const uint32 SET_CUSTOM_REFRESH_MSG = 'scrf';
static const uint32 DIM_COUNT_MSG = 'scrf';
static const uint32 MAKE_INITIAL_MSG = 'mkin';

//Constants
static const char kAppSignature[] = "application/x-vnd.Be-SCRN";
static const int32 gMinRefresh = 45; //This is the minimum selectable refresh
static const int32 gMaxRefresh = 140; //This is the maximum selectable refresh

#endif //CONSTANTS_H
