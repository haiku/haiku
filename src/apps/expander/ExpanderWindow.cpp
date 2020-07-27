/*
 * Copyright 2004-2006, Jérôme DUVAL. All rights reserved.
 * Copyright 2010, Karsten Heimrich. All rights reserved.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ExpanderWindow.h"

#include <algorithm>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Entry.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>

#include "ExpanderApp.h"
#include "ExpanderThread.h"
#include "ExpanderPreferences.h"
#include "PasswordAlert.h"


const uint32 MSG_SOURCE			= 'mSOU';
const uint32 MSG_DEST			= 'mDES';
const uint32 MSG_EXPAND			= 'mEXP';
const uint32 MSG_SHOW			= 'mSHO';
const uint32 MSG_STOP			= 'mSTO';
const uint32 MSG_PREFERENCES	= 'mPRE';
const uint32 MSG_SOURCETEXT		= 'mSTX';
const uint32 MSG_DESTTEXT		= 'mDTX';
const uint32 MSG_SHOWCONTENTS	= 'mSCT';


class StatusView : public BStringView {
public:
	StatusView()
		:
		BStringView(NULL, "")
	{
	}

	virtual ~StatusView()
	{
	}

	void SetStatus(const BString &text)
	{
		fStatus = text;
		Invalidate();
	}

	void Draw(BRect updateRect)
	{
		BString truncated = fStatus;
		if(fStatus.IsEmpty() == false) {
			be_plain_font->TruncateString(&truncated, B_TRUNCATE_END,
				Bounds().Width());
		}

		SetText(truncated);
		BStringView::Draw(updateRect);
	}

private:
	BString fStatus;
};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ExpanderWindow"


ExpanderWindow::ExpanderWindow(BRect frame, const entry_ref* ref,
	BMessage* settings)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Expander"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL),
	fSourcePanel(NULL),
	fDestPanel(NULL),
	fSourceChanged(true),
	fListingThread(NULL),
	fListingStarted(false),
	fExpandingThread(NULL),
	fExpandingStarted(false),
	fSettings(*settings),
	fPreferences(NULL)
{
	_CreateMenuBar();

	fDestButton = new BButton(B_TRANSLATE("Destination"),
		new BMessage(MSG_DEST));
	fSourceButton = new BButton(B_TRANSLATE("Source"),
		new BMessage(MSG_SOURCE));
	fExpandButton = new BButton(B_TRANSLATE("Expand"),
		new BMessage(MSG_EXPAND));

	BSize size = fDestButton->PreferredSize();
	size.width = std::max(size.width, fSourceButton->PreferredSize().width);
	size.width = std::max(size.width, fExpandButton->PreferredSize().width);

	fDestButton->SetExplicitSize(size);
	fSourceButton->SetExplicitSize(size);
	fExpandButton->SetExplicitSize(size);

	fListingText = new BTextView("listingText");
	fListingText->SetText("");
	fListingText->MakeEditable(false);
	fListingText->SetStylable(false);
	fListingText->SetWordWrap(false);
	BFont font = be_fixed_font;
	fListingText->SetFontAndColor(&font);
	fScrollView = new BScrollView("", fListingText,	B_INVALIDATE_AFTER_LAYOUT,
		true, true);

	const float spacing = be_control_look->DefaultItemSpacing();
	BGroupLayout* pathLayout;
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fBar)
		.AddGroup(B_VERTICAL, B_USE_ITEM_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
				.Add(fSourceButton)
				.Add(fSourceText = new BTextControl(NULL, NULL,
					new BMessage(MSG_SOURCETEXT)))
				.End()
			.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
				.Add(fDestButton)
				.Add(fDestText = new BTextControl(NULL, NULL,
					new BMessage(MSG_DESTTEXT)))
				.End()
			.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
				.Add(fExpandButton)
				.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
					.GetLayout(&pathLayout)
					.Add(fShowContents = new BCheckBox(
						B_TRANSLATE("Show contents"),
						new BMessage(MSG_SHOWCONTENTS)))
					.Add(fStatusView = new StatusView())
					.End()
				.End()
			.SetInsets(B_USE_WINDOW_SPACING)
			.End()
		.Add(fScrollView);

	pathLayout->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	size = GetLayout()->View()->PreferredSize();
	fSizeLimit = size.Height() - fScrollView->PreferredSize().height - spacing;

	fStatusView->SetExplicitMinSize(BSize(50.0f, B_SIZE_UNSET));
	fStatusView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	ResizeTo(Bounds().Width(), fSizeLimit);
	SetSizeLimits(size.Width(), 32767.0f, fSizeLimit, fSizeLimit);
	SetZoomLimits(Bounds().Width(), fSizeLimit);
	fPreviousHeight = -1;

	fScrollView->Hide();

	Show();
}


ExpanderWindow::~ExpanderWindow()
{
	if (fDestPanel && fDestPanel->RefFilter())
		delete fDestPanel->RefFilter();

	if (fSourcePanel && fSourcePanel->RefFilter())
		delete fSourcePanel->RefFilter();

	delete fDestPanel;
	delete fSourcePanel;
}


bool
ExpanderWindow::ValidateDest()
{
	BEntry entry(fDestText->Text(), true);
	BVolume volume;
	if (!entry.Exists()) {
		BAlert* alert = new BAlert("destAlert",
			B_TRANSLATE("Destination folder doesn't exist. "
				"Would you like to create it?"),
			B_TRANSLATE("Create"), B_TRANSLATE("Cancel"), NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() != 0)
			return false;

		if (create_directory(fDestText->Text(), 0755) != B_OK) {
			BAlert* alert = new BAlert("stopAlert",
				B_TRANSLATE("Failed to create the destination folder."),
				B_TRANSLATE("Cancel"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}

		BEntry newEntry(fDestText->Text(), true);
		newEntry.GetRef(&fDestRef);
		return true;

	}

	if (!entry.IsDirectory()) {
		BAlert* alert = new BAlert("destAlert",
			B_TRANSLATE("The destination is not a folder."),
			B_TRANSLATE("Cancel"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return false;
	}

	if (entry.GetVolume(&volume) != B_OK || volume.IsReadOnly()) {
		BAlert* alert = new BAlert("destAlert",
			B_TRANSLATE("The destination is read only."),
			B_TRANSLATE("Cancel"), NULL, NULL, B_WIDTH_AS_USUAL,
			B_EVEN_SPACING,	B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return false;
	}

	entry.GetRef(&fDestRef);
	return true;
}


void
ExpanderWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SOURCE:
		{
			BEntry entry(fSourceText->Text(), true);
			entry_ref srcRef;
			if (entry.Exists() && entry.IsDirectory())
				entry.GetRef(&srcRef);
			if (!fSourcePanel) {
				BMessenger messenger(this);
				fSourcePanel = new BFilePanel(B_OPEN_PANEL, &messenger, &srcRef,
					B_FILE_NODE, false, NULL, new RuleRefFilter(fRules), true);
				(fSourcePanel->Window())->SetTitle(
					B_TRANSLATE("Expander: Open"));
			} else
				fSourcePanel->SetPanelDirectory(&srcRef);
			fSourcePanel->Show();
			break;
		}

		case MSG_DEST:
		{
			BEntry entry(fDestText->Text(), true);
			entry_ref destRef;
			if (entry.Exists() && entry.IsDirectory())
				entry.GetRef(&destRef);
			if (!fDestPanel) {
				BMessenger messenger(this);
				fDestPanel = new DirectoryFilePanel(B_OPEN_PANEL, &messenger,
					&destRef, B_DIRECTORY_NODE, false, NULL,
					new DirectoryRefFilter(), true);
			} else
				fDestPanel->SetPanelDirectory(&destRef);
			fDestPanel->Show();
			break;
		}

		case MSG_DIRECTORY:
		{
			entry_ref ref;
			fDestPanel->GetPanelDirectory(&ref);
			fDestRef = ref;
			BEntry entry(&ref);
			BPath path(&entry);
			fDestText->SetText(path.Path());
			fDestPanel->Hide();
			break;
		}

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			RefsReceived(message);
			break;

		case MSG_EXPAND:
			if (!ValidateDest())
				break;
			if (!fExpandingStarted) {
				StartExpanding();
				break;
			}
			// supposed to fall through
		case MSG_STOP:
			if (fExpandingStarted) {
				fExpandingThread->SuspendExternalExpander();
				BAlert* alert = new BAlert("stopAlert",
					B_TRANSLATE("Are you sure you want to stop expanding this "
						"archive? The expanded items may not be complete."),
					B_TRANSLATE("Stop"), B_TRANSLATE("Continue"), NULL,
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				if (alert->Go() == 0) {
					fExpandingThread->ResumeExternalExpander();
					StopExpanding();
				} else
					fExpandingThread->ResumeExternalExpander();
			}
			break;

		case MSG_SHOW:
			fShowContents->SetValue(fShowContents->Value() == B_CONTROL_OFF
				? B_CONTROL_ON : B_CONTROL_OFF);
			// supposed to fall through
		case MSG_SHOWCONTENTS:
			// change menu item label
			fShowItem->SetLabel(fShowContents->Value() == B_CONTROL_OFF
				? B_TRANSLATE("Show contents") : B_TRANSLATE("Hide contents"));

			if (fShowContents->Value() == B_CONTROL_OFF) {
				if (fListingStarted)
					StopListing();

				fScrollView->Hide();
				_UpdateWindowSize(false);
			} else {
				fScrollView->Show();
				StartListing();
			}
			break;

		case MSG_SOURCETEXT:
		{
			BEntry entry(fSourceText->Text(), true);
			if (!entry.Exists()) {
				BAlert* alert = new BAlert("srcAlert",
					B_TRANSLATE("The file doesn't exist"),
					B_TRANSLATE("Cancel"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
				break;
			}

			entry_ref ref;
			entry.GetRef(&ref);
			ExpanderRule* rule = fRules.MatchingRule(&ref);
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
			string += B_TRANSLATE_MARK(" is not supported");
			BAlert* alert = new BAlert("srcAlert", string.String(),
				B_TRANSLATE("Cancel"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_INFO_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();

			fShowContents->SetEnabled(false);
			fExpandButton->SetEnabled(false);
			fExpandItem->SetEnabled(false);
			fShowItem->SetEnabled(false);
			break;
		}

		case MSG_DESTTEXT:
			ValidateDest();
			break;

		case MSG_PREFERENCES:
			if (fPreferences == NULL)
				fPreferences = new ExpanderPreferences(&fSettings);

			fPreferences->Show();
			break;

		case 'outp':
			if (!fExpandingStarted && fListingStarted) {
				// Check if the vertical scroll bar is at the end
				float max, pos;
				fScrollView->ScrollBar(B_VERTICAL)->GetRange(NULL, &max);
				pos = fScrollView->ScrollBar(B_VERTICAL)->Value();
				bool atEnd = (pos == max);

				BString string;
				int32 i = 0;
				while (message->FindString("output", i++, &string) == B_OK) {
					float length = fListingText->StringWidth(string.String());

					if (length > fLongestLine)
						fLongestLine = length;

					fListingText->Insert(string.String());
				}

				if (atEnd && fScrollView->ScrollBar(B_VERTICAL)->Value() == pos) {
					fScrollView->ScrollBar(B_VERTICAL)->GetRange(NULL, &max);
					fScrollView->ScrollBar(B_VERTICAL)->SetValue(max);
				}
			} else if (fExpandingStarted) {
				BString string;
				int32 i = 0;
				while (message->FindString("output", i++, &string) == B_OK) {
					if (strstr(string.String(), "Enter password") != NULL) {
						fExpandingThread->SuspendExternalExpander();
						BString password;
						PasswordAlert* alert =
							new PasswordAlert("passwordAlert", string);
						alert->Go(password);
						fExpandingThread->ResumeExternalExpander();
						fExpandingThread->PushInput(password);
					}
				}
			}
			break;

		case 'errp':
		{
			BString string;
			if (message->FindString("error", &string) == B_OK
				&& fExpandingStarted) {
				fExpandingThread->SuspendExternalExpander();
				if (strstr(string.String(), "password") != NULL) {
					BString password;
					PasswordAlert* alert = new PasswordAlert("passwordAlert",
						string);
					alert->Go(password);
					fExpandingThread->ResumeExternalExpander();
					fExpandingThread->PushInput(password);
				} else {
					BAlert* alert = new BAlert("stopAlert", string,
						B_TRANSLATE("Stop"), B_TRANSLATE("Continue"), NULL,
						B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
					alert->SetShortcut(0, B_ESCAPE);
					if (alert->Go() == 0) {
						fExpandingThread->ResumeExternalExpander();
						StopExpanding();
					} else
						fExpandingThread->ResumeExternalExpander();
				}
			}
			break;
		}

		case 'exit':
			// thread has finished
			// (finished, quit, killed, we don't know)
			// reset window state
			if (fExpandingStarted) {
				fStatusView->SetStatus(B_TRANSLATE("File expanded"));
				StopExpanding();
				OpenDestFolder();
				CloseWindowOrKeepOpen();
			} else if (fListingStarted) {
				fSourceChanged = false;
				StopListing();
				_ExpandListingText();
			} else
				fStatusView->SetStatus("");
			break;

		case 'exrr':
			// thread has finished
			// reset window state

			fStatusView->SetStatus(B_TRANSLATE("Error when expanding archive"));
			CloseWindowOrKeepOpen();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
ExpanderWindow::CanQuit()
{
	if ((fSourcePanel && fSourcePanel->IsShowing())
		|| (fDestPanel && fDestPanel->IsShowing())) {
		return false;
	}

	if (fExpandingStarted) {
		fExpandingThread->SuspendExternalExpander();
		BAlert* alert = new BAlert("stopAlert",
			B_TRANSLATE("Are you sure you want to stop expanding this "
				"archive? The expanded items may not be complete."),
			B_TRANSLATE("Stop"), B_TRANSLATE("Continue"), NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 0) {
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

	if (fListingStarted)
		StopListing();

	be_app->PostMessage(B_QUIT_REQUESTED);
	fSettings.ReplacePoint("window_position", Frame().LeftTop());
	((ExpanderApp*)(be_app))->UpdateSettingsFrom(&fSettings);

	return true;
}


void
ExpanderWindow::RefsReceived(BMessage* message)
{
	entry_ref ref;
	int32 i = 0;
	int8 destinationFolder = 0x63;
	fSettings.FindInt8("destination_folder", &destinationFolder);

	while (message->FindRef("refs", i++, &ref) == B_OK) {
		BEntry entry(&ref, true);
		BPath path(&entry);
		BNode node(&entry);

		if (node.IsFile()) {
			fSourceChanged = true;
			fSourceRef = ref;
			fSourceText->SetText(path.Path());
			if (destinationFolder == 0x63) {
				BPath parent;
				path.GetParent(&parent);
				fDestText->SetText(parent.Path());
				get_ref_for_path(parent.Path(), &fDestRef);
			} else if (destinationFolder == 0x65) {
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
			if (message->FindBool("fromApp", &fromApp) == B_OK) {
				AutoExpand();
			} else
				AutoListing();
		} else if (node.IsDirectory()) {
			fDestRef = ref;
			fDestText->SetText(path.Path());
		}
	}
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ExpanderMenu"

void
ExpanderWindow::_CreateMenuBar()
{
	fBar = new BMenuBar("menu_bar", B_ITEMS_IN_ROW, B_INVALIDATE_AFTER_LAYOUT);
	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(fSourceItem
		= new BMenuItem(B_TRANSLATE("Set source" B_UTF8_ELLIPSIS),
			new BMessage(MSG_SOURCE), 'O'));
	menu->AddItem(fDestItem
		= new BMenuItem(B_TRANSLATE("Set destination" B_UTF8_ELLIPSIS),
			new BMessage(MSG_DEST), 'D'));
	menu->AddSeparatorItem();
	menu->AddItem(fExpandItem = new BMenuItem(B_TRANSLATE("Expand"),
		new BMessage(MSG_EXPAND), 'E'));
	fExpandItem->SetEnabled(false);
	menu->AddItem(fShowItem = new BMenuItem(B_TRANSLATE("Show contents"),
		new BMessage(MSG_SHOW), 'L'));
	fShowItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fStopItem = new BMenuItem(B_TRANSLATE("Stop"),
		new BMessage(MSG_STOP), 'K'));
	fStopItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W'));
	fBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Settings"));
	menu->AddItem(fPreferencesItem
		= new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
			new BMessage(MSG_PREFERENCES), ','));
	fBar->AddItem(menu);
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ExpanderWindow"

void
ExpanderWindow::StartExpanding()
{
	ExpanderRule* rule = fRules.MatchingRule(&fSourceRef);
	if (!rule)
		return;

	BEntry destEntry(fDestText->Text(), true);
	if (!destEntry.Exists()) {
		BAlert* alert = new BAlert("destAlert",
		B_TRANSLATE("The folder was either moved, renamed or not supported."),
		B_TRANSLATE("Cancel"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}

	BMessage message;
	message.AddString("cmd", rule->ExpandCmd());
	message.AddRef("srcRef", &fSourceRef);
	message.AddRef("destRef", &fDestRef);

	fExpandButton->SetLabel(B_TRANSLATE("Stop"));
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
	BString text(B_TRANSLATE("Expanding '%s'" B_UTF8_ELLIPSIS));
	text.ReplaceFirst("%s", path.Leaf());
	fStatusView->SetStatus(text.String());

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

	fExpandButton->SetLabel(B_TRANSLATE("Expand"));
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
ExpanderWindow::_ExpandListingText()
{
	float delta = fLongestLine - fListingText->Frame().Width();

	if (delta > 0) {
		BScreen screen;
		BRect screenFrame = screen.Frame();

		if (Frame().right + delta > screenFrame.right)
			delta = screenFrame.right - Frame().right - 4.0f;

		ResizeBy(delta, 0.0f);
	}

	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	if (minWidth < Frame().Width() + delta) {
		// set the Zoom limit as the minimal required size
		SetZoomLimits(Frame().Width() + delta,
			std::min(fSizeLimit + fListingText->TextRect().Height()
				+ fLineHeight + B_H_SCROLL_BAR_HEIGHT + 1.0f,
				maxHeight));
	} else {
		// set the zoom limit based on minimal window size allowed
		SetZoomLimits(minWidth,
			std::min(fSizeLimit + fListingText->TextRect().Height()
				+ fLineHeight + B_H_SCROLL_BAR_HEIGHT + 1.0f,
				maxHeight));
	}
}


void
ExpanderWindow::_UpdateWindowSize(bool showContents)
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	float bottom = fSizeLimit;

	if (showContents) {
		if (fPreviousHeight < 0.0) {
			BFont font;
			font_height fontHeight;
			fListingText->GetFont(&font);
			font.GetHeight(&fontHeight);
			fLineHeight = ceilf(fontHeight.ascent + fontHeight.descent
				+ fontHeight.leading);
			fPreviousHeight = bottom + 10.0 * fLineHeight;
		}
		minHeight = bottom + 5.0 * fLineHeight;
		maxHeight = 32767.0;

		bottom = std::max(fPreviousHeight, minHeight);
	} else {
		minHeight = fSizeLimit;
		maxHeight = fSizeLimit;
		fPreviousHeight = Frame().Height();
	}

	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	ResizeTo(Frame().Width(), bottom);
}


void
ExpanderWindow::StartListing()
{
	_UpdateWindowSize(true);

	if (!fSourceChanged)
		return;

	fPreviousHeight = -1.0;

	fLongestLine = 0.0f;

	ExpanderRule* rule = fRules.MatchingRule(&fSourceRef);
	if (!rule)
		return;

	BMessage message;
	message.AddString("cmd", rule->ListingCmd());
	message.AddRef("srcRef", &fSourceRef);

	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(false);
	fDestItem->SetEnabled(false);
	fExpandItem->SetEnabled(false);
	fShowItem->SetEnabled(true);
	fShowItem->SetLabel(B_TRANSLATE("Hide contents"));
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(false);

	fSourceButton->SetEnabled(false);
	fDestButton->SetEnabled(false);
	fExpandButton->SetEnabled(false);

	BEntry entry(&fSourceRef);
	BPath path(&entry);
	BString text(B_TRANSLATE("Creating listing for '%s'" B_UTF8_ELLIPSIS));
	text.ReplaceFirst("%s", path.Leaf());
	fStatusView->SetStatus(text.String());
	fListingText->SetText("");
	fListingText->MakeSelectable(false);

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

	fListingText->MakeSelectable(true);
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
	fStatusView->SetStatus("");
}


void
ExpanderWindow::CloseWindowOrKeepOpen()
{
	bool expandFiles = false;
	fSettings.FindBool("automatically_expand_files", &expandFiles);

	bool closeWhenDone = false;
	fSettings.FindBool("close_when_done", &closeWhenDone);

	if (expandFiles || closeWhenDone)
		PostMessage(B_QUIT_REQUESTED);
}


void
ExpanderWindow::OpenDestFolder()
{
	bool openFolder = true;
	fSettings.FindBool("open_destination_folder", &openFolder);

	if (!openFolder)
		return;

	BMessage* message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", &fDestRef);
	BPath path(&fDestRef);
	BMessenger tracker("application/x-vnd.Be-TRAK");
	tracker.SendMessage(message);
}


void
ExpanderWindow::AutoListing()
{
	bool showContents = false;
	fSettings.FindBool("show_contents_listing", &showContents);

	if (!showContents)
		return;

	fShowContents->SetValue(B_CONTROL_ON);
	fShowContents->Invoke();
}


void
ExpanderWindow::AutoExpand()
{
	bool expandFiles = false;
	fSettings.FindBool("automatically_expand_files", &expandFiles);

	if (!expandFiles) {
		AutoListing();
		return;
	}

	fExpandButton->Invoke();
}
