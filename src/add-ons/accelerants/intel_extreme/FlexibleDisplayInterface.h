/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
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
		void						EnablePLL(uint32 lanes);
		void						DisablePLL();

		pipe_index					PipeIndex()
										 { return fPipeIndex; };

protected:
		pipe_index					fPipeIndex;
};


class FDIReceiver {
public:
									FDIReceiver(pipe_index pipeIndex);
									~FDIReceiver();

		void						Enable();
		void						Disable();

		bool						IsPLLEnabled();
		void						EnablePLL(uint32 lanes);
		void						DisablePLL();

		void						SwitchClock(bool toPCDClock);

		pipe_index					PipeIndex()
										 { return fPipeIndex; };

protected:
		pipe_index					fPipeIndex;
};


class FDILink {
public:
									FDILink(pipe_index pipeIndex);
									~FDILink();

		FDITransmitter&				Transmitter()
										{ return fTransmitter; };
		FDIReceiver&				Receiver()
										{ return fReceiver; };

		status_t					PreTrain(
										display_timing* target,
										uint32* linkBandwidth,
										uint32* lanes,
										uint32* bitsPerPixel);
		status_t					Train(display_timing* target, uint32 lanes);

private:
		status_t					_NormalTrain(uint32 lanes);
		status_t					_IlkTrain(uint32 lanes);
		status_t					_SnbTrain(uint32 lanes);
		status_t					_ManualTrain(uint32 lanes);
		status_t					_AutoTrain(uint32 lanes);

		FDITransmitter				fTransmitter;
		FDIReceiver					fReceiver;

		pipe_index					fPipeIndex;
};


#endif // INTEL_FDI_H
