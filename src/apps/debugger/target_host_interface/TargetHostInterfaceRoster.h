/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_INTERFACE_ROSTER_H
#define TARGET_HOST_INTERFACE_ROSTER_H

#include <OS.h>

#include <Locker.h>
#include <ObjectList.h>


class Settings;
class TargetHostInterface;
class TargetHostInterfaceInfo;


class TargetHostInterfaceRoster {
public:
								TargetHostInterfaceRoster();
	virtual						~TargetHostInterfaceRoster();

	static	TargetHostInterfaceRoster* Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			status_t			Init();
			status_t			RegisterInterfaceInfos();

			int32				CountInterfaceInfos();
			TargetHostInterfaceInfo*
								InterfaceInfoAt(int32 index) const;

			status_t			CreateInterface(TargetHostInterfaceInfo* info,
									Settings* settings,
									TargetHostInterface*& _interface) const;

			int32				CountActiveInterfaces();
			TargetHostInterface* ActiveInterfaceAt(int32 index) const;

private:
			typedef BObjectList<TargetHostInterfaceInfo> InfoList;
			typedef BObjectList<TargetHostInterface> InterfaceList;

private:
			BLocker				fLock;
	static	TargetHostInterfaceRoster* sDefaultInstance;

			InfoList			fInterfaceInfos;
			InterfaceList		fActiveInterfaces;
};


#endif	// TARGET_HOST_INTERFACE_ROSTER_H
