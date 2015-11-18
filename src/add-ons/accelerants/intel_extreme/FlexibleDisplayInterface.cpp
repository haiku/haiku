/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "FlexibleDisplayInterface.h"

#include <stdlib.h>
#include <string.h>
#include <KernelExport.h>

#include "accelerant.h"
#include "intel_extreme.h"


#define TRACE_FDI
#ifdef TRACE_FDI
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - FDITransmitter


FDITransmitter::FDITransmitter(pipe_index pipeIndex)
	:
	fRegisterBase(PCH_FDI_TX_BASE_REGISTER)
{
	if (pipeIndex == INTEL_PIPE_B)
		fRegisterBase += PCH_FDI_TX_PIPE_OFFSET * 1;
}


FDITransmitter::~FDITransmitter()
{
}


bool
FDITransmitter::IsPLLEnabled()
{
	return (read32(fRegisterBase + PCH_FDI_TX_CONTROL) & FDI_TX_PLL_ENABLED)
		!= 0;
}


void
FDITransmitter::EnablePLL()
{
	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & FDI_TX_PLL_ENABLED) != 0) {
		// already enabled, possibly IronLake where it always is
		return;
	}

	write32(targetRegister, value | FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100); // warmup 10us + dmi delay 20us, be generous
}


void
FDITransmitter::DisablePLL()
{
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_ILK)) {
		// on IronLake the FDI PLL is alaways enabled, so no point in trying...
		return;
	}

	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


// #pragma mark - FDIReceiver


FDIReceiver::FDIReceiver(pipe_index pipeIndex)
	:
	fRegisterBase(PCH_FDI_RX_BASE_REGISTER)
{
	if (pipeIndex == INTEL_PIPE_B)
		fRegisterBase += PCH_FDI_RX_PIPE_OFFSET * 1;
}


FDIReceiver::~FDIReceiver()
{
}


bool
FDIReceiver::IsPLLEnabled()
{
	return (read32(fRegisterBase + PCH_FDI_RX_CONTROL) & FDI_RX_PLL_ENABLED)
		!= 0;
}


void
FDIReceiver::EnablePLL()
{
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & FDI_RX_PLL_ENABLED) != 0)
		return;

	write32(targetRegister, value | FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(200); // warmup 10us + dmi delay 20us, be generous
}


void
FDIReceiver::DisablePLL()
{
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


void
FDIReceiver::SwitchClock(bool toPCDClock)
{
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	write32(targetRegister, (read32(targetRegister) & ~FDI_RX_CLOCK_MASK)
		| (toPCDClock ? FDI_RX_CLOCK_PCD : FDI_RX_CLOCK_RAW));
	read32(targetRegister);
	spin(200);
}


// #pragma mark - FDILink


FDILink::FDILink(pipe_index pipeIndex)
	:
	fTransmitter(pipeIndex),
	fReceiver(pipeIndex)
{
}


FDILink::~FDILink()
{
}
