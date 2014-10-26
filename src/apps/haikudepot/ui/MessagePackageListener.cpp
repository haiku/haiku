/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MessagePackageListener.h"

#include <Messenger.h>
#include <View.h>

#include "PackageInfo.h"


MessagePackageListener::MessagePackageListener(BView* view)
	:
	fView(view)
{
}


MessagePackageListener::~MessagePackageListener()
{
}


void
MessagePackageListener::PackageChanged(const PackageInfoEvent& event)
{
	BMessenger messenger(fView);
	if (!messenger.IsValid())
		return;

	BMessage message(MSG_UPDATE_PACKAGE);
	message.AddString("title", event.Package()->Title());
	message.AddUInt32("changes", event.Changes());

	messenger.SendMessage(&message);
}


void
MessagePackageListener::SetPackage(const PackageInfoRef& package)
{
	if (fPackage == package)
		return;

	PackageInfoListenerRef listener(this);

	if (fPackage.Get() != NULL)
		fPackage->RemoveListener(listener);

	fPackage = package;

	if (fPackage.Get() != NULL)
		fPackage->AddListener(listener);
}


const PackageInfoRef&
MessagePackageListener::Package() const
{
	return fPackage;
}
