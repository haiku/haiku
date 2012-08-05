/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "BootManagerController.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>
#include <String.h>

#include "BootDrive.h"
#include "DefaultPartitionPage.h"
#include "DescriptionPage.h"
#include "DrivesPage.h"
#include "FileSelectionPage.h"
#include "LegacyBootMenu.h"
#include "PartitionsPage.h"
#include "WizardView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BootManagerController"


BootManagerController::BootManagerController()
	:
	fBootDrive(NULL),
	fBootMenu(NULL)
{
	// set defaults
	fSettings.AddBool("install", true);
	fSettings.AddInt32("defaultPartition", 0);
	fSettings.AddInt32("timeout", -1);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append("bootman/MBR");
		fSettings.AddString("file", path.Path());
		// create directory
		BPath parent;
		if (path.GetParent(&parent) == B_OK) {
			BDirectory directory;
			directory.CreateDirectory(parent.Path(), NULL);
		}
	} else {
		fSettings.AddString("file", "");
	}

	// That's the only boot menu we support at the moment.
	fBootMenus.AddItem(new LegacyBootMenu());
}


BootManagerController::~BootManagerController()
{
}


void
BootManagerController::Previous(WizardView* wizard)
{
	if (CurrentState() != kStateEntry)
		WizardController::Previous(wizard);
	else {
		fSettings.ReplaceBool("install", false);
		WizardController::Next(wizard);
	}
}


int32
BootManagerController::InitialState()
{
	return kStateEntry;
}


int32
BootManagerController::NextState(int32 state)
{
	switch (state) {
		case kStateEntry:
		{
			const char* path;
			if (fSettings.FindString("disk", &path) != B_OK)
				return kStateErrorEntry;

			delete fBootDrive;

			fBootDrive = new BootDrive(path);
			fBootMenu = fBootDrive->InstalledMenu(fBootMenus);

			if (fSettings.FindBool("install")) {
				int32 nextState = fBootMenu != NULL
					? kStatePartitions : kStateSaveMBR;

				// TODO: call BootDrive::AddSupportedMenus() once we support
				// more than one type of boot menu - we'll probably need a
				// requester to choose from them then as well.
				if (fBootMenu == NULL)
					fBootMenu = fBootMenus.ItemAt(0);

				fCollectPartitionsStatus = fBootMenu->CollectPartitions(
					*fBootDrive, fSettings);

				return nextState;
			}

			return kStateUninstall;
		}

		case kStateErrorEntry:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kStateSaveMBR:
			if (_SaveMBR())
				return kStateMBRSaved;
			break;

		case kStateMBRSaved:
			return kStatePartitions;

		case kStatePartitions:
			if (_HasSelectedPartitions())
				return kStateDefaultPartitions;
			break;

		case kStateDefaultPartitions:
			return kStateInstallSummary;

		case kStateInstallSummary:
			if (_WriteBootMenu())
				return kStateInstalled;
			break;

		case kStateInstalled:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kStateUninstall:
			if (_RestoreMBR())
				return kStateUninstalled;
			break;

		case kStateUninstalled:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
	}
	// cannot leave the current state/page
	return -1;
}


