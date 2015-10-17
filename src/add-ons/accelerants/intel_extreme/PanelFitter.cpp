/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "PanelFitter.h"

#include "accelerant.h"
#include "intel_extreme.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_FITTER
#ifdef TRACE_FITTER
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - PanelFitter


PanelFitter::PanelFitter(int32 pipeIndex)
	:
	fBaseRegister(PCH_PANEL_FITTER_BASE_REGISTER
		+ pipeIndex * PCH_PANEL_FITTER_PIPE_OFFSET)
{
}


bool
PanelFitter::IsEnabled()
{
	return (read32(fBaseRegister + PCH_PANEL_FITTER_CONTROL)
		& PANEL_FITTER_ENABLED) != 0;
}


void
PanelFitter::Enable(const display_mode& mode)
{
	// TODO: program the right window size and position based on the mode
	_Enable(true);
}


void
PanelFitter::Disable()
{
	_Enable(false);
}


void
PanelFitter::_Enable(bool enable)
{
	uint32 targetRegister = fBaseRegister + PCH_PANEL_FITTER_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~PANEL_FITTER_ENABLED
		| (enable ? PANEL_FITTER_ENABLED | 0));
	read32(targetRegister);
}
