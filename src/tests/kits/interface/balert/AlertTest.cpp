// AlertTest.cpp
#include "AlertTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Alert.h>
#include <Point.h>
#include <TextView.h>
#include <Button.h>
#include <Rect.h>

#define ASSERT_DEQUAL(x,y) CPPUNIT_ASSERT_DOUBLES_EQUAL((x),(y),0.01)

// Required by CPPUNIT_ASSERT_EQUAL(rgb_color, rgb_color)
bool operator==(const rgb_color &left, const rgb_color &right)
{
	if (left.red == right.red && left.green == right.green &&
	    left.blue == right.blue && left.alpha == right.alpha)
	    return true;
	else
		return false;
}
// Required by CPPUNIT_ASSERT_EQUAL(rgb_color, rgb_color)
ostream &operator<<(ostream &stream, const rgb_color &clr)
{
	return stream << "rgb_color(" << clr.red << ", " <<
		clr.green << ", " << clr.blue << ", " << clr.alpha << ")";
}

// For storing expected state of windows or views.
struct GuiInfo {
	const char*	label;
	float		width;
	float		height;
	BPoint		topleft;
};

// For storing all the information required to create and
// verify the state of a BAlert object.
class AlertTestInfo {
public:
	AlertTestInfo(AlertTest *pTest);
	~AlertTestInfo();
	
	void SetWinInfo(const GuiInfo &winInfo);
	void SetTextViewInfo(const GuiInfo &textInfo);
	void SetButtonInfo(int32 btn, const GuiInfo &btnInfo);
	
	void SetButtonWidthMode(button_width widthMode);
	void SetButtonSpacingMode(button_spacing spacingMode);
	void SetAlertType(alert_type alertType);
	
	void GuiInfoTest();

private:
	AlertTest*		fTest;
	GuiInfo			fWinInfo;
	GuiInfo			fTextInfo;
	GuiInfo			fButtonInfo[3];
	int32 			fButtonCount;
	button_width	fWidthMode;
	button_spacing	fSpacingMode;
	alert_type		fAlertType;
};

AlertTestInfo::AlertTestInfo(AlertTest *pTest)
{
	memset(this, 0, sizeof(AlertTestInfo));
	
	fTest = pTest;
	fWidthMode = B_WIDTH_AS_USUAL;
	fSpacingMode = B_EVEN_SPACING;
	fAlertType = B_INFO_ALERT;
}

AlertTestInfo::~AlertTestInfo()
{
	fTest = NULL;
	fButtonCount = 0;
}

void
AlertTestInfo::SetWinInfo(const GuiInfo &winInfo)
{
	fWinInfo = winInfo;
}

void
AlertTestInfo::SetTextViewInfo(const GuiInfo &textInfo)
{
	fTextInfo = textInfo;
}

void
AlertTestInfo::SetButtonInfo(int32 btn, const GuiInfo &btnInfo)
{
	if (btn < 0 || btn > 2 || btn > fButtonCount) {
		CPPUNIT_ASSERT(false);
		return;
	}
	fButtonInfo[btn] = btnInfo;
	if (btn == fButtonCount && fButtonCount < 3)
		fButtonCount++;
}
	
void
AlertTestInfo::SetButtonWidthMode(button_width widthMode)
{
	fWidthMode = widthMode;
}

void
AlertTestInfo::SetButtonSpacingMode(button_spacing spacingMode)
{
	fSpacingMode = spacingMode;
}

void
AlertTestInfo::SetAlertType(alert_type alertType)
{
	fAlertType = alertType;
}
	
