/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCAL_TARGET_HOST_INTERFACE_INFO_H
#define LOCAL_TARGET_HOST_INTERFACE_INFO_H

#include "TargetHostInterfaceInfo.h"


class LocalTargetHostInterfaceInfo : public TargetHostInterfaceInfo {
public:
								LocalTargetHostInterfaceInfo();
	virtual						~LocalTargetHostInterfaceInfo();

	virtual	status_t			Init();

	virtual	bool				IsLocal() const;
	virtual	bool				IsConfigured(Settings* settings) const;
	virtual	SettingsDescription* GetSettingsDescription() const;

	virtual	status_t			CreateInterface(Settings* settings,
									TargetHostInterface*& _interface) const;

private:
			BString				fName;
};

#endif	// TARGET_HOST_INTERFACE_INFO_H