bool
BootManagerController::_HasSelectedPartitions()
{
	BMessage message;
	for (int32 i = 0; fSettings.FindMessage("partition", i, &message) == B_OK;
			i++) {
		bool show;
		if (message.FindBool("show", &show) == B_OK && show)
			return true;
	}

	BAlert* alert = new BAlert("info",
		B_TRANSLATE("At least one partition must be selected!"),
		B_TRANSLATE_COMMENT("OK", "Button"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();

	return false;
}


bool
BootManagerController::_WriteBootMenu()
{
	BAlert* alert = new BAlert("confirm", B_TRANSLATE("About to write the "
			"boot menu to disk. Are you sure you want to continue?"),
		B_TRANSLATE_COMMENT("Write boot menu", "Button"),
		B_TRANSLATE_COMMENT("Back", "Button"), NULL, B_WIDTH_AS_USUAL,
		B_WARNING_ALERT);

	if (alert->Go() == 1)
		return false;

	fWriteBootMenuStatus = fBootMenu->Install(*fBootDrive, fSettings);
	return true;
}


bool
BootManagerController::_SaveMBR()
{
	BString path;
	fSettings.FindString("file", &path);
	BFile file(path.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	fSaveMBRStatus = fBootMenu->SaveMasterBootRecord(&fSettings, &file);
	return true;
}


bool
BootManagerController::_RestoreMBR()
{
	BString disk;
	BString path;
	fSettings.FindString("disk", &disk);
	fSettings.FindString("file", &path);

	BString message;
	message << B_TRANSLATE_COMMENT("About to restore the Master Boot Record "
		"(MBR) of %disk from %file. Do you wish to continue?",
		"Don't translate the place holders: %disk and %file");
	message.ReplaceFirst("%disk", disk);
	message.ReplaceFirst("%file", path);

	BAlert* alert = new BAlert("confirm", message.String(),
		B_TRANSLATE_COMMENT("Restore MBR", "Button"),
		B_TRANSLATE_COMMENT("Back", "Button"),
		NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() == 1)
		return false;

	BFile file(path.String(), B_READ_ONLY);
	fRestoreMBRStatus = fBootMenu->RestoreMasterBootRecord(&fSettings, &file);
	return true;
}


WizardPageView*
BootManagerController::CreatePage(int32 state, WizardView* wizard)
{
	WizardPageView* page = NULL;
	BRect frame(0, 0, 300, 250);

	switch (state) {
		case kStateEntry:
			fSettings.ReplaceBool("install", true);
			page = new DrivesPage(wizard, fBootMenus, &fSettings, "drives");
			break;
		case kStateErrorEntry:
			page = _CreateErrorEntryPage();
			wizard->SetPreviousButtonHidden(true);
			wizard->SetNextButtonLabel(B_TRANSLATE_COMMENT("Done", "Button"));
			break;
		case kStateSaveMBR:
			page = _CreateSaveMBRPage(frame);
			wizard->SetPreviousButtonHidden(false);
			break;
		case kStateMBRSaved:
			page = _CreateMBRSavedPage();
			break;
		case kStatePartitions:
			page = new PartitionsPage(&fSettings, "partitions");
			wizard->SetPreviousButtonHidden(false);
			break;
		case kStateDefaultPartitions:
			page = new DefaultPartitionPage(&fSettings, frame, "default");
			break;
		case kStateInstallSummary:
			page = _CreateInstallSummaryPage();
			break;
		case kStateInstalled:
			page = _CreateInstalledPage();
			wizard->SetNextButtonLabel(B_TRANSLATE_COMMENT("Done", "Button"));
			break;
		case kStateUninstall:
			page = _CreateUninstallPage(frame);
			wizard->SetPreviousButtonHidden(false);
			break;
		case kStateUninstalled:
			// TODO prevent overwriting MBR after clicking "Previous"
			page = _CreateUninstalledPage();
			wizard->SetNextButtonLabel(B_TRANSLATE_COMMENT("Done", "Button"));
			break;
	}

	return page;
}


WizardPageView*
BootManagerController::_CreateErrorEntryPage()
{
	BString description;

	if (fCollectPartitionsStatus == B_PARTITION_TOO_SMALL) {
		description << B_TRANSLATE_COMMENT("Partition table not compatible",
				"Title") << "\n\n"
			<< B_TRANSLATE("The partition table of the first hard disk is not "
				"compatible with Boot Manager.\n"
				"Boot Manager needs 2 KB available space before the first "
				"partition.");
	} else {
		description << B_TRANSLATE_COMMENT("Error reading partition table",
				"Title") << "\n\n"
			<< B_TRANSLATE("Boot Manager is unable to read the partition "
				"table!");
	}

	return new DescriptionPage("errorEntry", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateSaveMBRPage(BRect frame)
{
	BString description;
	BString disk;
	fSettings.FindString("disk", &disk);

	description << B_TRANSLATE_COMMENT("Backup Master Boot Record", "Title")
		<< "\n" << B_TRANSLATE("The Master Boot Record (MBR) of the boot "
			"device:\n"
			"\t%s\n"
			"will now be saved to disk. Please select a file to "
			"save the MBR into.\n\n"
			"If something goes wrong with the installation or if "
			"you later wish to remove the boot menu, simply run the "
			"bootman program and choose the 'Uninstall' option.");
	description.ReplaceFirst("%s", disk);

	return new FileSelectionPage(&fSettings, frame, "saveMBR",
		description.String(),
		B_SAVE_PANEL);
}


WizardPageView*
BootManagerController::_CreateMBRSavedPage()
{
	BString description;
	BString file;
	fSettings.FindString("file", &file);

	if (fSaveMBRStatus == B_OK) {
		description << B_TRANSLATE_COMMENT("Old Master Boot Record saved",
				"Title") << "\n"
			<< B_TRANSLATE("The old Master Boot Record was successfully "
				"saved to %s.") << "\n";
	} else {
		description << B_TRANSLATE_COMMENT("Old Master Boot Record Saved "
				"failure", "Title") << "\n"
			<< B_TRANSLATE("The old Master Boot Record could not be saved "
				"to %s") << "\n";
	}
	description.ReplaceFirst("%s", file);

	return new DescriptionPage("summary", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateInstallSummaryPage()
{
	BString description;
	BString disk;
	fSettings.FindString("disk", &disk);

	description << B_TRANSLATE_COMMENT("Summary", "Title") << "\n"
		<< B_TRANSLATE("About to write the following boot menu to the boot "
			"disk (%s). Please verify the information below before continuing.")
		<< "\n\n";
	description.ReplaceFirst("%s", disk);

	BMessage message;
	for (int32 i = 0; fSettings.FindMessage("partition", i, &message) == B_OK;
			i++) {
		bool show;
		if (message.FindBool("show", &show) != B_OK || !show)
			continue;

		BString name;
		BString path;
		message.FindString("name", &name);
		message.FindString("path", &path);

		BString displayName;
		if (fBootMenu->GetDisplayText(name.String(), displayName) == B_OK)
			description << displayName << "\t(" << path << ")\n";
		else
			description << name << "\t(" << path << ")\n";
	}

	return new DescriptionPage("summary", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateInstalledPage()
{
	BString description;

	if (fWriteBootMenuStatus == B_OK) {
		description << B_TRANSLATE_COMMENT("Installation of boot menu "
				"completed", "Title") << "\n"
			<< B_TRANSLATE("The boot manager has been successfully installed "
				"on your system.");
	} else {
		description << B_TRANSLATE_COMMENT("Installation of boot menu failed",
				"Title") << "\n"
			<< B_TRANSLATE("An error occurred writing the boot menu. "
				"The Master Boot Record might be destroyed, "
				"you should restore the MBR now!");
	}

	return new DescriptionPage("done", description, true);
}


WizardPageView*
BootManagerController::_CreateUninstallPage(BRect frame)
{
	BString description;
	description << B_TRANSLATE_COMMENT("Uninstall boot manager", "Title")
		<< "\n\n"
		<< B_TRANSLATE("Please locate the Master Boot Record (MBR) save file "
			"to restore from. This is the file that was created when the "
			"boot manager was first installed.");

	return new FileSelectionPage(&fSettings, frame, "restoreMBR",
		description.String(), B_OPEN_PANEL);
}


WizardPageView*
BootManagerController::_CreateUninstalledPage()
{
	BString description;
	BString disk;
	BString file;
	fSettings.FindString("disk", &disk);
	fSettings.FindString("file", &file);

	if (fRestoreMBRStatus == B_OK) {
		description << B_TRANSLATE_COMMENT("Uninstallation of boot menu "
				"completed", "Title") << "\n"
			<< B_TRANSLATE("The Master Boot Record of the boot device "
				"(%DISK) has been successfully restored from %FILE.");
		description.ReplaceFirst("%DISK", disk);
		description.ReplaceFirst("%FILE", file);
	} else {
		description << B_TRANSLATE_COMMENT("Uninstallation of boot menu "
				"failed", "Title") << "\n"
			<< B_TRANSLATE("The Master Boot Record could not be restored!");
	}

	return new DescriptionPage("summary", description.String(), true);
}
