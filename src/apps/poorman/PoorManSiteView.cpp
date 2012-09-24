/* PoorManSiteView.cpp
 *
 *	Philip Harrison
 *	Started: 5/07/2004
 *	Version: 0.1
 */
 
#include <Box.h>
#include <LayoutBuilder.h>

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
	fWebDir = new BTextControl(STR_TXT_DIRECTORY, NULL, NULL);
	SetWebDir(win->WebDir());
	
	// Select Web Directory Button
	fSelectWebDir = new BButton("Select Web Dir", STR_BTN_DIRECTORY,
		new BMessage(MSG_PREF_SITE_BTN_SELECT));
	
	// Index File Name Text Control
	fIndexFileName = new BTextControl(STR_TXT_INDEX, NULL, NULL);
	SetIndexFileName(win->IndexFileName());
	
	
	BGroupLayout* webSiteLocationLayout = new BGroupLayout(B_VERTICAL, 0);
	webSiteLocation->SetLayout(webSiteLocationLayout);
	
	BGroupLayout* webSiteOptionsLayout = new BGroupLayout(B_VERTICAL, 0);
	webSiteOptions->SetLayout(webSiteOptionsLayout);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_ITEM_INSETS)
		.AddGroup(webSiteLocationLayout)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.SetInsets(0, B_USE_ITEM_INSETS, 0, 0)
				.AddTextControl(fWebDir, 0, 0, B_ALIGN_LEFT, 1, 2)
				.Add(fSelectWebDir, 2, 1)
				.AddTextControl(fIndexFileName, 0, 2, B_ALIGN_LEFT, 1, 2)
				.SetColumnWeight(1, 10.f)
				.End()
			.End()
		.AddGroup(webSiteOptionsLayout)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddStrut(B_USE_ITEM_SPACING)
			.AddGroup(B_HORIZONTAL)
				.SetInsets(0)
				.Add(fSendDir)
				.AddGlue()
				.End()
			.AddGlue();
}
