/*
 * Copyright 2008-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <Autolock.h>
#include <DirectWindow.h>

#include <DirectWindowPrivate.h>

class RenderingBuffer;

struct BufferState {
	BufferState(const direct_buffer_state& state)
		:
		fState(state)
	{
	}

	inline direct_buffer_state Action() const
	{
		return (direct_buffer_state)(fState & B_DIRECT_MODE_MASK);
	}

	inline direct_buffer_state Reason() const
	{
		return (direct_buffer_state)(fState & ~B_DIRECT_MODE_MASK);
	}

	direct_buffer_state fState;
};


class DirectWindowData {
public:
								DirectWindowData();
								~DirectWindowData();

			status_t			InitCheck() const;

			status_t			GetSyncData(
									direct_window_sync_data& data) const;
			status_t			SyncronizeWithClient();

			status_t			SetState(const direct_buffer_state& bufferState,
									const direct_driver_state& driverState,
									RenderingBuffer *renderingBuffer,
									const BRect& windowFrame,
									const BRegion& clipRegion);
			
			BRect				old_window_frame;
			bool				full_screen;

private:
			status_t			_SyncronizeWithClient();

			direct_buffer_info*		fBufferInfo;
			sem_id				fSem;
			sem_id				fAcknowledgeSem;
			area_id				fBufferArea;
			direct_buffer_state		fPreviousState;
			int32				fTransition;
			bool				fStarted;
};

