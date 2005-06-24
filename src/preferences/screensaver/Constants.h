#ifndef __CONSTANTS__
#define __CONSTANTS__

int secondsToSlider(int val);

static const char *kTimes[]={"30 seconds", "1 minute",             "1 minute 30 seconds",
							"2 minutes",  "2 minutes 30 seconds", "3 minutes",
							"4 minutes",  "5 minutes",            "6 minutes",
							"7 minutes",  "8 minutes",            "9 minutes",
							"10 minutes", "15 minutes",           "20 minutes",
							"25 minutes", "30 minutes",           "40 minutes",
							"50 minutes", "1 hour",               "1 hour 30 minutes",
							"2 hours",    "2 hours 30 minutes",   "3 hours",
							"4 hours",    "5 hours"};

static const int kTimeInSeconds[]={ 	30,    60,   90,
									120,   150,  180,
									240,   300,  360,
									420,   480,  540,
									600,   900,  1200,
									1500,  1800, 2400,
									3000,  3600, 5400,
									7200,  9000, 10800,
									14400, 18000};

inline BPoint 
scaleDirect(float x, float y,BRect area) 
{
	return BPoint(area.Width()*x+area.left,area.Height()*y+area.top);
}


inline BRect scaleDirect (float x1,float x2,float y1,float y2,BRect area) 
{
	return BRect(area.Width()*x1+area.left,area.Height()*y1+area.top, area.Width()*x2+area.left,area.Height()*y2+area.top);
}

static const float kPositionalX[]= {0,.1,.25,.3,.7,.75,.9,1.0};
static const float kPositionalY[]= {0,.1,.7,.8,.9,1.0};

inline BPoint 
scale(int x, int y,BRect area) 
{ 
	return scaleDirect(kPositionalX[x],kPositionalY[y],area); 
}


inline BRect 
scale(int x1, int x2, int y1, int y2,BRect area) 
{ 
	return scaleDirect(kPositionalX[x1],kPositionalX[x2],kPositionalY[y1],kPositionalY[y2],area); 
}


inline BPoint 
scale3(int x, int y,BRect area,bool invertX,bool invertY) 
{ 
	float arrowX[]= {0,.25,.5,.66667,.90,.9};
	float arrowY[]= {0,.15,.25,.3333333,.6666667,1.0};

	return scaleDirect(((invertX)?1-arrowX[x]:arrowX[x]),((invertY)?1-arrowY[y]:arrowY[y]),area); 
}



const int kTab1_chg       ='TAB1';
const int kTab2_chg       ='TAB2';
const int kPwbutton       ='PWBT';
const int kDone_clicked   ='DONE';
const int kCancel_clicked ='CNCL';
const int kButton_changed ='BTNC';
const int kShow           ='SHOW';
const int kPopulate       ='POPU';
const int kUtilize        ='UTIL';
const int kSaver_sel      ='SSEL';
const int kTest_btn       ='TEST';
const int kAdd_btn        ='ADD ';
const int kUpdatelist     ='UPDL';

const rgb_color kBlack      = {0,0,0,0};
const rgb_color kDarkGrey   = {150,150,150,0};
const rgb_color kGrey       = {200,200,200,0};
const rgb_color kLightBlue  = {200,200,255,0};
const rgb_color kLightGreen = {255,200,200,0};
const rgb_color kRed        = {255,100,100,0};


#endif
