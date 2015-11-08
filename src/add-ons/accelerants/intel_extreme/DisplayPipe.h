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


enum pipe_index {
	INTEL_PIPE_A,
	INTEL_PIPE_B
};


void program_pipe_color_modes(uint32 colorMode);

//class FDILink;
//class PanelFitter;

class DisplayPipe {
public:
									DisplayPipe(pipe_index pipeIndex);
									~DisplayPipe();

		pipe_index					Index()
										{ return fPipeIndex; }

		bool						IsEnabled();
		void						Enable(const display_mode& mode);
		void						Disable();

		void						ConfigureTimings(
										const pll_divisors& divisors);

		// access to the various parts of the pipe
	//	::FDILink*					FDILink()
	//									{ return fFDILink; }
	//	::PanelFitter*				PanelFitter()
	//									{ return fPanelFitter; }

private:
		void						_Enable(bool enable);

	//	FDILink*					fFDILink;
	//	PanelFitter*				fPanelFitter;

		addr_t						fBaseRegister;
		pipe_index					fPipeIndex;
};


#endif // INTEL_PIPE_H
