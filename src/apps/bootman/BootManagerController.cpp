/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "BootManagerController.h"

#include "WizardView.h"

#include "EntryPage.h"
#include "PartitionsPage.h"
#include "DefaultPartitionPage.h"
#include "DescriptionPage.h"
#include "FileSelectionPage.h"
#include "LegacyBootDrive.h"


#define USE_TEST_BOOT_DRIVE 0

#if USE_TEST_BOOT_DRIVE
	#include "TestBootDrive.h"
#endif


#include <Alert.h>
#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>


BootManagerController::BootManagerController()
{
#if USE_TEST_BOOT_DRIVE
	fBootDrive = new TestBootDrive();
#else
	fBootDrive = new LegacyBootDrive(); 
#endif

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
	
	fReadPartitionsStatus = fBootDrive->ReadPartitions(&fSettings);
}


BootManagerController::~BootManagerController()
{
}


int32
BootManagerController::InitialState()
{
	if (fReadPartitionsStatus == B_OK)
		return kStateEntry;
	return kStateErrorEntry;
}


int32
BootManagerController::NextState(int32 state)
{
	switch (state) {
		case kStateEntry:
			{
				bool install;
				fSettings.FindBool("install", &install);
				if (install) {
					if (fBootDrive->IsBootMenuInstalled(&fSettings))
						return kStatePartitions;
					else
						return kStateSaveMBR;
				}
				else
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
	for (int32 i = 0; fSettings.FindMessage("partition", i, &message) == B_OK; i ++) {
		bool show;
		if (message.FindBool("show", &show) == B_OK && show)
			return true;	
	}
	
	BAlert* alert = new BAlert("info", "At least one partition must be selected!", 
		"OK");
	alert->Go();
	
	return false;
}


bool
BootManagerController::_WriteBootMenu()
{
		BAlert* alert = new BAlert("confirm", "About to write the boot menu "
			"to disk. Are you sure you want to continue?", "Write Boot Menu", "Back",
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		
		if (alert->Go() == 1)
			return false;
		
		fWriteBootMenuStatus = fBootDrive->WriteBootMenu(&fSettings);
		return true;
}


bool
BootManagerController::_SaveMBR()
{
	BString path;
	fSettings.FindString("file", &path);
	BFile file(path.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	fSaveMBRStatus = fBootDrive->SaveMasterBootRecord(&fSettings, &file);	
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
	message << "About to restore the Master Boot Record (MBR) of "
		<< disk << " from " << path << ". "
		"Do you wish to continue?";
	BAlert* alert = new BAlert("confirm", message.String(), 
		"Restore MBR", "Back",
		NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() == 1)
		return false;

	BFile file(path.String(), B_READ_ONLY);
	fRestoreMBRStatus = fBootDrive->RestoreMasterBootRecord(&fSettings, &file);	
	return true;
}


WizardPageView*
BootManagerController::CreatePage(int32 state, WizardView* wizard)
{
	WizardPageView* page = NULL;
	BRect frame = wizard->PageFrame();
	
	switch (state) {
		case kStateEntry:
			page = new EntryPage(&fSettings, frame, "entry");
			wizard->SetPreviousButtonHidden(true);
			break;
		case kStateErrorEntry:
			page = _CreateErrorEntryPage(frame);
			wizard->SetPreviousButtonHidden(true);
			wizard->SetNextButtonLabel("Done");
			break;
		case kStateSaveMBR:
			page = _CreateSaveMBRPage(frame);
			wizard->SetPreviousButtonHidden(false);
			break;
		case kStateMBRSaved:
			page = _CreateMBRSavedPage(frame);
			break;
		case kStatePartitions:
			page = new PartitionsPage(&fSettings, frame, "partitions");
			wizard->SetPreviousButtonHidden(false);
			break;
		case kStateDefaultPartitions:
			page = new DefaultPartitionPage(&fSettings, frame, "default");
			break;
		case kStateInstallSummary:
			page = _CreateInstallSummaryPage(frame);
			wizard->SetNextButtonLabel("Next");
			break;
		case kStateInstalled:
			page = _CreateInstalledPage(frame);
			wizard->SetNextButtonLabel("Done");
			break;
		case kStateUninstall:
			page = _CreateUninstallPage(frame);
			wizard->SetPreviousButtonHidden(false);
			wizard->SetNextButtonLabel("Next");
			break;
		case kStateUninstalled:
			// TODO prevent overwriting MBR after clicking "Previous"
			page = _CreateUninstalledPage(frame);
			wizard->SetNextButtonLabel("Done");
			break;
	}
	
	return page;
}


WizardPageView*
BootManagerController::_CreateErrorEntryPage(BRect frame)
{
	BString description;

	if (fReadPartitionsStatus == kErrorBootSectorTooSmall)
		description << 
			"Partition Table Not Compatible\n\n"
			"The partition table of the first hard disk is not compatible "
			"with Boot Manager.\n"
			"Boot Manager needs 2 KB available space before the first partition.";	
	else
		description << 
			"Error Reading Partition Table\n\n"
			"Boot Manager is unable to read the partition table!";	
	
	return new DescriptionPage(frame, "errorEntry", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateSaveMBRPage(BRect frame)
{
	BString description;
	BString disk;
	fSettings.FindString("disk", &disk);
	
	description << 
		"Backup Master Boot Record\n\n"
		"The Master Boot Record (MBR) of the boot device:\n"
		"\t" << disk << "\n"
		"will now be saved to disk. Please select a file to "
		"save the MBR into.\n\n"
		"If something goes wrong with the installation or if "
		"you later wish to remove the boot menu, simply run the "
		"bootman program and choose to 'Uninstall' option.";	
	
	return new FileSelectionPage(&fSettings, frame, "saveMBR", description.String(), 
		B_SAVE_PANEL);
}


WizardPageView*
BootManagerController::_CreateMBRSavedPage(BRect frame)
{
	BString description;
	BString file;
	fSettings.FindString("file", &file);
	
	if (fSaveMBRStatus == B_OK) {
		description << 
			"Old Master Boot Record Saved\n\n"
			"The old Master Boot Record was successfully save to "
			<< file << ".\n"; 
	} else {
		description << 
			"Old Master Boot Record Saved Failure\n\n"
			"The old Master Boot Record could not be saved to "
			<< file << ".\n"; 
	}
	
	return new DescriptionPage(frame, "summary", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateInstallSummaryPage(BRect frame)
{
	BString description;
	BString disk;
	fSettings.FindString("disk", &disk);
	
	description << 
		"Summary\n\n"
		"About to write the following boot menu to the boot disk "
		"(" << disk << "). Please verify the information below "
		"before continuing.\n\n";

	BMessage message;
	for (int32 i = 0; fSettings.FindMessage("partition", i, &message) == B_OK; i ++) {
		
		bool show;
		if (message.FindBool("show", &show) != B_OK || !show)
			continue;
		
		BString name;
		BString path;
		message.FindString("name", &name);
		message.FindString("path", &path);
		description << name << "\t(" << path << ")\n";
	}
	
	return new DescriptionPage(frame, "summary", description.String(), true);
}


WizardPageView*
BootManagerController::_CreateInstalledPage(BRect frame)
{
	const char* description;
	
	if (fWriteBootMenuStatus == B_OK) {
		description = "Installation Boot Menu Completed\n\n"
		"The boot manager has been successfully installed "
		"on your system.";
	} else {
		description = "Installation Boot Menu Failed\n\n"
		"An error occured writing the boot menu. The Master Boot Record "
		"might be destroyed, you should restore the MBR now!";
	}
	
	return new DescriptionPage(frame, "done", description, true);
}


WizardPageView*
BootManagerController::_CreateUninstallPage(BRect frame)
{
	BString description;
	
	description << 
		"Uninstall Boot Manager\n\n"
		"Please locate the Master Boot Record (MBR) save file to "
		"restore from. This is the file that was created when the "
		"boot manager was first installed.";
	
	return new FileSelectionPage(&fSettings, frame, "restoreMBR", description.String(),
		B_OPEN_PANEL);
}


WizardPageView* 
BootManagerController::_CreateUninstalledPage(BRect frame)
{
	BString description;
	BString disk;
	BString file;
	fSettings.FindString("disk", &disk);
	fSettings.FindString("file", &file);
	
	if (fRestoreMBRStatus == B_OK) {
		description << 
			"Uninstallation Boot Menu Completed\n\n"
			"The Master Boot Record of the boot device " 
			"(" << disk << ") has been successfully restored from "
			<< file << ".";
	} else {
		description <<
			"Uninstallation Boot Menu Failed\n\n"
			"The Master Boot Record could not be restored!";
	}
	
	return new DescriptionPage(frame, "summary", description.String(), true);
}
