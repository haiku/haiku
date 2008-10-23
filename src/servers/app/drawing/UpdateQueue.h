/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef UPDATE_QUEUE_H
#define UPDATE_QUEUE_H

#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <Region.h>

#include "HWInterface.h"


class UpdateQueue : public BLocker, public HWInterfaceListener {
 public:
 								UpdateQueue(HWInterface* interface);
 	virtual						~UpdateQueue();

	virtual	void				FrameBufferChanged();

			status_t			Init();
			void				Shutdown();

			void				AddRect(const BRect& rect);

 private:
	static	int32				_ExecuteUpdatesEntry(void *cookie);
			int32				_ExecuteUpdates();

	volatile bool				fQuitting;
			HWInterface*		fInterface;

			BRegion				fUpdateRegion;
			thread_id			fUpdateExecutor;
			sem_id				fRetraceSem;
			bigtime_t			fRefreshDuration;
};

#endif	// UPDATE_QUEUE_H
