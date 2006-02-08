/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderWindow.cpp
//
// Copyright (c) 2004 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Entry.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Screen.h>
#include <Alert.h>
#include <SupportDefs.h>

#include "ExpanderApp.h"
#include "ExpanderWindow.h"
#include "ExpanderThread.h"
#include "ExpanderPreferences.h"

const uint32 MSG_SOURCE		= 'mSOU';
const uint32 MSG_DEST		= 'mDES';
const uint32 MSG_EXPAND		= 'mEXP';
const uint32 MSG_SHOW		= 'mSHO';
const uint32 MSG_STOP		= 'mSTO';
const uint32 MSG_CLOSE		= 'mCLO';
const uint32 MSG_PREFERENCES = 'mPRE';
const uint32 MSG_SOURCETEXT = 'mSTX';
const uint32 MSG_DESTTEXT = 'mDTX';
const uint32 MSG_SHOWCONTENTS = 'mSCT';

ExpanderWindow::ExpanderWindow(BRect frame_rect, const entry_ref *ref, BMessage *settings)
	: BWindow(frame_rect, "Expand-O-Matic", B_TITLED_WINDOW, B_NOT_ZOOMABLE),
	fSourceChanged(true),
	fListingThread(NULL),
	fListingStarted(false),
	fExpandingThread(NULL),
	fExpandingStarted(false),
	fSettings(*settings),
	fPreferences(NULL)
{
	fSourcePanel = NULL;
	fDestPanel = NULL;
	
	// create menu bar	
	fBar = new BMenuBar(BRect(0, 0, Bounds().right, 20), "menu_bar");
	BMenu *menu = new BMenu("File");
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("About Expander", new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app_messenger);
	menu->AddSeparatorItem();
	menu->AddItem(fSourceItem = new BMenuItem("Set Source...", new BMessage(MSG_SOURCE), 'O'));
	menu->AddItem(fDestItem = new BMenuItem("Set Destination...", new BMessage(MSG_DEST), 'D'));
	menu->AddSeparatorItem();
	menu->AddItem(fExpandItem = new BMenuItem("Expand", new BMessage(MSG_EXPAND), 'E'));
	fExpandItem->SetEnabled(false);
	menu->AddItem(fShowItem = new BMenuItem("Show Contents", new BMessage(MSG_SHOW), 'L'));
	fShowItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fStopItem = new BMenuItem("Stop", new BMessage(MSG_STOP), 'K'));
	fStopItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close", new BMessage(MSG_CLOSE), 'W'));
	fBar->AddItem(menu);
	
	menu = new BMenu("Edit");
	menu->AddItem(fPreferencesItem = new BMenuItem("Preferences...", new BMessage(MSG_PREFERENCES), 'P'));
	fBar->AddItem(menu);
	AddChild(fBar);
	
	BRect rect = Bounds();
	rect.top += fBar->Bounds().Height() + 1;
	BBox *box = new BBox(rect, "background", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(box);

	float size = 8 + be_plain_font->StringWidth("Destination")/2;
	rect = BRect(42 - size, 10, 42 + size, 25);
	fSourceButton = new BButton(rect, "sourceButton", "Source", 
		new BMessage(MSG_SOURCE), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(fSourceButton);
	rect.OffsetBy(0, 28);
	fDestButton = new BButton(rect, "destButton", "Destination", 
		new BMessage(MSG_DEST), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(fDestButton);
	rect.OffsetBy(0, 28);
	fExpandButton = new BButton(rect, "expandButton", "Expand", 
		new BMessage(MSG_EXPAND), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(fExpandButton);
	fExpandButton->SetEnabled(false);
	
	rect = BRect(80, 12, Bounds().right - 10, 25);
	fSourceText = new BTextControl(rect, "sourceText", "", NULL, 
		new BMessage(MSG_SOURCETEXT), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fSourceText->SetDivider(0);
	box->AddChild(fSourceText);
		
	rect.OffsetBy(0, 28);
	fDestText = new BTextControl(rect, "destText", "", NULL, 
		new BMessage(MSG_DESTTEXT), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fDestText->SetDivider(0);
	box->AddChild(fDestText);
	
	BRect frameRect = rect.OffsetByCopy(0, 27);
	frameRect.left += 4;
	frameRect.right = frameRect.left + 250;
	BRect textRect(frameRect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	fExpandedText = new BTextView(frameRect, "expandedText", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fExpandedText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fExpandedText->MakeEditable(false);
	fExpandedText->MakeSelectable(false);
	box->AddChild(fExpandedText);
	fExpandedText->SetText("");
		
	rect = BRect(Bounds().right - 38 - be_plain_font->StringWidth("Show Contents"), rect.bottom + 13, Bounds().right - 10, rect.bottom + 28);
	fShowContents = new BCheckBox(rect, "showContents", "Show Contents", 
		new BMessage(MSG_SHOWCONTENTS), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	box->AddChild(fShowContents);
	fShowContents->SetEnabled(false);
	
	frameRect = Bounds();
	frameRect.InsetBy(10, 0);
	frameRect.top += frameRect.Height() - 21;
	frameRect.bottom = frameRect.top + 100;
	frameRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	textRect = frameRect;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	fListingText = new BTextView(frameRect, "listingText", textRect,
		be_fixed_font, NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);
	fListingText->SetText("");
	fListingText->MakeEditable(false);
	fListingScroll = new BScrollView("listingScroll", fListingText, 
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	box->AddChild(fListingScroll);
	fListingScroll->Hide();
	
	SetSizeLimits(450, BScreen(this).Frame().Width(), 120, 120);
	
	// finish creating window
	Show();
}

ExpanderWindow::~ExpanderWindow()
{
}

void
ExpanderWindow::FrameResized(float width, float height)
{
	if (fListingText->DoesWordWrap()) {
		BRect textRect;
		textRect = fListingText->Bounds();
		textRect.OffsetTo(B_ORIGIN);
		textRect.InsetBy(1,1);
		fListingText->SetTextRect(textRect);
	}
}

void
ExpanderWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_SOURCE:
			if (!fSourcePanel)
				fSourcePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 
					B_FILE_NODE, false, NULL, new RuleRefFilter(fRules), true);
			fSourcePanel->Show();
			break;
		case MSG_DEST:
			if (!fDestPanel)
				fDestPanel = new DirectoryFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 
					B_DIRECTORY_NODE, false, NULL, new DirectoryRefFilter(), true);
			fDestPanel->Show();
			break;
		case MSG_DIRECTORY:
			{
				entry_ref ref;
				fDestPanel->GetPanelDirectory(&ref);
				fDestRef = ref;
				BEntry entry(&ref);
				BPath path(&entry);
				fDestText->SetText(path.Path());
				fDestPanel->Hide();
			}
			break;
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			RefsReceived(msg);
			break;
		}
		case MSG_CLOSE:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_EXPAND:
			if (!fExpandingStarted) {
				StartExpanding();
				break;	
			}
		case MSG_STOP:
			if (fExpandingStarted) {
				fExpandingThread->SuspendExternalExpander();
				BAlert *alert = new BAlert("stopAlert", "Are you sure you want to stop expanding this\n"
				"archive? The expanded items may not be complete.", "Stop", "Continue", NULL, 
				B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
				if (alert->Go()==0) {
					fExpandingThread->ResumeExternalExpander();
					StopExpanding();
				} else
					fExpandingThread->ResumeExternalExpander();
			}
			break;
		case MSG_SHOW:
			fShowContents->SetValue(fShowContents->Value() == B_CONTROL_OFF ? B_CONTROL_ON : B_CONTROL_OFF);
			fShowItem->SetLabel(fShowContents->Value() == B_CONTROL_OFF ? "Show Contents" : "Hide Contents");
		case MSG_SHOWCONTENTS:
			if (fShowContents->Value() == B_CONTROL_OFF) {
				if (fListingStarted)
					StopListing();
				SetSizeLimits(450, BScreen().Frame().Width(), 120, 120);
				ResizeTo(Frame().Width(), 120);
				fShowItem->SetLabel("Show Contents");
			} else
				StartListing();
			break;
		case MSG_SOURCETEXT:
			{
				BEntry entry(fSourceText->Text(), true);
				if (!entry.Exists()) {
					BAlert *alert = new BAlert("srcAlert", "The file doesn't exist",
					"Cancel", NULL, NULL, 
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
					alert->Go();
					break;
				}
				
				entry_ref ref;
				entry.GetRef(&ref);
				ExpanderRule *rule = fRules.MatchingRule(&ref);
				if (rule) {
					fSourceChanged = true;
					fSourceRef = ref;
					fShowContents->SetEnabled(true);
					fExpandButton->SetEnabled(true);
					fExpandItem->SetEnabled(true);
					fShowItem->SetEnabled(true);
					break;
				}
				
				BString string = "The file : ";
				string += fSourceText->Text();
				string += " is not supported";
				BAlert *alert = new BAlert("srcAlert", string.String(), 
					"Cancel", NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_INFO_ALERT);
				alert->Go();
				
				fShowContents->SetEnabled(false);
				fExpandButton->SetEnabled(false);
				fExpandItem->SetEnabled(false);
				fShowItem->SetEnabled(false);
			}
			break;
		case MSG_DESTTEXT: 
			{
				BEntry entry(fDestText->Text(), true);
				if (!entry.Exists()) {
					BAlert *alert = new BAlert("destAlert", "The directory was either moved, renamed or not\n"
					"supported.",
					"Cancel", NULL, NULL, 
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
					alert->Go();
					break;
				}
				entry.GetRef(&fDestRef);
			}
			break;
		case MSG_PREFERENCES:
			if (!fPreferences)
				fPreferences = new ExpanderPreferences(&fSettings);
			fPreferences->Show();
			break;
		case 'outp':
			if (!fExpandingStarted && fListingStarted) {
				BString string;
				int32 i=0;
				while (msg->FindString("output", i++, &string)==B_OK)
					fListingText->Insert(string.String());
				fListingText->ScrollToSelection();
			}
			
			break;
		case 'exit':	// thread has finished		(finished, quit, killed, we don't know)
			// reset window state
			if (fExpandingStarted) {
				fExpandedText->SetText("File expanded");
				StopExpanding();
				OpenDestFolder();
				CloseWindowOrKeepOpen();
			} else if (fListingStarted){
				fSourceChanged = false;
				StopListing();
			} else
				fExpandedText->SetText("");
			break;
		case 'exrr':	// thread has finished
			// reset window state
			
			fExpandedText->SetText("Error when expanding archive");
			CloseWindowOrKeepOpen();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
ExpanderWindow::CanQuit()
{
	if ((fSourcePanel && fSourcePanel->IsShowing())
		|| (fDestPanel && fDestPanel->IsShowing()))
		return false;
			
	if (fExpandingStarted) {
		fExpandingThread->SuspendExternalExpander();
		BAlert *alert = new BAlert("stopAlert", "Are you sure you want to stop expanding this\n"
		"archive? The expanded items may not be complete.", "Stop", "Continue", NULL, 
		B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		if (alert->Go()==0) {
			fExpandingThread->ResumeExternalExpander();
			StopExpanding();
		} else {
			fExpandingThread->ResumeExternalExpander();
			return false;
		}
	}
	return true;
}

bool
ExpanderWindow::QuitRequested()
{
	if (!CanQuit())
		return false;
		
	if (fListingStarted) {
		StopListing();
	}
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	fSettings.ReplacePoint("window_position", Frame().LeftTop());
	((ExpanderApp*)(be_app))->UpdateSettingsFrom(&fSettings);
	return true;
}

void 
ExpanderWindow::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	int8 destination_folder = 0x63;
	fSettings.FindInt8("destination_folder", &destination_folder);
	
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		
		BEntry entry(&ref, true);
		BPath path(&entry);
		BNode node(&entry);
		
		if (node.IsFile()) {
			fSourceChanged = true;
			fSourceRef = ref;
			fSourceText->SetText(path.Path());
			if (destination_folder==0x63) {
				BPath parent;
				path.GetParent(&parent);
				fDestText->SetText(parent.Path());
				get_ref_for_path(parent.Path(), &fDestRef);
			} else if (destination_folder==0x65) {
				fSettings.FindRef("destination_folder_use", &fDestRef);
				BEntry dEntry(&fDestRef, true);
				BPath dPath(&dEntry);
				fDestText->SetText(dPath.Path());
			}
			
			BEntry dEntry(&fDestRef, true);
			if (dEntry.Exists()) {
				fExpandButton->SetEnabled(true);
				fExpandItem->SetEnabled(true);
			}
		
			if (fShowContents->Value() == B_CONTROL_ON) {
				StopListing(); 
				StartListing();
			} else {
				fShowContents->SetEnabled(true);
				fShowItem->SetEnabled(true);
			}

			bool fromApp;
			if (msg->FindBool("fromApp", &fromApp)==B_OK) {
				AutoExpand();
			} else
				AutoListing();
		} else if(node.IsDirectory()) {
			fDestRef = ref;
			fDestText->SetText(path.Path());
		}
	}
}

void
ExpanderWindow::StartExpanding()
{
	BMessage message;
	
	ExpanderRule *rule = fRules.MatchingRule(&fSourceRef);
	if (!rule) {
		return;
	}
	
	BEntry destEntry(fDestText->Text(), true);
	if (!destEntry.Exists()) {
		BAlert *alert = new BAlert("destAlert", "The directory was either moved, renamed or not\n"
		"supported.",
		"Cancel", NULL, NULL, 
		B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->Go();
		return;
	}
	
	message.AddString("cmd", rule->ExpandCmd());
	message.AddRef("srcRef", &fSourceRef);
	message.AddRef("destRef", &fDestRef);
	
	fExpandButton->SetLabel("Stop");
	fSourceButton->SetEnabled(false);
	fDestButton->SetEnabled(false);
	fShowContents->SetEnabled(false);
	fSourceItem->SetEnabled(false);
	fDestItem->SetEnabled(false);
	fExpandItem->SetEnabled(false);
	fShowItem->SetEnabled(false);
	fStopItem->SetEnabled(true);
	fPreferencesItem->SetEnabled(false);
	
	BEntry entry(&fSourceRef);
	BPath path(&entry);
	BString text("Expanding file ");
	text.Append(path.Leaf());
	fExpandedText->SetText(text.String());
	
	fExpandingThread = new ExpanderThread(&message, new BMessenger(this));
	fExpandingThread->Start();
	
	fExpandingStarted = true;
}

void
ExpanderWindow::StopExpanding(void)
{
	if (fExpandingThread) {
		fExpandingThread->InterruptExternalExpander();
		fExpandingThread = NULL;
	}

	fExpandingStarted = false;
	
	fExpandButton->SetLabel("Expand");
	fSourceButton->SetEnabled(true);
	fDestButton->SetEnabled(true);
	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(true);
	fDestItem->SetEnabled(true);
	fExpandItem->SetEnabled(true);
	fShowItem->SetEnabled(true);
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(true);
}

void
ExpanderWindow::StartListing()
{
	SetSizeLimits(450, BScreen().Frame().Width(), 335, BScreen().Frame().Height());
	ResizeTo(Frame().Width(), 335);
	fListingScroll->ResizeTo(fListingScroll->Bounds().Width(), 210);
	if (fListingScroll->IsHidden())
		fListingScroll->Show();

	if (!fSourceChanged)
		return;
		
	BMessage message;
	
	ExpanderRule *rule = fRules.MatchingRule(&fSourceRef);
	if (!rule) {
		return;
	}
	message.AddString("cmd", rule->ListingCmd());
	message.AddRef("srcRef", &fSourceRef);
	
	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(false);
	fDestItem->SetEnabled(false);
	fExpandItem->SetEnabled(false);
	fShowItem->SetEnabled(true);
	fShowItem->SetLabel("Hide Contents");
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(false);
	
	fSourceButton->SetEnabled(false);
	fDestButton->SetEnabled(false);
	fExpandButton->SetEnabled(false);
	
	BEntry entry(&fSourceRef);
	BPath path(&entry);
	BString text("Creating listing for ");
	text.Append(path.Leaf());
	fExpandedText->SetText(text.String());
	fListingText->SetText("");
	
	fListingThread = new ExpanderThread(&message, new BMessenger(this));
	fListingThread->Start();

	fListingStarted = true;
}

void
ExpanderWindow::StopListing(void)
{
	if (fListingThread) {
		fListingThread->InterruptExternalExpander();
		fListingThread = NULL;
	}

	fListingStarted = false;
	
	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(true);
	fDestItem->SetEnabled(true);
	fExpandItem->SetEnabled(true);
	fShowItem->SetEnabled(true);
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(true);
	
	fSourceButton->SetEnabled(true);
	fDestButton->SetEnabled(true);
	fExpandButton->SetEnabled(true);
	fExpandedText->SetText("");
}

void
ExpanderWindow::CloseWindowOrKeepOpen()
{
	bool automatically_expand_files = false;
	fSettings.FindBool("automatically_expand_files", &automatically_expand_files);
	bool close_when_done = false;
	fSettings.FindBool("close_when_done", &close_when_done);
	if (automatically_expand_files || close_when_done)
		PostMessage(B_QUIT_REQUESTED);
}

void
ExpanderWindow::OpenDestFolder()
{
	bool open_destination_folder = true;
	fSettings.FindBool("open_destination_folder", &open_destination_folder);

	if (!open_destination_folder)
		return;
		
	BMessage * message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", &fDestRef);
	BPath path(&fDestRef);
	BMessenger tracker( "application/x-vnd.Be-TRAK" );
	tracker.SendMessage( message );
}

void
ExpanderWindow::AutoListing()
{
	bool show_contents_listing = false;
	fSettings.FindBool("show_contents_listing", &show_contents_listing);
	
	if (!show_contents_listing)
		return;
		
	fShowContents->SetValue(B_CONTROL_ON);
	fShowContents->Invoke();
}

void
ExpanderWindow::AutoExpand()
{
	bool automatically_expand_files = false;
	fSettings.FindBool("automatically_expand_files", &automatically_expand_files);

	if (!automatically_expand_files) {
		AutoListing();
		return;
	}
		
	fExpandButton->Invoke();
}
