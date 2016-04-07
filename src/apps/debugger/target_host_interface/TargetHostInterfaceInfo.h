/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_INTERFACE_INFO_H
#define TARGET_HOST_INTERFACE_INFO_H

#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Settings;
class SettingsDescription;
class TargetHostInterface;


class TargetHostInterfaceInfo : public BReferenceable {
public:
								TargetHostInterfaceInfo(const BString& name);
	virtual						~TargetHostInterfaceInfo();

			const BString&		Name() const { return fName; }

	virtual	status_t			Init() = 0;

	virtual	bool				IsLocal() const = 0;
	virtual	bool				IsConfigured(Settings* settings) const = 0;
	virtual	SettingsDescription* GetSettingsDescription() const = 0;

	virtual	status_t			CreateInterface(Settings* settings,
									TargetHostInterface*& _interface) const
									= 0;

private:
			BString				fName;
};

#endif	// TARGET_HOST_INTERFACE_INFO_H
