/*
 * Copyright 2008-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef DIRECT_WINDOW_SUPPORT_H
#define DIRECT_WINDOW_SUPPORT_H


#include <Autolock.h>
#include <DirectWindow.h>

#include <DirectWindowPrivate.h>

class RenderingBuffer;


class DirectWindowData {
public:
								DirectWindowData();
								~DirectWindowData();

			status_t			InitCheck() const;

			status_t			GetSyncData(
									direct_window_sync_data& data) const;
			status_t			SyncronizeWithClient();

			status_t			SetState(direct_buffer_state bufferState,
									direct_driver_state driverState,
									RenderingBuffer* renderingBuffer,
									const BRect& windowFrame,
									const BRegion& clipRegion);

			BRect				old_window_frame;
			bool				full_screen;

private:
			status_t			_SyncronizeWithClient();

			direct_buffer_info*	fBufferInfo;
			sem_id				fSem;
			sem_id				fAcknowledgeSem;
			area_id				fBufferArea;
};


#endif	// DIRECT_WINDOW_SUPPORT_H
