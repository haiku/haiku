/* PoorManSiteView.cpp
 *
 *	Philip Harrison
 *	Started: 5/07/2004
 *	Version: 0.1
 */
 
#include <Box.h>
#include <GroupLayoutBuilder.h>

#include "constants.h"
#include "PoorManSiteView.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"

PoorManSiteView::PoorManSiteView(const char* name)
	: BView(name, B_WILL_DRAW, NULL)
{
	PoorManWindow* win;
	win = ((PoorManApplication *)be_app)->GetPoorManWindow();

	SetLayout(new BGroupLayout(B_VERTICAL));

	// Web Site Location BBox
	BBox* webSiteLocation = new BBox("Web Location");
	webSiteLocation->SetLabel(STR_BBX_LOCATION);
	
	// Web Site Options BBox
	BBox* webSiteOptions = new BBox("Web Options");
	webSiteOptions->SetLabel(STR_BBX_OPTIONS);

	// Send Directory List if No Index
	fSendDir = new BCheckBox("Send Dir", STR_CBX_DIR_LIST_LABEL,
		new BMessage(MSG_PREF_SITE_CBX_INDEX));
	// set the checkbox to the value the program has
	SetSendDirValue(win->DirListFlag());

	// Web Directory Text Control
	fWebDir = new BTextControl("Web Dir", STR_TXT_DIRECTORY, NULL);
	SetWebDir(win->WebDir());
	
	// Select Web Directory Button
	fSelectWebDir = new BButton("Select Web Dir", STR_BTN_DIRECTORY,
		new BMessage(MSG_PREF_SITE_BTN_SELECT));
	
	// Index File Name Text Control
	fIndexFileName = new BTextControl("Index File Name", STR_TXT_INDEX, NULL);
	SetIndexFileName(win->IndexFileName());
	
	webSiteOptions->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(fSendDir)
			.AddGlue())
		.SetInsets(5, 5, 5, 5));
	
	webSiteLocation->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fWebDir)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(fSelectWebDir))
		.Add(fIndexFileName)
		.SetInsets(5, 5, 5, 5));
	
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(webSiteLocation)
		.Add(webSiteOptions)
		.SetInsets(5, 5, 5, 5));
}
