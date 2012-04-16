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
install_function(void *data)
{
	// TODO: Inform if already one thread is running
	PackageInstall *install = static_cast<PackageInstall *>(data);
	if (data == NULL)
		return -1;

	install->Install();
	return 0;
}


PackageInstall::PackageInstall(PackageView *parent)
	: fParent(parent),
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
		fThreadId = spawn_thread(install_function, "install_package", B_NORMAL_PRIORITY,
				static_cast<void *>(this));
		resume_thread(fThreadId);
	}
	fIdLocker.Unlock();

	return ret;
}


void
PackageInstall::Stop()
{
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
	uint32 msg = _Install();
	if (fParent && fParent->Looper())
		fParent->Looper()->PostMessage(new BMessage(msg), fParent);
}


uint32
PackageInstall::_Install()
{
	PackageInfo *info = fParent->GetPackageInfo();
	pkg_profile *type = static_cast<pkg_profile *>(info->GetProfile(
		fParent->GetCurrentType()));
	uint32 n = type->items.CountItems(), m = info->GetScriptCount();

	PackageStatus *progress = fParent->GetStatusWindow();
	progress->Reset(n + m + 5);

	progress->StageStep(1, B_TRANSLATE("Preparing package"));

	InstalledPackageInfo packageInfo(info->GetName(), info->GetVersion());

	status_t err = packageInfo.InitCheck();
	if (err == B_OK) {
		// The package is already installed, inform the user
		BAlert *reinstall = new BAlert("reinstall",
			B_TRANSLATE("The given package seems to be already installed on "
				"your system. Would you like to uninstall the existing one "
				"and continue the installation?"),
			B_TRANSLATE("Continue"),
			B_TRANSLATE("Abort"));

		if (reinstall->Go() == 0) {
			// Uninstall the package
			err = packageInfo.Uninstall();
			if (err != B_OK) {
				fprintf(stderr, "Error on uninstall\n");
				return P_MSG_I_ERROR;
			}

			err = packageInfo.SetTo(info->GetName(), info->GetVersion(), true);
			if (err != B_OK) {
				fprintf(stderr, "Error on SetTo\n");
				return P_MSG_I_ERROR;
			}
		} else {
			// Abort the installation
			return P_MSG_I_ABORT;
		}
	} else if (err == B_ENTRY_NOT_FOUND) {
		err = packageInfo.SetTo(info->GetName(), info->GetVersion(), true);
		if (err != B_OK) {
			fprintf(stderr, "Error on SetTo\n");
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
	PackageItem *iter;
	ItemState state;
	uint32 i;
	int32 choice;
	BString label;

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

	const char *installPath = fParent->GetCurrentPath()->Path();
	for (i = 0; i < n; i++) {
		state.Reset(fItemExistsPolicy); // Reset the current item state
		iter = static_cast<PackageItem *>(type->items.ItemAt(i));

		err = iter->DoInstall(installPath, &state);
		if (err == B_FILE_EXISTS) {
			// Writing to path failed because path already exists - ask the user
			// what to do and retry the writing process
			choice = fParent->ItemExists(*iter, state.destination,
				fItemExistsPolicy);
			if (choice != P_EXISTS_ABORT) {
				state.policy = choice;
				err = iter->DoInstall(installPath, &state);
			}
		}

		if (err != B_OK) {
			fprintf(stderr, "Error while writing path\n");
			return P_MSG_I_ERROR;
		}

		if (progress->Stopped())
			return P_MSG_I_ABORT;
		label = "";
		label << (uint32)(i + 1) << " of " << (uint32)n;
		progress->StageStep(1, NULL, label.String());

		packageInfo.AddItem(state.destination.Path());
	}

	progress->StageStep(1, B_TRANSLATE("Running post-installation scripts"),
		 "");

	PackageScript *scr;
	status_t status;
	// Run all scripts
	for (i = 0; i < m; i++) {
		scr = info->GetScript(i);

		fCurrentScriptLocker.Lock();
		fCurrentScript = scr;

		if (scr->DoInstall() != B_OK) {
			fprintf(stderr, "Error while running script\n");
			return P_MSG_I_ERROR;
		}
		fCurrentScriptLocker.Unlock();

		wait_for_thread(scr->GetThreadId(), &status);
		fCurrentScriptLocker.Lock();
		scr->SetThreadId(-1);
		fCurrentScript = NULL;
		fCurrentScriptLocker.Unlock();

		if (progress->Stopped())
			return P_MSG_I_ABORT;
		label = "";
		label << (uint32)(i + 1) << " of " << (uint32)m;
		progress->StageStep(1, NULL, label.String());
	}

	progress->StageStep(1, B_TRANSLATE("Finishing installation"), "");

	err = packageInfo.Save();
	if (err != B_OK)
		return P_MSG_I_ERROR;

	progress->StageStep(1, B_TRANSLATE("Done"));

	// Inform our parent that we finished
	return P_MSG_I_FINISHED;
}

