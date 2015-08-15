/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfLoadingStateHandler.h"

#include <sys/wait.h>

#include <Entry.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>

#include "AutoDeleter.h"
#include "DwarfFile.h"
#include "DwarfImageDebugInfoLoadingState.h"
#include "package/manager/PackageManager.h"
#include "Tracing.h"
#include "UserInterface.h"


using namespace BPackageKit;
using BPackageKit::BManager::BPrivate::BPackageManager;


enum {
	USER_CHOICE_INSTALL_PACKAGE = 0,
	USER_CHOICE_LOCATE_FILE ,
	USER_CHOICE_SKIP
};


DwarfLoadingStateHandler::DwarfLoadingStateHandler()
	:
	ImageDebugLoadingStateHandler()
{
}


DwarfLoadingStateHandler::~DwarfLoadingStateHandler()
{
}


bool
DwarfLoadingStateHandler::SupportsState(
	SpecificImageDebugInfoLoadingState* state)
{
	return dynamic_cast<DwarfImageDebugInfoLoadingState*>(state) != NULL;
}


void
DwarfLoadingStateHandler::HandleState(
	SpecificImageDebugInfoLoadingState* state, UserInterface* interface)
{
	DwarfImageDebugInfoLoadingState* dwarfState
		= dynamic_cast<DwarfImageDebugInfoLoadingState*>(state);

	if (dwarfState == NULL) {
		ERROR("DwarfLoadingStateHandler::HandleState() passed "
			"non-dwarf state object %p.", state);
		return;
	}

	DwarfFileLoadingState& fileState = dwarfState->GetFileState();

	BString requiredPackage;
	_GetMatchingDebugInfoPackage(fileState.externalInfoFileName,
		requiredPackage);

	// loop so that the user has a chance to retry or locate the file manually
	// in case package installation fails, e.g. due to transient download
	// issues.
	for (;;) {
		int32 choice;
		BString message;
		if (interface->IsInteractive()) {
			if (requiredPackage.IsEmpty()) {
				message.SetToFormat("The debug information file '%s' for "
					"image '%s' is missing. Would you like to locate the file "
					"manually?", fileState.externalInfoFileName.String(),
					fileState.dwarfFile->Name());
				choice = interface->SynchronouslyAskUser("Debug info missing",
					message.String(), "Locate", "Skip", NULL);
				if (choice == 0)
					choice = USER_CHOICE_LOCATE_FILE;
				else if (choice == 1)
					choice = USER_CHOICE_SKIP;
			} else {
				message.SetToFormat("The debug information file '%s' for "
					"image '%s' is missing, but can be found in the package "
					"'%s'. Would you like to install it, or locate the file "
					"manually?", fileState.externalInfoFileName.String(),
					fileState.dwarfFile->Name(), requiredPackage.String());
				choice = interface->SynchronouslyAskUser("Debug info missing",
					message.String(), "Install", "Locate", "Skip");
			}
		} else {
			choice = requiredPackage.IsEmpty()
				? USER_CHOICE_SKIP : USER_CHOICE_INSTALL_PACKAGE;
		}

		if (choice == USER_CHOICE_INSTALL_PACKAGE) {
			// TODO: integrate the package installation functionality directly.
			BString command;
			command.SetToFormat("/bin/pkgman install -y %s",
				requiredPackage.String());
			BString notification;
			notification.SetToFormat("Installing package %s" B_UTF8_ELLIPSIS,
				requiredPackage.String());
			interface->NotifyBackgroundWorkStatus(notification);
			int error = system(command.String());
			if (interface->IsInteractive()) {
				if (WIFEXITED(error)) {
					error = WEXITSTATUS(error);
					if (error == B_OK)
						break;
					message.SetToFormat("Package installation failed: %s.",
						strerror(error));
					interface->NotifyUser("Error", message.String(),
						USER_NOTIFICATION_ERROR);
					continue;
				}
			}
			break;
		} else if (choice == USER_CHOICE_LOCATE_FILE) {
			entry_ref ref;
			interface->SynchronouslyAskUserForFile(&ref);
			BPath path(&ref);
			if (path.InitCheck() == B_OK)
				fileState.locatedExternalInfoPath = path.Path();
			break;
		} else
			break;
	}

	fileState.state = DWARF_FILE_LOADING_STATE_USER_INPUT_PROVIDED;
}


status_t
DwarfLoadingStateHandler::_GetMatchingDebugInfoPackage(
	const BString& debugFileName, BString& _packageName)
{
	BString resolvableName;
	BPackageVersion requiredVersion;
	BPackageManager::ClientInstallationInterface clientInterface;
	BPackageManager::UserInteractionHandler handler;

	BPackageManager packageManager(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM,
		&clientInterface, &handler);
	packageManager.Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
		| BPackageManager::B_ADD_REMOTE_REPOSITORIES);
	BObjectList<BSolverPackage> packages;
	status_t error = _GetResolvableName(debugFileName, resolvableName,
		requiredVersion);
	if (error != B_OK)
		return error;

	error = packageManager.Solver()->FindPackages(resolvableName,
		BSolver::B_FIND_IN_PROVIDES, packages);
	if (error != B_OK)
		return error;
	else if (packages.CountItems() == 0)
		return B_ENTRY_NOT_FOUND;

	for (int32 i = 0; i < packages.CountItems(); i++) {
		BSolverPackage* package = packages.ItemAt(i);
		if (requiredVersion.Compare(package->Version()) == 0) {
			_packageName = package->Name();
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DwarfLoadingStateHandler::_GetResolvableName(const BString& debugFileName,
	BString& _resolvableName, BPackageVersion& _resolvableVersion)
{
	BString fileName;
	BString packageName;
	BString packageVersion;

	int32 startIndex = 0;
	int32 endIndex = debugFileName.FindFirst('(');
	if (endIndex < 0)
		return B_BAD_VALUE;

	debugFileName.CopyInto(fileName, 0, endIndex);
	startIndex = endIndex + 1;
	endIndex = debugFileName.FindFirst('-', startIndex);
	if (endIndex < 0)
		return B_BAD_VALUE;

	debugFileName.CopyInto(packageName, startIndex, endIndex - startIndex);
	startIndex = endIndex + 1;
	endIndex = debugFileName.FindFirst(')', startIndex);
	if (endIndex < 0)
		return B_BAD_VALUE;

	debugFileName.CopyInto(packageVersion, startIndex,
		endIndex - startIndex);

	_resolvableName.SetToFormat("debuginfo:%s(%s)", fileName.String(), packageName.String());

	return _resolvableVersion.SetTo(packageVersion);
}
