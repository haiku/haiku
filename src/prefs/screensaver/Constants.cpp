#ifndef __CONSTANTS__
#define __CONSTANTS__

static const char *times[]={"30 seconds", "1 minute",             "1 minute 30 seconds",
							"2 minutes",  "2 minutes 30 seconds", "3 minutes",
							"4 minutes",  "5 minutes",            "6 minutes",
							"7 minutes",  "8 minutes",            "9 minutes",
							"10 minutes", "15 minutes",           "20 minutes",
							"25 minutes", "30 minutes",           "40 minutes",
							"50 minutes", "1 hour",               "1 hour 30 minutes",
							"2 hours",    "2 hours 30 minutes",   "3 hours",
							"4 hours",    "5 hours"};
static const int timeInSeconds[]={ 	30,    60,   90,
									120,   150,  180,
									240,   300,  360,
									420,   480,  540,
									600,   900,  1200,
									1500,  1800, 2400,
									3000,  3600, 5400,
									7200,  9000, 10800,
									14400, 18000};

int secondsToSlider(int);

const int TAB1_CHG='TAB1';
const int TAB2_CHG='TAB2';
const int PWBUTTON='PWBT';
const int DONE_CLICKED='DONE';
const int CANCEL_CLICKED='CNCL';
const int BUTTON_CHANGED='BTNC';
const int SHOW='SHOW';
const int POPULATE='POPU';
const int UTILIZE='UTIL';
const int SAVER_SEL='SSEL';

rgb_color black = {0,0,0,0};
rgb_color darkGrey = {150,150,150,0};
rgb_color grey = {200,200,200,0};
rgb_color lightBlue = {200,200,255,0};
rgb_color lightGreen = {255,200,200,0};
rgb_color red = {255,100,100,0};


#endif
