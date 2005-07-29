#ifndef __CONSTANTS__
#define __CONSTANTS__

int secondsToSlider(int val);

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



const int32 kTab1_chg       ='TAB1';
const int32 kTab2_chg       ='TAB2';
const int32 kPwbutton       ='PWBT';
const int32 kDone_clicked   ='DONE';
const int32 kCancel_clicked ='CNCL';
const int32 kButton_changed ='BTNC';
const int32 kShow           ='SHOW';
const int32 kPopulate       ='POPU';
const int32 kUtilize        ='UTIL';
const int32 kSaver_sel      ='SSEL';
const int32 kTest_btn       ='TEST';
const int32 kAdd_btn        ='ADD ';
const int32 kUpdatelist     ='UPDL';
const int32 kPasswordCheckbox = 'PWCB';
const int32 kRunSliderChanged = 'RSCG';
const int32 kPasswordSliderChanged = 'PWCG';
const int32 kTurnOffSliderChanged = 'TUCG';
const int32 kEnableScreenSaverChanged = 'ESCH';

const rgb_color kBlack      = {0,0,0,0};
const rgb_color kDarkGrey   = {150,150,150,0};
const rgb_color kGrey       = {200,200,200,0};
const rgb_color kLightBlue  = {200,200,255,0};
const rgb_color kLightGreen = {255,200,200,0};
const rgb_color kRed        = {255,100,100,0};


#endif
