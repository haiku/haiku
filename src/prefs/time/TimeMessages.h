/*
	
	TimeMessages.h
	
*/

#ifndef TIME_MESSAGES_H
#define TIME_MESSAGES_H

#define OBOS_APP_SIGNATURE "application/x-vnd.OpenBeOS-TIME"

const uint32 ERROR_DETECTED = 'ERor';

//Timezonemessages
const uint32 REGION_CHANGED = 'REch';
const uint32 TIME_ZONE_CHANGED = 'TZch';

//SetButton
const uint32 SET_TIME_ZONE = 'SEti';

//local and GMT settings
const uint32 RTC_SETTINGS = 'RTse';

// clock tick message
const uint32 OB_TIME_UPDATE ='obTU';

// clicked on day in claendar
const uint32 OB_DAY_CHANGED = 'obDC';

//notice for clock ticks
const uint32 OB_TM_CHANGED = 'obTC';

//notice for user changes
const uint32 OB_USER_CHANGE = 'obUC';

// local/gmt radiobuttons
const uint32 OB_RTC_CHANGE = 'obRC';

#endif //TIME_MESSAGES_H
