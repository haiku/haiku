/* PoorManSiteView.cpp
 *
 *	Philip Harrison
 *	Started: 5/07/2004
 *	Version: 0.1
 */
 
#include <Box.h>

#include "constants.h"
#include "PoorManSiteView.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"

PoorManSiteView::PoorManSiteView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	PoorManWindow	*	win;
	win = ((PoorManApplication *)be_app)->GetPoorManWindow();

	SetViewColor(BACKGROUND_COLOR);

	// Web Site Location BBox
	BRect webLocationRect;
	webLocationRect = rect;
	webLocationRect.top -= 5.0;
	webLocationRect.left -= 5.0;
	webLocationRect.right -= 7.0;
	webLocationRect.bottom -= 98.0;

	BBox * webSiteLocation = new BBox(webLocationRect, "Web Location");
	webSiteLocation->SetLabel(STR_BBX_LOCATION);
	AddChild(webSiteLocation);
	
	// Web Site Options BBox
	BRect webOptionsRect;
	webOptionsRect = webLocationRect;
	webOptionsRect.top = webOptionsRect.bottom + 10.0;
	webOptionsRect.bottom = webOptionsRect.top + 80.0;

	BBox * webSiteOptions = new BBox(webOptionsRect, "Web Options");
	webSiteOptions->SetLabel(STR_BBX_OPTIONS);
	AddChild(webSiteOptions);

	// Send Directory List if No Index
	float left = 10.0;
	float top = 20.0;
	float box_size = 13.0;
	BRect sendDirRect(left, top, webOptionsRect.Width() - 5.0, top + box_size);
	sendDir = new BCheckBox(sendDirRect, "Send Dir", STR_CBX_DIR_LIST_LABEL, new BMessage(MSG_PREF_SITE_CBX_INDEX));
	// set the checkbox to the value the program has
	SetSendDirValue(win->DirListFlag());
	webSiteOptions->AddChild(sendDir);


	// Finish the Web Site Location Section
	BRect webSiteLocationRect;
	webSiteLocationRect = webLocationRect;
	webSiteLocationRect.InsetBy(10.0, 7.0);
	webSiteLocationRect.top += 13.0;
	webSiteLocationRect.bottom = webSiteLocationRect.top + 19.0;
	
	// Web Directory Text Control
	webDir = new BTextControl(webSiteLocationRect, "Web Dir", 
		STR_TXT_DIRECTORY, NULL, NULL);
	webDir->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	webDir->SetDivider(80.0);
	SetWebDir(win->WebDir());
	webSiteLocation->AddChild(webDir);
	
	// Select Web Directory Button
	BRect selectWebDirRect;

	selectWebDirRect.top = webSiteLocationRect.bottom + 5.0;
	selectWebDirRect.right = webSiteLocationRect.right + 2.0;
	selectWebDirRect.left = selectWebDirRect.right
		- webSiteLocation->StringWidth("Select Web Dir") - 24.0;
	selectWebDirRect.bottom = selectWebDirRect.top + 19.0;
	
	selectWebDir = new BButton(selectWebDirRect, "Select Web Dir", 
		STR_BTN_DIRECTORY, new BMessage(MSG_PREF_SITE_BTN_SELECT));
	webSiteLocation->AddChild(selectWebDir);
	
	// Index File Name Text Control
	//webDirRect.InsetBy(10.0, 7.0);
	webSiteLocationRect.top += 63.0;
	webSiteLocationRect.bottom = webSiteLocationRect.top + 19.0;
	
	indexFileName = new BTextControl(webSiteLocationRect,
		"Index File Name", STR_TXT_INDEX, NULL, NULL);
	indexFileName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	indexFileName->SetDivider(80.0);
	SetIndexFileName(win->IndexFileName());
	webSiteLocation->AddChild(indexFileName);
	
}
