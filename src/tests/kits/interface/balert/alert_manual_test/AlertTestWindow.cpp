// AlertTestWindow.cpp

#include <Application.h>
#include <Roster.h>
#include <Alert.h>
#include <TextView.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>
#include <stdlib.h>
#include "AlertTestWindow.h"

const char *k20X = "XXXXXXXXXXXXXXXXXXXX";
const char *k40X = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const char *k60X = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

AlertTestWindow::AlertTestWindow(BRect frame)
	: BWindow(frame, "AlertTestWindow", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fAlertType = 'H';
	
	BString strLabel = "Alert Manual Test";
	app_info info;
	if (be_app->GetAppInfo(&info) >= B_OK) {
		BEntry entry(&info.ref);
		if (entry.InitCheck() >= B_OK) {
			BPath path(&entry);
			if (path.InitCheck() >= B_OK) {
				strLabel.Append(" (");
				strLabel.Append(path.Leaf());
				strLabel.Append(")");
				printf(": Version: %s\n", path.Leaf());
				
				if (path.Leaf()[0] == 'b')
					fAlertType = 'B';
				else
					fAlertType = 'H';
			}
		}
	}
	
	fTitleView = new BStringView(BRect(10, 10, Bounds().Width() - 10, 30),
		"title", strLabel.String());
	fTitleView->SetFontSize(16);
	
	fRunButton = new BButton(BRect(10, 40, 100, 60),
		"runbtn", "Run", new BMessage(MSG_RUN_BUTTON));
	
	AddChild(fTitleView);
	AddChild(fRunButton);
}

void
AlertTestWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_RUN_BUTTON:
			printf("%c<Run Button\n", fAlertType);
			Test();
			break;
		default:
			break;
	}
}

void which_label(const char *text, BString &outString)
{
	int nX = 0;
	if (strcmp(text, k60X) == 0)
		nX = 60;
	else if (strcmp(text, k40X) == 0)
		nX = 40;
	else if (strcmp(text, k20X) == 0)
		nX = 20;
	
	outString = "";
	if (nX == 0) {
		outString << '"' << text << '"';
	} else {
		outString << 'k' << nX << 'X';
	}
}

void
AlertTestWindow::Test()
{
	BAlert *pAlert = new BAlert(
		"alert1",
		k60X,
		k20X, "OK", "Cancel",
		B_WIDTH_AS_USUAL, // widthStyle
		B_OFFSET_SPACING,
		B_EMPTY_ALERT		// alert_type		
	);
	if (fAlertType == 'H') {
		BView *master = pAlert->ChildAt(0);
		master->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	}
	
	BPoint pt;
	BString strLabel;
	BButton *pBtns[3] = { NULL };
	pBtns[0] = pAlert->ButtonAt(0);
	pBtns[1] = pAlert->ButtonAt(1);
	pBtns[2] = pAlert->ButtonAt(2);
	
	BTextView *pTextView = pAlert->TextView();
	
	// Window info
	printf("wi.width = %.1ff;\n"
		"wi.height = %.1ff;\n"
		"ati.SetWinInfo(wi);\n",
		pAlert->Bounds().Width(), pAlert->Bounds().Height());
		
	// TextView info
	printf("\n");
	which_label(pTextView->Text(), strLabel);
	pt = pTextView->ConvertToParent(BPoint(0, 0));
	printf("ti.label = %s;\n"
		"ti.width = %.1ff;\n"
		"ti.height = %.1ff;\n"
		"ti.topleft.Set(%.1ff, %.1ff);\n"
		"ati.SetTextViewInfo(ti);\n",
		strLabel.String(), pTextView->Bounds().Width(),
		pTextView->Bounds().Height(), pt.x, pt.y);
		
	// Button info
	printf("\n");
	int32 i = 0;
	while (i < 3 && pBtns[i] != NULL) {
		BButton *pb = pBtns[i];
		which_label(pb->Label(), strLabel);
		pt = pb->ConvertToParent(BPoint(0, 0));
		printf("bi.label = %s;\n"
			"bi.width = %.1ff;\n"
			"bi.height = %.1ff;\n"
			"bi.topleft.Set(%.1ff, %.1ff);\n"
			"ati.SetButtonInfo(%d, bi);\n",
			strLabel.String(), pb->Bounds().Width(),
			pb->Bounds().Height(), pt.x, pt.y,
			(int)i);
		i++;
	}

	int32 result = pAlert->Go();
	printf("%c<Clicked: %d\n", fAlertType, static_cast<int>(result));
	pAlert = NULL;
}

bool
AlertTestWindow::QuitRequested()
{
	printf("%c<Quit\n", fAlertType);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
