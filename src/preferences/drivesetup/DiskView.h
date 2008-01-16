/*
 * Copyright 2007-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef DISK_VIEW_H
#define DISK_VIEW_H


#include <DiskDevice.h>
#include <View.h>


class DiskView : public BView {
	typedef BView Inherited;
public:
								DiskView(const BRect& frame,
									uint32 resizeMode);
	virtual						~DiskView();

	// BView interface
	virtual	void				Draw(BRect updateRect);	
	virtual	void				FrameResized(float width, float height);

			void				SetDisk(BDiskDevice* disk,
									partition_id selectedPartition);
private:
			void				_UpdateLayout();

			BDiskDevice*		fDisk;

			class PartitionDrawer;
			PartitionDrawer*	fPartitionDrawer;
};


#endif // DISK_VIEW_H
