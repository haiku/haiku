/*
 * Copyright 2008-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef DIRECT_WINDOW_INFO_H
#define DIRECT_WINDOW_INFO_H


#include <Autolock.h>
#include <DirectWindow.h>

#include <DirectWindowPrivate.h>

class RenderingBuffer;


class DirectWindowInfo {
public:
								DirectWindowInfo();
								~DirectWindowInfo();

			status_t			InitCheck() const;

			status_t			GetSyncData(
									direct_window_sync_data& data) const;

			status_t			SetState(direct_buffer_state bufferState,
									direct_driver_state driverState,
									RenderingBuffer* renderingBuffer,
									const BRect& windowFrame,
									const BRegion& clipRegion);

			void				EnableFullScreen(const BRect& frame,
									window_feel feel);
			void				DisableFullScreen();

			bool				IsFullScreen() const { return fFullScreen; }
			const BRect&		OriginalFrame() const { return fOriginalFrame; }
			window_feel			OriginalFeel() const { return fOriginalFeel; }

private:
			status_t			_SyncronizeWithClient();

			direct_buffer_info*	fBufferInfo;
			sem_id				fSem;
			sem_id				fAcknowledgeSem;
			area_id				fBufferArea;

			BRect				fOriginalFrame;
			window_feel			fOriginalFeel;
			bool				fFullScreen;
};


#endif	// DIRECT_WINDOW_INFO_H
