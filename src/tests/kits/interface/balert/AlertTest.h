// AlertTest.h

#ifndef ALERT_TEST_H
#define ALERT_TEST_H

#include <TestCase.h>
#include <TestShell.h>

class CppUnit::Test;

class AlertTest : public BTestCase {
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	//
	// For conciseness, the test function names are made of codes
	// which indicate which BAlert construtor options are chosen.
	//
	// The format is:
	//
	//		btn0[_btn1[_btn2]]_msg_widthMode_spacingMode_alertType
	//
	// where:	btn#	is the label of the button for that index, 
	//			msg		is the alert message
	//			widthMode is the button_width setting:
	//				UW = B_WIDTH_AS_USUAL
	//				WW = B_WIDTH_FROM_WIDEST
	//				LW = B_WIDTH_FROM_LABEL
	//			spacingMode	is the button_spacing setting:
	//				ES = B_EVEN_SPACING
	//				OS = B_OFFSET_SPACING
	//			alertType specifies the icon displayed on the alert:
	//				EA = B_EMPTY_ALERT
	//				IA = B_INFO_ALERT
	//				LA = B_IDEA_ALERT
	//				WA = B_WARNING_ALERT
	//				SA = B_STOP_ALERT
	
	////// UW_ES_IA - One Button //////
	void empty_empty_UW_ES_IA();
	void OK_X_UW_ES_IA();
	void OK_60X_UW_ES_IA();
	void twentyX_60X_UW_ES_IA();
	void fortyX_60X_UW_ES_IA();
	
	////// LW_ES_IA - One Button //////
	void empty_empty_LW_ES_IA();
	void OK_X_LW_ES_IA();
	void twentyX_60X_LW_ES_IA();
	void fortyX_60X_LW_ES_IA();
	
	////// WW_ES_IA - One Button //////
	void empty_empty_WW_ES_IA();
	void OK_X_WW_ES_IA();
	void twentyX_60X_WW_ES_IA();
	
	////// UW_ES_EA - One Button //////
	void OK_X_UW_ES_EA();
	void fortyX_60X_UW_ES_EA();
	
	////// UW_OS_IA - One Button //////
	void OK_X_UW_OS_IA();
	void fortyX_60X_UW_OS_IA();
	
	////// LW_OS_IA - One Button //////
	void OK_X_LW_OS_IA();
	
	////// UW_OS_EA - One Button //////
	void OK_X_UW_OS_EA();
	
	////// UW_ES_IA - Two Button //////
	void OK_Cancel_60X_UW_ES_IA();
	void twentyX_Cancel_60X_UW_ES_IA();
	void twentyX_20X_60X_UW_ES_IA();
	
	////// LW_ES_IA - Two Button //////
	void empty_empty_X_LW_ES_IA();
	void OK_Cancel_60X_LW_ES_IA();
	
	////// WW_ES_IA - Two Button //////
	void empty_empty_X_WW_ES_IA();
	void OK_Cancel_60X_WW_ES_IA();
	void twentyX_Cancel_60X_WW_ES_IA();
	void twentyX_20X_WW_ES_IA();
	
	////// UW_ES_EA - Two Button //////
	void OK_Cancel_60X_UW_ES_EA();
	void twentyX_20X_60X_UW_ES_EA();
	
	////// UW_OS_IA - Two Button //////
	void OK_Cancel_60X_UW_OS_IA();
	
	////// LW_OS_IA - Two Button //////
	void OK_Cancel_60X_LW_OS_IA();
	
	////// LW_OS_EA - Two Button //////
	void twentyX_OK_60X_LW_OS_EA();
	
	////// UW_ES_IA - Three Button //////
	void twentyX_20X_20X_60X_UW_ES_IA();
	
	////// LW_ES_IA - Three Button //////
	void empty_empty_empty_X_LW_ES_IA();
	void Yes_No_Cancel_X_LW_ES_IA();
	void twentyX_20X_20X_60X_LW_ES_IA();
	
	////// WW_ES_IA - Three Button //////
	void empty_empty_empty_X_WW_ES_IA();
	void Monkey_Dog_Cat_X_WW_ES_IA();
	void X_20X_X_WW_ES_IA();
	void Yes_No_Cancel_X_WW_ES_IA();
	void twentyX_20X_20X_60X_WW_ES_IA();
	
	////// UW_ES_EA - Three Button //////
	void twentyX_20X_20X_60X_UW_ES_EA();
	
	////// UW_OS_IA - Three Button //////
	void Yes_No_Cancel_60X_UW_OS_IA();
	
	////// LW_OS_IA - Three Button //////
	void Yes_No_Cancel_60X_LW_OS_IA();
	
	////// WW_OS_IA - Three Button //////
	void Monkey_Dog_Cat_60X_WW_OS_IA();
	
	////// UW_OS_EA - Three Button //////
	void twentyX_OK_Cancel_60X_UW_OS_EA();
};

#endif	// ALERT_TEST_H
