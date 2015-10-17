/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef INTEL_PIPE_H
#define INTEL_PIPE_H


class FDILink;
class PanelFitter;

struct pll_divisors;

class DisplayPipe {
public:
									DisplayPipe(int32 pipeIndex);
virtual								~DisplayPipe();

		bool						IsEnabled();
		void						Enable(const display_mode& mode);
		void						Disable();

		void						ConfigureTimings(
										const pll_divisors& divisors);

		// access to the various parts of the pipe
		::FDILink*					FDILink()
										{ return fFDILink; }
		::PanelFitter*				PanelFitter()
										{ return fPanelFitter; }

private:
		void						_Enable(bool enable);

		FDILink*					fFDILink;
		PanelFitter*				fPanelFitter;

		uint32						fRegisterBase;
};

#endif // INTEL_PIPE_H
