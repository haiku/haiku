/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SubWindow.h"

#include <AutoLocker.h>

#include "SubWindowManager.h"


// #pragma mark - SubWindowKey


SubWindowKey::~SubWindowKey()
{
}


// #pragma mark - ObjectSubWindowKey


ObjectSubWindowKey::ObjectSubWindowKey(void* object)
	:
	fObject(object)
{
}


bool
ObjectSubWindowKey::Equals(const SubWindowKey* other) const
{
	if (this == other)
		return true;

	const ObjectSubWindowKey* key
		= dynamic_cast<const ObjectSubWindowKey*>(other);
	return key != NULL && fObject == key->fObject;
}


size_t
ObjectSubWindowKey::HashCode() const
{
	return (size_t)(addr_t)fObject;
}


// #pragma mark - SubWindow


SubWindow::SubWindow(SubWindowManager* manager, BRect frame, const char* title,
	window_type type, uint32 flags, uint32 workspace)
	:
	BWindow(frame, title, type, flags, workspace),
	fSubWindowManager(manager),
	fSubWindowKey(NULL)

{
	fSubWindowManager->AcquireReference();
}


SubWindow::~SubWindow()
{
	RemoveFromSubWindowManager();
	fSubWindowManager->ReleaseReference();
}


bool
SubWindow::AddToSubWindowManager(SubWindowKey* key)
{
	AutoLocker<SubWindowManager> locker(fSubWindowManager);

	fSubWindowKey = key;

	if (!fSubWindowManager->AddSubWindow(this)) {
		fSubWindowKey = NULL;
		return false;
	}

	return true;
}


bool
SubWindow::RemoveFromSubWindowManager()
{
	if (fSubWindowKey == NULL)
		return false;

	fSubWindowManager->RemoveSubWindow(this);
	delete fSubWindowKey;
	fSubWindowKey = NULL;
	return true;
}
