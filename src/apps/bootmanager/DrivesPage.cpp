/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "DrivesPage.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <DiskDeviceRoster.h>
#include <DiskDevice.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <TextView.h>

#include <StringForSize.h>

#include "BootDrive.h"
#include "WizardView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DrivesPage"


const uint32 kMsgSelectionChanged = 'slch';


class DriveItem : public BListItem {
public:
								DriveItem(const BDiskDevice& device);
	virtual						~DriveItem();

			bool				IsInstalled() const;
			bool				CanBeInstalled() const;
			bool				IsBootDrive() const;
			const char*			Path() const { return fPath.Path(); }

			BMessage&			Settings() { return fSettings; }
			BootDrive*			Drive() { return fDrive; }

protected:
	virtual void				DrawItem(BView* owner, BRect frame,
									bool complete = false);
	virtual	void				Update(BView* owner, const BFont* font);

private:
			BMessage			fSettings;
			BootDrive*			fDrive;
			BString				fName;
			BPath				fPath;
			BString				fSize;
			float				fBaselineOffset;
			float				fSecondBaselineOffset;
			float				fSizeWidth;
};


DriveItem::DriveItem(const BDiskDevice& device)
	:
	fBaselineOffset(0),
	fSizeWidth(0)
{
	// TODO: retrieve disk name!
	if (device.Name() != NULL && device.Name()[0])
		fName = device.Name();
	else
		fName = B_TRANSLATE_COMMENT("Hard Drive", "Default disk name");

	device.GetPath(&fPath);

	char buffer[256];
	fSize = string_for_size(device.Size(), buffer, sizeof(buffer));
}


DriveItem::~DriveItem()
{
}


bool
DriveItem::IsInstalled() const
{
	return true;
}


bool
DriveItem::CanBeInstalled() const
{
	return true;
}


bool
DriveItem::IsBootDrive() const
{
	return true;
}


void
DriveItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	owner->PushState();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			owner->SetHighColor(tint_color(owner->LowColor(), B_DARKEN_2_TINT));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(owner->LowColor());

		owner->FillRect(frame);
	}

	rgb_color black = {0, 0, 0, 255};

	if (!IsEnabled())
		owner->SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	else
		owner->SetHighColor(black);

	// device
	owner->MovePenTo(frame.left + 4, frame.top + fSecondBaselineOffset);
	owner->DrawString(fPath.Path());

	// size
	owner->MovePenTo(frame.right - 4 - fSizeWidth, frame.top + fBaselineOffset);
	owner->DrawString(fSize.String());

	// name
	BFont boldFont;
	owner->GetFont(&boldFont);
	boldFont.SetFace(B_BOLD_FACE);
	owner->SetFont(&boldFont);

	owner->MovePenTo(frame.left + 4, frame.top + fBaselineOffset);
	owner->DrawString(fName.String());

	owner->PopState();
}


void
DriveItem::Update(BView* owner, const BFont* font)
{
	fSizeWidth = font->StringWidth(fSize.String());

	BFont boldFont(font);
	boldFont.SetFace(B_BOLD_FACE);
	float width = 8 + boldFont.StringWidth(fPath.Path())
		+ be_control_look->DefaultItemSpacing() + fSizeWidth;
	float pathWidth = font->StringWidth(fPath.Path());
	if (width < pathWidth)
		width = pathWidth;

	SetWidth(width);

	font_height fheight;
	font->GetHeight(&fheight);

	float lineHeight = ceilf(fheight.ascent) + ceilf(fheight.descent)
		+ ceilf(fheight.leading);

	fBaselineOffset = 2 + ceilf(fheight.ascent + fheight.leading / 2);
	fSecondBaselineOffset = fBaselineOffset + lineHeight;

	SetHeight(2 * lineHeight + 4);
}


// #pragma mark -


DrivesPage::DrivesPage(WizardView* wizardView, BMessage* settings,
	const char* name)
	:
	WizardPageView(settings, name),
	fWizardView(wizardView)
{
	BString text;
	text << B_TRANSLATE_COMMENT("Drives", "Title") << "\n"
		<< B_TRANSLATE("Please select the drive you want the boot manager to "
			"be installed to or uninstalled from.");
	BTextView* description = CreateDescription("description", text);
	MakeHeading(description);

	fDrivesView = new BListView("drives");
	fDrivesView->SetSelectionMessage(new BMessage(kMsgSelectionChanged));

	bool any = _FillDrivesView();

	BScrollView* scrollView = new BScrollView("scrollView", fDrivesView, 0,
		false, true);

	SetLayout(new BGroupLayout(B_VERTICAL));

	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.Add(description, 0.5)
		.Add(scrollView, 1);

	if (any) {
		fWizardView->SetPreviousButtonLabel(
			B_TRANSLATE_COMMENT("Uninstall", "Button"));
		fWizardView->SetPreviousButtonHidden(false);
	} else {
		fWizardView->SetPreviousButtonHidden(true);
		fWizardView->SetNextButtonLabel(
			B_TRANSLATE_COMMENT("Quit", "Button"));
	}
}


DrivesPage::~DrivesPage()
{
}


void
DrivesPage::PageCompleted()
{
	DriveItem* item = _SelectedDriveItem();

	if (fSettings->ReplaceString("disk", item->Path()) != B_OK)
		fSettings->AddString("disk", item->Path());

	fSettings->ReplaceBool("install", true);
}


void
DrivesPage::AttachedToWindow()
{
	fDrivesView->SetTarget(this);
}


void
DrivesPage::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSelectionChanged:
		{
			_UpdateWizardButtons(_SelectedDriveItem());
			break;
		}

		default:
			WizardPageView::MessageReceived(message);
			break;
	}
}


/*!	Builds the list view items, and adds them to fDriveView.
	Returns whether there were any possible install targets. Automatically
	selects the boot drive.
*/
bool
DrivesPage::_FillDrivesView()
{
	bool any = false;

	BDiskDeviceRoster roster;
	BDiskDevice device;
	while (roster.GetNextDevice(&device) == B_OK) {
		if (device.HasMedia() && !device.IsReadOnly()) {
			DriveItem* item = new DriveItem(device);
			if (item->CanBeInstalled())
				any = true;
			fDrivesView->AddItem(item);
			if (item->IsBootDrive()) {
				fDrivesView->Select(fDrivesView->CountItems() - 1);
				_UpdateWizardButtons(item);
			}
		}
	}

	return any;
}


DriveItem*
DrivesPage::_SelectedDriveItem()
{
	return (DriveItem*)fDrivesView->ItemAt(fDrivesView->CurrentSelection());
}


void
DrivesPage::_UpdateWizardButtons(DriveItem* item)
{
	fWizardView->SetPreviousButtonEnabled(
		item->CanBeInstalled() && item->IsInstalled());
	fWizardView->SetNextButtonEnabled(item->CanBeInstalled());

	fWizardView->SetNextButtonLabel(
		item->IsInstalled() && item->CanBeInstalled()
			? B_TRANSLATE_COMMENT("Update", "Button")
			: B_TRANSLATE_COMMENT("Install", "Button"));
}
