/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef ROOT_H
#define ROOT_H


#include <Node.h>
#include <String.h>

#include <Referenceable.h>

#include <packagefs.h>


class Volume;


class Root : public BReferenceable {
public:
								Root();
	virtual						~Root();

			status_t			Init(const node_ref& nodeRef);

			const node_ref&		NodeRef() const		{ return fNodeRef; }
			dev_t				DeviceID() const	{ return fNodeRef.device; }
			ino_t				NodeID() const		{ return fNodeRef.node; }
			const BString&		Path() const		{ return fPath; }

			status_t			RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);

			Volume*				FindVolume(dev_t deviceID) const;

protected:
	virtual	void				LastReferenceReleased();

private:
			Volume**			_GetVolume(PackageFSMountType mountType);

private:
			node_ref			fNodeRef;
			BString				fPath;
			Volume*				fSystemVolume;
			Volume*				fCommonVolume;
			Volume*				fHomeVolume;
};


#endif	// ROOT_H
