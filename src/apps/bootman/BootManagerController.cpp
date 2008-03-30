/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BootManagerController.h"

#include "WizardView.h"

#include "EntryPage.h"
#include "PartitionsPage.h"
#include "DefaultPartitionPage.h"
#include "DescriptionPage.h"
#include "FileSelectionPage.h"

// TODO remove
#define USE_TEST_BOOT_DRIVE 1

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
	// TODO implement
	fBootDrive = NULL; 
#endif

	// set defaults
	fSettings.AddBool("install", true);
	fSettings.AddInt32("defaultPartition", 0);
	fSettings.AddInt32("timeout", -1);
	
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append("bootman/MBR");
		fSettings.AddString("file", path.Path());		
	} else {
		fSettings.AddString("file", "");
	}
	
	fBootDrive->ReadPartitions(&fSettings);
}


BootManagerController::~BootManagerController()
{
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
				bool install;
				fSettings.FindBool("install", &install);
				if (install) {
					if (fBootDrive->IsBootMenuInstalled())
						return kStatePartitions;
					else
						return kStateSaveMBR;
				}
				else
					return kStateUninstall;
			}
		
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
		
		status_t status = fBootDrive->WriteBootMenu(&fSettings);
		fSettings.RemoveName("installStatus");
		fSettings.AddInt32("installStatus", status);
		
		return true;
}


bool
BootManagerController::_SaveMBR()
{
	BFile file("", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	status_t state = fBootDrive->SaveMasterBootRecord(&file);	
	fSettings.RemoveName("saveMBRStatus");
	fSettings.AddInt32("saveMBRStatus", state);

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
	status_t state = fBootDrive->RestoreMasterBootRecord(&file);	
	fSettings.RemoveName("restoreMBRStatus");
	fSettings.AddInt32("restoreMBRStatus", state);
	
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
	
	FileSelectionPage* page = new FileSelectionPage(&fSettings, frame, "saveMBR", description.String());
	return page;
}


WizardPageView*
BootManagerController::_CreateMBRSavedPage(BRect frame)
{
	BString description;
	BString file;
	fSettings.FindString("file", &file);
	
	description << 
		"Old Master Boot Record Saved\n\n"
		"The old Master Boot Record was successfully save to "
		<< file << ".\n"; 
	
	DescriptionPage* page = new DescriptionPage(frame, "summary", description.String(), true);
	return page;
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
	
	DescriptionPage* page = new DescriptionPage(frame, "summary", description.String(), true);
	return page;
}


WizardPageView*
BootManagerController::_CreateInstalledPage(BRect frame)
{
	const char* description;
	
	int32 status;
	fSettings.FindInt32("installStatus", &status);
	if (status == B_OK) {
		description = "Installation Boot Menu Completed\n\n"
		"The boot manager has been successfully installed "
		"on your system.";
	} else {
		// TODO
		description = "Installation Boot Menu Failed\n\n"
		"TODO";
	}
	
	DescriptionPage* page = new DescriptionPage(frame, "done", description, true);
	return page;	
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
	
	FileSelectionPage* page = new FileSelectionPage(&fSettings, frame, "restoreMBR", description.String());
	return page;
}


WizardPageView* 
BootManagerController::_CreateUninstalledPage(BRect frame)
{
	BString description;
	BString disk;
	BString file;
	fSettings.FindString("disk", &disk);
	fSettings.FindString("file", &file);
	
	int32 status;
	fSettings.FindInt32("restoreMBRStatus", &status);
	if (status == B_OK) {
		description << 
			"Uninstallation Boot Menu Completed\n\n"
			"The Master Boot Record of the boot device " 
			"(" << disk << ") has been successfully restored from "
			<< file << ".";
	} else {
		// TODO
		description <<
			"Uninstallation Boot Menu Failed\n\n"
			"TODO";
	}
	
	DescriptionPage* page = new DescriptionPage(frame, "summary", description.String(), true);
	return page;

}
