/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef INTEL_PIPE_H
#define INTEL_PIPE_H


#include <edid.h>

#include "intel_extreme.h"

#include "pll.h"
#include "FlexibleDisplayInterface.h"


#define MAX_PIPES	2


void program_pipe_color_modes(uint32 colorMode);

//class FDILink;
//class PanelFitter;

class Pipe {
public:
									Pipe(pipe_index pipeIndex);
									~Pipe();

		pipe_index					Index()
										{ return fPipeIndex; }

		bool						IsEnabled();
		void						Enable(bool enable);
		void						Disable();

		void						Configure(display_mode* mode);
		void						ConfigureTimings(display_mode* mode,
										bool hardware = true);
		void						ConfigureClocks(
										const pll_divisors& divisors,
										uint32 pixelClock,
										uint32 extraFlags);

		// access to the various parts of the pipe
		::FDILink*					FDI()
										{ return fFDILink; }
	//	::PanelFitter*				PanelFitter()
	//									{ return fPanelFitter; }

private:
		void						_ConfigureTranscoder(display_mode* mode);

		bool						fHasTranscoder;

		FDILink*					fFDILink;
	//	PanelFitter*				fPanelFitter;

		pipe_index					fPipeIndex;

		addr_t						fPipeOffset;
		addr_t						fPlaneOffset;
};


#endif // INTEL_PIPE_H
