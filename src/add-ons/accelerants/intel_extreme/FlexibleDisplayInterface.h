/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef INTEL_FDI_H
#define INTEL_FDI_H


#include "intel_extreme.h"


class FDITransmitter {
public:
									FDITransmitter(pipe_index pipeIndex);
									~FDITransmitter();

		void						Enable();
		void						Disable();

		bool						IsPLLEnabled();
		void						EnablePLL();
		void						DisablePLL();

private:
		uint32						fRegisterBase;
};


class FDIReceiver {
public:
									FDIReceiver(pipe_index pipeIndex);
									~FDIReceiver();

		void						Enable();
		void						Disable();

		bool						IsPLLEnabled();
		void						EnablePLL();
		void						DisablePLL();

		void						SwitchClock(bool toPCDClock);

protected:
		uint32						fRegisterBase;
};


class FDILink {
public:
									FDILink(pipe_index pipeIndex);
									~FDILink();

		FDITransmitter&				Transmitter()
										{ return fTransmitter; };
		FDIReceiver&				Receiver()
										{ return fReceiver; };

private:
		FDITransmitter				fTransmitter;
		FDIReceiver					fReceiver;
};


#endif // INTEL_FDI_H
