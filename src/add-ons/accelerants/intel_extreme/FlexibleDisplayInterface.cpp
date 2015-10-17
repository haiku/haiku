/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "FlexibleDisplayInterface.h"

#include "accelerant.h"
#include "intel_extreme.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_FDI
#ifdef TRACE_FDI
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - FDITransmitter


FDITransmitter::FDITransmitter(int32 pipeIndex)
	:
	fBaseRegister(PCH_FDI_TX_BASE_REGISTER + pipeIndex * PCH_FDI_TX_PIPE_OFFSET)
{
}


bool
FDITransmitter::IsPLLEnabled()
{
	return (read32(fBaseRegister + PCH_FDI_TX_CONTROL) & PCH_FDI_TX_PLL_ENABLED)
		!= 0;
}


void
FDITransmitter::EnablePLL()
{
	uint32 targetRegister = fBaseRegister + PCH_FDI_TX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & PCH_FDI_TX_PLL_ENABLED) != 0) {
		// already enabled, possibly IronLake where it always is
		return;
	}

	write32(targetRegister, value | PCH_FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100); // warmup 10us + dmi delay 20us, be generous
}


void
FDITransmitter::DisablePLL()
{
	if (gInfo->shared_info->device_type.IsGroup(INTEL_TYPE_ILK)) {
		// on IronLake the FDI PLL is alaways enabled, so no point in trying...
		return;
	}

	uint32 targetRegister = fBaseRegister + PCH_FDI_TX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~PCH_FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


// #pragma mark - FDIReceiver


FDIReceiver::FDIReceiver(int32 pipeIndex)
	:
	fBaseRegister(PCH_FDI_RX_BASE_REGISTER + pipeIndex * PCH_FDI_RX_PIPE_OFFSET)
{
}


bool
FDIReceiver::IsPLLEnabled()
{
	return (read32(fBaseRegister + PCH_FDI_RX_CONTROL) & PCH_FDI_RX_PLL_ENABLED)
		!= 0;
}


void
FDIReceiver::EnablePLL()
{
	uint32 targetRegister = fBaseRegister + PCH_FDI_RX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & PCH_FDI_RX_PLL_ENABLED) != 0)
		return;

	write32(targetRegister, value | PCH_FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(200); // warmup 10us + dmi delay 20us, be generous
}


void
FDIReceiver::DisablePLL()
{
	uint32 targetRegister = fBaseRegister + PCH_FDI_RX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~PCH_FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


void
FDIReceiver::SwtichClock(bool toPCDClock)
{
	uint32 targetRegister = fBaseRegister + PCH_FDI_RX_CONTROL;
	write32(targetRegister, (read32(targetRegister) & ~PCH_FDI_RX_CLOCK_MASK)
		| (toPCDClock ? PCH_FDI_RX_CLOCK_PCD : PCH_FDI_RX_CLOCK_RAW));
	read32(targetRegister);
	spin(200);
}


// #pragma mark - FDILink


FDILink::FDILink(int32 pipeIndex)
	:
	fTransmitter(pipeIndex),
	fReceiver(pipeIndex)
{
}
