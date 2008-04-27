/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 */
#ifndef TIME_MESSAGES_H
#define TIME_MESSAGES_H

//Timezone messages
const uint32 H_REGION_CHANGED = 'h_RC';
const uint32 H_CITY_CHANGED = 'h_CC';
const uint32 H_CITY_SET = 'h_CS';


//SetButton
const uint32 H_SET_TIME_ZONE = 'hSTZ';

//local and GMT settings
const uint32 RTC_SETTINGS = 'RTse';

// clock tick message
const uint32 H_TIME_UPDATE ='obTU';

//notice for clock ticks
const uint32 H_TM_CHANGED = 'obTC';

//notice for user changes
const uint32 H_USER_CHANGE = 'obUC';

// local/ gmt radiobuttons
const uint32 kRTCUpdate = '_rtc';

// sunday/ monday radio button
const uint32 kWeekStart = '_kws';

// clicked on day in calendar
const uint32 kDayChanged = '_kdc';

// clicked on revert button
const uint32 kMsgRevert = 'rvrt';

#endif //TIME_MESSAGES_H

