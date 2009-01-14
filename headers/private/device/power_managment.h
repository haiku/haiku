#ifndef POWER_MANAGMENT_H
#define POWER_MANAGMENT_H

#include <Drivers.h>

// io controls
enum { 
	// ioctl response with kMagicFreqID
	IDENTIFY_DEVICE = B_DEVICE_OP_CODES_END + 20001,
	
	// CPU Frequence:
	// get a list of freq_info, the list is terminated with a element with
	// frequency = 0
	GET_CPU_FREQ_STATES = B_DEVICE_OP_CODES_END + 20005,
	// get and set a freq_info
	GET_CURENT_CPU_FREQ_STATE,
	SET_CPU_FREQ_STATE,
	// start watching for frequency changes, ioctl blocks until the frequency
	// has changed
	WATCH_CPU_FREQ,
	// stop all watching ioctl, ioctl return B_ERROR
	STOP_WATCHING_CPU_FREQ
}; 

// CPU Frequence:
// magic id returned by IDENTIFY_DEVICE
const uint32 kMagicFreqID = 48921;

#define MAX_CPU_FREQUENCY_STATES 10

typedef struct {
	uint16			frequency;	// [Mhz]
	uint16			volts;
	uint16			id;
	int				power;
} freq_info;


#endif