void
AlertTestInfo::GuiInfoTest()
{
	fTest->NextSubTest();
	// Dummy application object required to create Window objects.
	BApplication app("application/x-vnd.Haiku-interfacekit_alerttest");
	BAlert *pAlert = new BAlert(
		fWinInfo.label,
		fTextInfo.label,
		fButtonInfo[0].label,
		fButtonInfo[1].label,
		fButtonInfo[2].label,
		fWidthMode,
		fSpacingMode,
		fAlertType
	);
	CPPUNIT_ASSERT(pAlert);
	
	// Alert Window Width/Height
	fTest->NextSubTest();
	ASSERT_DEQUAL(fWinInfo.width, pAlert->Bounds().Width());
	ASSERT_DEQUAL(fWinInfo.height, pAlert->Bounds().Height());
	
	// [k] MasterView
	fTest->NextSubTest();
	BView *masterView = pAlert->ChildAt(0);
	CPPUNIT_ASSERT(masterView);
	
	// [k] MasterView Color
	fTest->NextSubTest();
	CPPUNIT_ASSERT_EQUAL(ui_color(B_PANEL_BACKGROUND_COLOR),
		masterView->ViewColor());
	
	// Test all the buttons
	BButton *btns[3] = { NULL };
	for (int32 i = 0; i < 3; i++) {
		fTest->NextSubTest();
		btns[i] = pAlert->ButtonAt(i);
		
		if (i >= fButtonCount) {
			// If there is should be no button at this index
			CPPUNIT_ASSERT_EQUAL((BButton*)NULL, btns[i]);
		} else {
			// If there should be a button at this index
			CPPUNIT_ASSERT(btns[i]);
			
			CPPUNIT_ASSERT(
				strcmp(fButtonInfo[i].label, btns[i]->Label()) == 0);
				
			ASSERT_DEQUAL(fButtonInfo[i].width, btns[i]->Bounds().Width());
			
			ASSERT_DEQUAL(fButtonInfo[i].height, btns[i]->Bounds().Height());
			
			BPoint pt = btns[i]->ConvertToParent(BPoint(0, 0));
			ASSERT_DEQUAL(fButtonInfo[i].topleft.x, pt.x);
			ASSERT_DEQUAL(fButtonInfo[i].topleft.y, pt.y);
			
			if (i == fButtonCount - 1) {
				// Default button
				CPPUNIT_ASSERT_EQUAL(true, btns[i]->IsDefault());
			}
		}
	}
	
	// [k] TextView
	fTest->NextSubTest();
	BTextView *textView = pAlert->TextView();
	CPPUNIT_ASSERT(textView);
	
	// [k] TextView ViewColor()
	fTest->NextSubTest();
	CPPUNIT_ASSERT_EQUAL(ui_color(B_PANEL_BACKGROUND_COLOR), textView->ViewColor());
	
	// [k] TextView IsEditable()
	fTest->NextSubTest();
	CPPUNIT_ASSERT_EQUAL(false, textView->IsEditable());
	
	// [k] TextView IsSelectable()
	fTest->NextSubTest();
	CPPUNIT_ASSERT_EQUAL(false, textView->IsSelectable());
	
	// [k] TextView DoesWordWrap()
	fTest->NextSubTest();
	CPPUNIT_ASSERT_EQUAL(true, textView->DoesWordWrap());
	
	// TextView Text
	fTest->NextSubTest();
	CPPUNIT_ASSERT(strcmp(fTextInfo.label, textView->Text()) == 0);
	
	// TextView Width/Height
	fTest->NextSubTest();
	ASSERT_DEQUAL(fTextInfo.width, textView->Bounds().Width());
	ASSERT_DEQUAL(fTextInfo.height, textView->Bounds().Height());
	
	// TextView Position
	fTest->NextSubTest();
	BPoint pt = textView->ConvertToParent(BPoint(0, 0));
	ASSERT_DEQUAL(fTextInfo.topleft.x, pt.x);
	ASSERT_DEQUAL(fTextInfo.topleft.y, pt.y);
	
	delete pAlert;
	pAlert = NULL;	
}

// Suite
CppUnit::Test *
AlertTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<AlertTest> TC;
	
	suite->addTest(
		new TC("Alert empty_empty_UW_ES_IA",
			&AlertTest::empty_empty_UW_ES_IA));
					
	suite->addTest(
		new TC("Alert OK_X_UW_ES_IA",
			&AlertTest::OK_X_UW_ES_IA));
			
	suite->addTest(
		new TC("Alert OK_60X_UW_ES_IA",
			&AlertTest::OK_60X_UW_ES_IA));
		
	return suite;
}		

// setUp
void
AlertTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
AlertTest::tearDown()
{
	BTestCase::tearDown();
}

void
AlertTest::empty_empty_UW_ES_IA()
{
	AlertTestInfo ati(this);
	GuiInfo wi, ti, bi;
	wi.label = "alert1";
	wi.width = 310.0f;
	wi.height = 64.0f;
	ati.SetWinInfo(wi);
	
	ti.label = "";
	ti.width = 245.0f;
	ti.height = 13.0f;
	ti.topleft.Set(55.0f, 6.0f);
	ati.SetTextViewInfo(ti);
	
	bi.label = "";
	bi.width = 75.0f;
	bi.height = 30.0f;
	bi.topleft.Set(229.0f, 28.0f);
	ati.SetButtonInfo(0, bi);
	
	ati.SetButtonWidthMode(B_WIDTH_AS_USUAL);
	ati.SetButtonSpacingMode(B_EVEN_SPACING);
	ati.SetAlertType(B_INFO_ALERT);
	
	ati.GuiInfoTest();
}

void
AlertTest::OK_X_UW_ES_IA()
{
	AlertTestInfo ati(this);
	GuiInfo wi, ti, bi;
	wi.label = "alert1";
	wi.width = 310.0f;
	wi.height = 64.0f;
	ati.SetWinInfo(wi);
	
	ti.label = "X";
	ti.width = 245.0f;
	ti.height = 13.0f;
	ti.topleft.Set(55.0f, 6.0f);
	ati.SetTextViewInfo(ti);
	
	bi.label = "OK";
	bi.width = 75.0f;
	bi.height = 30.0f;
	bi.topleft.Set(229.0f, 28.0f);
	ati.SetButtonInfo(0, bi);
	
	ati.SetButtonWidthMode(B_WIDTH_AS_USUAL);
	ati.SetButtonSpacingMode(B_EVEN_SPACING);
	ati.SetAlertType(B_INFO_ALERT);
	
	ati.GuiInfoTest();
}

void
AlertTest::OK_60X_UW_ES_IA()
{
	AlertTestInfo ati(this);
	GuiInfo wi, ti, bi;
	wi.label = "alert1";
	wi.width = 310.0f;
	wi.height = 77.0f;
	ati.SetWinInfo(wi);
	
	ti.label = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
	ti.width = 245.0f;
	ti.height = 26.0f;
	ti.topleft.Set(55.0f, 6.0f);
	ati.SetTextViewInfo(ti);
	
	bi.label = "OK";
	bi.width = 75.0f;
	bi.height = 30.0f;
	bi.topleft.Set(229.0f, 41.0f);
	ati.SetButtonInfo(0, bi);
	
	ati.SetButtonWidthMode(B_WIDTH_AS_USUAL);
	ati.SetButtonSpacingMode(B_EVEN_SPACING);
	ati.SetAlertType(B_INFO_ALERT);
	
	ati.GuiInfoTest();
}



