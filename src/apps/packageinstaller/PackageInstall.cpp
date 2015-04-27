/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageInstall.h"

#include "InstalledPackageInfo.h"
#include "PackageItem.h"
#include "PackageView.h"

#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInstall"


static int32
install_function(void* data)
{
	// TODO: Inform if already one thread is running
	if (data == NULL)
		return -1;

	PackageInstall* install = static_cast<PackageInstall*>(data);
	install->Install();
	return 0;
}


PackageInstall::PackageInstall(PackageView* parent)
	:
	fParent(parent),
	fThreadId(-1),
	fCurrentScript(NULL)
{
}


PackageInstall::~PackageInstall()
{
}


status_t
PackageInstall::Start()
{
	status_t ret = B_OK;

	fIdLocker.Lock();
	if (fThreadId > -1) {
		ret = B_BUSY;
	} else {
		fThreadId = spawn_thread(install_function, "install_package",
			B_NORMAL_PRIORITY, this);
		resume_thread(fThreadId);
	}
	fIdLocker.Unlock();

	return ret;
}


void
PackageInstall::Stop()
{
	// TODO: Argh! No killing of threads!! That leaks resources which they
	// allocated. Rather inform them they need to quit, which they do at the
	// next convenient time, then use wait_for_thread() here.
	fIdLocker.Lock();
	if (fThreadId > -1) {
		kill_thread(fThreadId);
		fThreadId = -1;
	}
	fIdLocker.Unlock();

	fCurrentScriptLocker.Lock();
	if (fCurrentScript != NULL) {
		thread_id id = fCurrentScript->GetThreadId();
		if (id > -1) {
			fCurrentScript->SetThreadId(-1);
			kill_thread(id);
		}
		fCurrentScript = NULL;
	}
	fCurrentScriptLocker.Unlock();
}


void
PackageInstall::Install()
{
	// A message sending wrapper around _Install()
	uint32 code = _Install();

	BMessenger messenger(fParent);
	if (messenger.IsValid()) {
		BMessage message(code);
		messenger.SendMessage(&message);
	}
}


static inline BString
get_item_progress_string(uint32 index, uint32 total) 
{
	BString label(B_TRANSLATE("%index% of %total%"));
	BString indexString;
	indexString << (index + 1);
	BString totalString;
	totalString << total;
	label.ReplaceAll("%index%", indexString);
	label.ReplaceAll("%total%", totalString);
	return label;
}


uint32
PackageInstall::_Install()
{
	PackageInfo* info = fParent->GetPackageInfo();
	pkg_profile* type = static_cast<pkg_profile*>(info->GetProfile(
		fParent->CurrentType()));
	uint32 n = type->items.CountItems();
	uint32 m = info->GetScriptCount();

	PackageStatus* progress = fParent->StatusWindow();
	progress->Reset(n + m + 5);

	progress->StageStep(1, B_TRANSLATE("Preparing package"));

	InstalledPackageInfo packageInfo(info->GetName(), info->GetVersion());

	status_t err = packageInfo.InitCheck();
	if (err == B_OK) {
		// The package is already installed, inform the user
		BAlert* reinstall = new BAlert("reinstall",
			B_TRANSLATE("The given package seems to be already installed on "
				"your system. Would you like to uninstall the existing one "
				"and continue the installation?"),
			B_TRANSLATE("Continue"),
			B_TRANSLATE("Abort"));
		reinstall->SetShortcut(1, B_ESCAPE);

		if (reinstall->Go() == 0) {
			// Uninstall the package
			err = packageInfo.Uninstall();
			if (err != B_OK) {
				fprintf(stderr, "Error uninstalling previously installed "
					"package: %s\n", strerror(err));
				// Ignore error
			}

			err = packageInfo.SetTo(info->GetName(), info->GetVersion(), true);
			if (err != B_OK) {
				fprintf(stderr, "Error marking installation of package: "
					"%s\n", strerror(err));
				return P_MSG_I_ERROR;
			}
		} else {
			// Abort the installation
			return P_MSG_I_ABORT;
		}
	} else if (err == B_ENTRY_NOT_FOUND) {
		err = packageInfo.SetTo(info->GetName(), info->GetVersion(), true);
		if (err != B_OK) {
				fprintf(stderr, "Error marking installation of package: "
					"%s\n", strerror(err));
			return P_MSG_I_ERROR;
		}
	} else if (progress->Stopped()) {
		return P_MSG_I_ABORT;
	} else {
		fprintf(stderr, "returning on error\n");
		return P_MSG_I_ERROR;
	}

	progress->StageStep(1, B_TRANSLATE("Installing files and folders"));

	// Install files and directories

	packageInfo.SetName(info->GetName());
	// TODO: Here's a small problem, since right now it's not quite sure
	//		which description is really used as such. The one displayed on
	//		the installer is mostly package installation description, but
	//		most people use it for describing the application in more detail
	//		then in the short description.
	//		For now, we'll use the short description if possible.
	BString description = info->GetShortDescription();
	if (description.Length() <= 0)
		description = info->GetDescription();
	packageInfo.SetDescription(description.String());
	packageInfo.SetSpaceNeeded(type->space_needed);

	fItemExistsPolicy = P_EXISTS_NONE;

	const char* installPath = fParent->CurrentPath()->Path();
	for (uint32 i = 0; i < n; i++) {
		ItemState state(fItemExistsPolicy);
		PackageItem* item = static_cast<PackageItem*>(type->items.ItemAt(i));

		err = item->DoInstall(installPath, &state);
		if (err == B_FILE_EXISTS) {
			// Writing to path failed because path already exists - ask the user
			// what to do and retry the writing process
			int32 choice = fParent->ItemExists(*item, state.destination,
				fItemExistsPolicy);
			if (choice != P_EXISTS_ABORT) {
				state.policy = choice;
				err = item->DoInstall(installPath, &state);
			}
		}

		if (err != B_OK) {
			fprintf(stderr, "Error '%s' while writing path\n", strerror(err));
			return P_MSG_I_ERROR;
		}

		if (progress->Stopped())
			return P_MSG_I_ABORT;

		// Update progress
		progress->StageStep(1, NULL, get_item_progress_string(i, n).String());

		// Mark installed item in packageInfo
		packageInfo.AddItem(state.destination.Path());
	}

	progress->StageStep(1, B_TRANSLATE("Running post-installation scripts"),
		 "");

	// Run all scripts
	// TODO: Change current working directory to installation location!
	for (uint32 i = 0; i < m; i++) {
		PackageScript* script = info->GetScript(i);

		fCurrentScriptLocker.Lock();
		fCurrentScript = script;

		status_t status = script->DoInstall(installPath);
		if (status != B_OK) {
			fprintf(stderr, "Error while running script: %s\n",
				strerror(status));
			fCurrentScriptLocker.Unlock();
			return P_MSG_I_ERROR;
		}
		fCurrentScriptLocker.Unlock();

		wait_for_thread(script->GetThreadId(), &status);

		fCurrentScriptLocker.Lock();
		script->SetThreadId(-1);
		fCurrentScript = NULL;
		fCurrentScriptLocker.Unlock();

		if (progress->Stopped())
			return P_MSG_I_ABORT;

		progress->StageStep(1, NULL, get_item_progress_string(i, m).String());
	}

	progress->StageStep(1, B_TRANSLATE("Finishing installation"), "");

	err = packageInfo.Save();
	if (err != B_OK)
		return P_MSG_I_ERROR;

	progress->StageStep(1, B_TRANSLATE("Done"));

	// Inform our parent that we finished
	return P_MSG_I_FINISHED;
}

