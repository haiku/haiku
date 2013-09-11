/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESULT_WINDOW_H
#define RESULT_WINDOW_H


#include <map>
#include <set>

#include <ObjectList.h>
#include <Window.h>


namespace BPackageKit {
	class BSolverPackage;
}

using BPackageKit::BSolverPackage;

class BButton;
class BGroupLayout;
class BGroupView;


class ResultWindow : public BWindow {
public:
			typedef std::set<BSolverPackage*> PackageSet;
			typedef BObjectList<BSolverPackage> PackageList;

public:
								ResultWindow();
	virtual						~ResultWindow();

			bool				AddLocationChanges(const char* location,
									const PackageList& packagesToInstall,
									const PackageSet& packagesAlreadyAdded,
									const PackageList& packagesToUninstall,
									const PackageSet& packagesAlreadyRemoved);
			bool				Go();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
			bool				_AddPackages(BGroupLayout* packagesGroup,
									const PackageList& packages,
									const PackageSet& ignorePackages,
									bool install);

private:
			sem_id				fDoneSemaphore;
			bool				fClientWaiting;
			bool				fAccepted;
			BGroupView*			fContainerView;
			BButton*			fCancelButton;
			BButton*			fApplyButton;
};


#endif	// RESULT_WINDOW_H
