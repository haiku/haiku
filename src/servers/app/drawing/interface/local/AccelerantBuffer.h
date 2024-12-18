/*
 * Copyright 2005 Michael Lotz <mmlr@mlotz.ch>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ACCELERANT_BUFFER_H
#define ACCELERANT_BUFFER_H

#include <Accelerant.h>
#include "RenderingBuffer.h"

class AccelerantBuffer : public RenderingBuffer {
public:
								AccelerantBuffer();
								AccelerantBuffer(const display_mode& mode,
									const frame_buffer_config& config);
								AccelerantBuffer(const AccelerantBuffer& other,
									bool offscreenBuffer = false);
	virtual						~AccelerantBuffer();

	virtual	status_t			InitCheck() const;

	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

			void				SetDisplayMode(const display_mode& mode);
			void				SetFrameBufferConfig(
									const frame_buffer_config& config);
			void				SetOffscreenBuffer(bool offscreenBuffer);

private:
			display_mode		fDisplayMode;
			frame_buffer_config	fFrameBufferConfig;

			bool				fDisplayModeSet : 1;
			bool				fFrameBufferConfigSet : 1;
			bool				fIsOffscreenBuffer : 1;
};

#endif // ACCELERANT_BUFFER_H
