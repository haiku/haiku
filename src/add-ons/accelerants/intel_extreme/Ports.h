/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef INTEL_PORTS_H
#define INTEL_PORTS_H


#include <edid.h>

#include "intel_extreme.h"

#include "Pipes.h"
#include "pll.h"


#define MAX_PORTS	20	// a generous upper bound

struct pll_limits;

enum port_type {
	INTEL_PORT_TYPE_ANY,		// wildcard for lookup functions
	INTEL_PORT_TYPE_ANALOG,
	INTEL_PORT_TYPE_DVI,
	INTEL_PORT_TYPE_LVDS,
	INTEL_PORT_TYPE_DP,
	INTEL_PORT_TYPE_eDP,
	INTEL_PORT_TYPE_HDMI
};

enum port_index {
	INTEL_PORT_ANY,				// wildcard for lookup functions
	INTEL_PORT_A,
	INTEL_PORT_B,
	INTEL_PORT_C,
	INTEL_PORT_D,
	INTEL_PORT_E
};


class Port {
public:
									Port(port_index index,
										const char* baseName);
virtual								~Port();

virtual	uint32						Type() const = 0;
		const char*					PortName() const
										{ return fPortName; }

		port_index					PortIndex() const
										{ return fPortIndex; }

virtual	bool						IsConnected() = 0;

		status_t					SetPipe(Pipe* pipe);
		::Pipe*						GetPipe()
										{ return fPipe; };

virtual	status_t					Power(bool enabled);

		bool						HasEDID();
virtual	status_t					GetEDID(edid1_info* edid,
										bool forceRead = false);

virtual	status_t					GetPLLLimits(pll_limits& limits);

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode) { return B_ERROR; };

virtual pipe_index					PipePreference();
//										{ return INTEL_PIPE_ANY; };

protected:
		void						_SetName(const char* name);

static	status_t					_GetI2CSignals(void* cookie, int* _clock,
										int* _data);
static	status_t					_SetI2CSignals(void* cookie, int clock,
										int data);

		display_mode				fCurrentMode;
		Pipe*						fPipe;

		status_t					fEDIDState;
		edid1_info					fEDIDInfo;

private:
virtual	addr_t						_DDCRegister() = 0;
virtual addr_t						_PortRegister() = 0;

		port_index					fPortIndex;
		char*						fPortName;
};


class AnalogPort : public Port {
public:
									AnalogPort();

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_ANALOG; }

virtual	bool						IsConnected();

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode);

protected:
virtual	addr_t						_DDCRegister();
virtual addr_t						_PortRegister();
};


class LVDSPort : public Port {
public:
									LVDSPort();

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_LVDS; }

virtual	bool						IsConnected();

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode);

virtual pipe_index					PipePreference();

protected:
virtual	addr_t						_DDCRegister();
virtual addr_t						_PortRegister();
};


class DigitalPort : public Port {
public:
									DigitalPort(
										port_index index = INTEL_PORT_B,
										const char* baseName = "DVI");

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_DVI; }

virtual	bool						IsConnected();

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode);

protected:
virtual	addr_t						_DDCRegister();
virtual addr_t						_PortRegister();
};


class HDMIPort : public DigitalPort {
public:
									HDMIPort(port_index index);

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_HDMI; }

virtual	bool						IsConnected();

protected:
virtual addr_t						_PortRegister();
};


class DisplayPort : public Port {
public:
									DisplayPort(port_index index,
										const char* baseName = "DisplayPort");

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_DP; }

virtual	bool						IsConnected();

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode);

virtual pipe_index					PipePreference();

protected:
virtual	addr_t						_DDCRegister();
virtual	addr_t						_PortRegister();

private:
		status_t					_SetPortLinkGen4(display_mode* target);
};


class EmbeddedDisplayPort : public DisplayPort {
public:
									EmbeddedDisplayPort();

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_eDP; }

virtual	bool						IsConnected();
};


class DigitalDisplayInterface : public Port {
public:
									DigitalDisplayInterface(
										port_index index = INTEL_PORT_A,
										const char* baseName = "Digital Display Interface");

virtual	uint32						Type() const
										{ return INTEL_PORT_TYPE_DVI; }

virtual	status_t					Power(bool enabled);

virtual	bool						IsConnected();

virtual status_t					SetDisplayMode(display_mode* mode,
										uint32 colorMode);

protected:
virtual	addr_t						_DDCRegister();
virtual addr_t						_PortRegister();
private:
		uint8						fMaxLanes;
};


#endif // INTEL_PORTS_H
