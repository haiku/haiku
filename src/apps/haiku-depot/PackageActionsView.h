/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ACTIONS_VIEW_H
#define PACKAGE_ACTIONS_VIEW_H

#include <GroupView.h>


class BButton;


class PackageActionsView : public BGroupView {
public:
								PackageActionsView();
	virtual						~PackageActionsView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			BButton*			fInstallButton;
			BButton*			fToggleActiveButton;
			BButton*			fUpdateButton;
			BButton*			fUninstallButton;
};

#endif // PACKAGE_ACTIONS_VIEW_H
