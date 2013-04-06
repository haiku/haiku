/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef ROOT_H
#define ROOT_H


#include <fs_info.h>
#include <String.h>

#include <Referenceable.h>

#include <packagefs.h>


class Volume;


class Root : public BReferenceable {
public:
								Root();
	virtual						~Root();

			status_t			Init(dev_t deviceID, ino_t nodeID);

			dev_t				DeviceID() const	{ return fDeviceID; }
			ino_t				NodeID() const		{ return fNodeID; }
			const BString&		Path() const
									{ return fPath; }

			status_t			RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);

			Volume*				FindVolume(dev_t deviceID) const;

protected:
	virtual	void				LastReferenceReleased();

private:
			Volume**			_GetVolume(PackageFSMountType mountType);

private:
			dev_t				fDeviceID;
			ino_t				fNodeID;
			BString				fPath;
			Volume*				fSystemVolume;
			Volume*				fCommonVolume;
			Volume*				fHomeVolume;
};


#endif	// ROOT_H
