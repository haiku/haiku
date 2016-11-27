/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_TARGET_HOST_INTERFACE_INFO_H
#define NETWORK_TARGET_HOST_INTERFACE_INFO_H

#include "TargetHostInterfaceInfo.h"


class NetworkTargetHostInterfaceInfo : public TargetHostInterfaceInfo {
public:
								NetworkTargetHostInterfaceInfo();
	virtual						~NetworkTargetHostInterfaceInfo();

	virtual	status_t			Init();

	virtual	bool				IsLocal() const;
	virtual	bool				IsConfigured(Settings* settings) const;
	virtual	SettingsDescription* GetSettingsDescription() const;

	virtual	status_t			CreateInterface(Settings* settings,
									TargetHostInterface*& _interface) const;

private:
			BString				fName;
			SettingsDescription* fDescription;
};

#endif	// NETWORK_TARGET_HOST_INTERFACE_INFO_H
