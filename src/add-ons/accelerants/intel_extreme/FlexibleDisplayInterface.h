/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef INTEL_FDI_H
#define INTEL_FDI_H


class FDITransmitter {
public:
									FDITransmitter(int32 pipeIndex);
virtual								~FDITransmitter();

		bool						IsPLLEnabled();
		void						EnablePLL();
		void						DisablePLL();

private:
		uint32						fRegisterBase;
};


class FDIReceiver {
public:
									FDIReceiver(int32 pipeIndex);

		bool						IsPLLEnabled();
		void						EnablePLL();
		void						DisablePLL();

		void						SwitchClock(bool toPCDClock);

protected:
		uint32						fRegisterBase;
};


class FDILink {
public:
									FDILink(int32 pipeIndex);

		FDITransmitter&				Transmitter();
		FDIReceiver&				Receiver();

private:
		FDITransmitter				fTransmitter;
		FDIReceiver					fReceiver;
};

#endif // INTEL_FDI_H
