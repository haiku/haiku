/*
 * Copyright 2008-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <DirectWindow.h>

#include <DirectWindowPrivate.h>

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

			bool				SetState(const direct_buffer_state& bufferState,
									const direct_driver_state& driverState);

			BRect				old_window_frame;
			direct_buffer_info*	buffer_info;
			bool				full_screen;

private:
			bool 				_HandleStop(const direct_buffer_state& state);
			bool 				_HandleStart(const direct_buffer_state& state);
			bool 				_HandleModify(const direct_buffer_state& state);
		
			sem_id				fSem;
			sem_id				fAcknowledgeSem;
			area_id				fBufferArea;
			int32				fTransition;
};

