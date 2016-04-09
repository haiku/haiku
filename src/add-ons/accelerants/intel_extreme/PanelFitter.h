/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef INTEL_FITTER_H
#define INTEL_FITTER_H


class PanelFitter {
public:
									PanelFitter(int32 pipeIndex);
virtual								~PanelFitter();

		bool						IsEnabled();
		void						Enable(const display_mode& mode);
		void						Disable();

private:
		void						_Enable(bool enable);

		uint32						fRegisterBase;
};

#endif // INTEL_FITTER_H
