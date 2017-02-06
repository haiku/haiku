/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ResultWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverRepository.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <ViewPort.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageResult"

using namespace BPackageKit;


static const uint32 kApplyMessage = 'rtry';


ResultWindow::ResultWindow()
	:
	BWindow(BRect(0, 0, 400, 300), B_TRANSLATE_COMMENT("Package changes",
			"Window title"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES),
	fDoneSemaphore(-1),
	fClientWaiting(false),
	fAccepted(false),
	fContainerView(NULL),
	fCancelButton(NULL),
	fApplyButton(NULL)

{
	fDoneSemaphore = create_sem(0, "package changes");
	if (fDoneSemaphore < 0)
		throw std::bad_alloc();

	BStringView* topTextView = NULL;
	BViewPort* viewPort = NULL;

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_SMALL_INSETS)
		.Add(topTextView = new BStringView(NULL, B_TRANSLATE(
				"The following additional package changes have to be made:")))
		.Add(new BScrollView(NULL, viewPort = new BViewPort(), 0, false, true))
		.AddGroup(B_HORIZONTAL)
			.Add(fCancelButton = new BButton(B_TRANSLATE("Cancel"),
				new BMessage(B_CANCEL)))
			.AddGlue()
			.Add(fApplyButton = new BButton(B_TRANSLATE("Apply changes"),
				new BMessage(kApplyMessage)))
		.End();

	topTextView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	viewPort->SetChildView(fContainerView = new BGroupView(B_VERTICAL, 0));

	// set small scroll step (large step will be set by the view port)
	font_height fontHeight;
	topTextView->GetFontHeight(&fontHeight);
	float smallStep = ceilf(fontHeight.ascent + fontHeight.descent);
	viewPort->ScrollBar(B_VERTICAL)->SetSteps(smallStep, smallStep);
}


ResultWindow::~ResultWindow()
{
	if (fDoneSemaphore >= 0)
		delete_sem(fDoneSemaphore);
}


bool
ResultWindow::AddLocationChanges(const char* location,
	const PackageList& packagesToInstall,
	const PackageSet& packagesAlreadyAdded,
	const PackageList& packagesToUninstall,
	const PackageSet& packagesAlreadyRemoved)
{
	BGroupView* locationGroup = new BGroupView(B_VERTICAL);
	ObjectDeleter<BGroupView> locationGroupDeleter(locationGroup);

	locationGroup->GroupLayout()->SetInsets(B_USE_SMALL_INSETS);

	float backgroundTint = B_NO_TINT;
	if ((fContainerView->CountChildren() & 1) != 0)
		backgroundTint = 1.04;
	locationGroup->SetViewUIColor(B_LIST_BACKGROUND_COLOR, backgroundTint);

	BStringView* locationView = new BStringView(NULL,
		BString().SetToFormat("in %s:", location));
	locationGroup->AddChild(locationView);
	locationView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	locationView->AdoptParentColors();
	BFont locationFont;
	locationView->GetFont(&locationFont);
	locationFont.SetFace(B_BOLD_FACE);
	locationView->SetFont(&locationFont);

	BGroupLayout* packagesGroup = new BGroupLayout(B_VERTICAL);
	locationGroup->GroupLayout()->AddItem(packagesGroup);
	packagesGroup->SetInsets(20, 0, 0, 0);

	bool packagesAdded = _AddPackages(packagesGroup, packagesToInstall,
		packagesAlreadyAdded, true);
	packagesAdded |= _AddPackages(packagesGroup, packagesToUninstall,
		packagesAlreadyRemoved, false);

	if (!packagesAdded)
		return false;

	fContainerView->AddChild(locationGroup);
	locationGroupDeleter.Detach();

	return true;
}


bool
ResultWindow::Go()
{
	AutoLocker<ResultWindow> locker(this);

	CenterOnScreen();
	Show();

	fAccepted = false;
	fClientWaiting = true;

	locker.Unlock();

	while (acquire_sem(fDoneSemaphore) == B_INTERRUPTED) {
	}

	locker.Lock();
	bool result = false;
	if (locker.IsLocked()) {
		result = fAccepted;
		Quit();
		locker.Detach();
	} else
		PostMessage(B_QUIT_REQUESTED);

	return result;
}


bool
ResultWindow::QuitRequested()
{
	if (fClientWaiting) {
		Hide();
		fClientWaiting = false;
		release_sem(fDoneSemaphore);
		return false;
	}

	return true;
}


void
ResultWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CANCEL:
		case kApplyMessage:
			Hide();
			fAccepted = message->what == kApplyMessage;
			fClientWaiting = false;
			release_sem(fDoneSemaphore);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ResultWindow::_AddPackages(BGroupLayout* packagesGroup,
	const PackageList& packages, const PackageSet& ignorePackages, bool install)
{
	bool packagesAdded = false;

	for (int32 i = 0; BSolverPackage* package = packages.ItemAt(i);
		i++) {
		if (ignorePackages.find(package) != ignorePackages.end())
			continue;

		BString text;
		if (install) {
			text.SetToFormat(B_TRANSLATE_COMMENT("install package %s from "
					"repository %s\n", "Don't change '%s' variables"),
				package->Info().FileName().String(),
				package->Repository()->Name().String());
		} else {
			text.SetToFormat(B_TRANSLATE_COMMENT("uninstall package %s\n",
					"Don't change '%s' variable"),
				package->VersionedName().String());
		}

		BStringView* packageView = new BStringView(NULL, text);
		packagesGroup->AddView(packageView);
		packageView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		packageView->AdoptParentColors();

		packagesAdded = true;
	}

	return packagesAdded;
}
