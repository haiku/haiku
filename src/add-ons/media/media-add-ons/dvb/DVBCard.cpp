/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h> // required on BeOS R5
#include <OS.h>

#include "DVBCard.h"


DVBCard::DVBCard(const char *path)
 :	fInitStatus(B_OK)
 ,	fDev(-1)
{
	printf("DVBCard opening %s\n", path);

	fDev = open(path, O_RDWR);
	if (fDev < 0) {
		printf("DVBCard opening %s failed\n", path);
		fInitStatus = B_ERROR;
		return;
	}
	
	dvb_frequency_info_t info;
	if (do_ioctl(fDev, DVB_GET_FREQUENCY_INFO, &info) < 0) {
		printf("DVB_GET_FREQUENCY_INFO failed with error %s\n", strerror(errno));
//		close(fDev);
//		fDev = -1;
//		return;
	}

	fFreqMin  = info.frequency_min;
	fFreqMax  = info.frequency_max;
	fFreqStep = info.frequency_step;
	
	fCaptureActive = false;
}


DVBCard::~DVBCard()
{
	if (fDev > 0)
		close(fDev);
}


status_t
DVBCard::InitCheck()
{
	return fInitStatus;
}
	

status_t
DVBCard::GetCardType(dvb_type_t *type)
{
	dvb_interface_info_t info;
	
	if (do_ioctl(fDev, DVB_GET_INTERFACE_INFO, &info) < 0) {
		printf("DVB_GET_INTERFACE_INFO failed with error %s\n", strerror(errno));
		return errno;
	}

	if (info.version < 1) {
		printf("DVBCard::GetCardInfo wrong API version\n");
		return B_ERROR;
	}

	*type = info.type;
	return B_OK;
}


status_t
DVBCard::GetCardInfo(char *_name, int max_name_len, char *_info, int max_info_len)
{
	dvb_interface_info_t info;
	
	if (do_ioctl(fDev, DVB_GET_INTERFACE_INFO, &info) < 0) {
		printf("DVB_GET_INTERFACE_INFO failed with error %s\n", strerror(errno));
		return errno;
	}
	
//	strlcpy(_name, info.name, max_name_len);
//	strlcpy(_info, info.info, max_info_len);
	strcpy(_name, info.name);
	strcpy(_info, info.info);

	if (info.version < 1) {
		printf("DVBCard::GetCardInfo wrong API version\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
DVBCard::SetTuningParameter(const dvb_tuning_parameters_t& param)
{
	if (fDev < 0)
		return B_NO_INIT;
		
	printf("DVBCard::SetTuningParameter:\n");
/*
	printf("frequency %Ld\n", param.frequency);
	printf("inversion %d\n", param.inversion);
	printf("bandwidth %d\n", param.bandwidth);
	printf("modulation %d\n", param.modulation);
	printf("hierarchy %d\n", param.hierarchy);
	printf("code_rate_hp %d\n", param.code_rate_hp);
	printf("code_rate_lp %d\n", param.code_rate_lp);
	printf("transmission_mode %d\n", param.transmission_mode);
	printf("guard_interval %d\n", param.guard_interval);
	printf("symbolrate %d\n", param.symbolrate);
	printf("polarity %d\n", param.polarity);
	printf("agc_inversion %d\n", param.agc_inversion);
*/
	params = param;// XXX temporary!
	
//	if (do_ioctl(fDev, DVB_SET_TUNING_PARAMETERS, const_cast<void *>(&param)) < 0) {
//	if (do_ioctl(fDev, DVB_SET_TUNING_PARAMETERS, (void *)(&param)) < 0) {
	if (ioctl(fDev, DVB_SET_TUNING_PARAMETERS, (void *)(&param)) < 0) {
		printf("DVB_SET_TUNING_PARAMETERS failed with error %s\n", strerror(errno));
		return errno;		
	}
	
	dvb_status_t status;
	
	bigtime_t start = system_time();
	bool has_lock = false;
	bool has_signal = false;
	bool has_carrier = false;
	do {
		if (do_ioctl(fDev, DVB_GET_STATUS, &status) < 0) {
			printf("DVB_GET_STATUS failed with error %s\n", strerror(errno));
			return errno;
		}
		if (!has_signal && status & DVB_STATUS_SIGNAL) {
			has_signal = true;
			printf("got signal after %.6f\n", (system_time() - start) / 1e6);
		}
		if (!has_carrier && status & DVB_STATUS_CARRIER) {
			has_carrier = true;
			printf("got carrier after %.6f\n", (system_time() - start) / 1e6);
		}
		if (!has_lock && status & DVB_STATUS_LOCK) {
			has_lock = true;
			printf("got lock after %.6f\n", (system_time() - start) / 1e6);
		}
		
		if ((status & (DVB_STATUS_SIGNAL|DVB_STATUS_CARRIER|DVB_STATUS_LOCK)) == (DVB_STATUS_SIGNAL|DVB_STATUS_CARRIER|DVB_STATUS_LOCK))
			break;
		snooze(25000);
	} while ((system_time() - start) < 5000000);


	if ((status & (DVB_STATUS_SIGNAL|DVB_STATUS_CARRIER|DVB_STATUS_LOCK)) != (DVB_STATUS_SIGNAL|DVB_STATUS_CARRIER|DVB_STATUS_LOCK)) {
		printf("DVB tuning failed! need these status flags: DVB_STATUS_SIGNAL, DVB_STATUS_CARRIER, DVB_STATUS_LOCK\n");
		return B_ERROR;
	}
	
	return B_OK;
}


status_t
DVBCard::GetTuningParameter(dvb_tuning_parameters_t *param)
{
	// XXX get from CARD
	*param = params;
	return B_OK;
}


status_t
DVBCard::GetSignalInfo(uint32 *ss, uint32 *ber, uint32 *snr)
{
	if (do_ioctl(fDev, DVB_GET_SS, ss) < 0) {
		printf("DVB_GET_SS failed with error %s\n", strerror(errno));
		return errno;
	}
	if (do_ioctl(fDev, DVB_GET_BER, ber) < 0) {
		printf("DVB_GET_BER failed with error %s\n", strerror(errno));
		return errno;
	}
	if (do_ioctl(fDev, DVB_GET_SNR, snr) < 0) {
		printf("DVB_GET_SNR failed with error %s\n", strerror(errno));
		return errno;
	}

	printf("ss %08lx, ber %08lx, snr %08lx\n", *ss, *ber, *snr);
	printf("ss %lu%%, ber %lu%%, snr %lu%%\n", *ss / (0xffffffff / 100), *ber / (0xffffffff / 100), *snr / (0xffffffff / 100));
	return B_OK;
}


status_t
DVBCard::CaptureStart()
{
	printf("DVBCard::CaptureStart\n");
	
	if (fCaptureActive) {
		printf("CaptureStart already called, doing nothing\n");
		return B_OK;
	}
	
	if (do_ioctl(fDev, DVB_START_CAPTURE, 0) < 0) {
		printf("DVB_START_CAPTURE failed with error %s\n", strerror(errno));
		return errno;
	}
	
	fCaptureActive = true;
	return B_OK;
}


status_t
DVBCard::CaptureStop()
{
	printf("DVBCard::CaptureStop\n");

	if (!fCaptureActive) {
		printf("CaptureStop already called, doing nothing\n");
		return B_OK;
	}

	if (do_ioctl(fDev, DVB_STOP_CAPTURE, 0) < 0) {
		printf("DVB_STOP_CAPTURE failed with error %s\n", strerror(errno));
		return errno;
	}

	fCaptureActive = false;
	return B_OK;
}


status_t
DVBCard::Capture(void **data, size_t *size, bigtime_t *end_time)
{
	dvb_capture_t cap;

	if (ioctl(fDev, DVB_CAPTURE, &cap) < 0) 
		return errno;

	*data	  = cap.data;
	*size	  = cap.size;
	*end_time = cap.end_time;

	return B_OK;
}


int
DVBCard::do_ioctl(int fDev, ulong op, void *arg)
{
	int res = 0; // silence stupid compiler warning
	int i;
	for (i = 0; i < 20; i++) {
		res = ioctl(fDev, op, arg);
		if (res >= 0)
			break;
	}
	if (i != 0)	printf("ioctl %lx repeated %d times\n", op, i);
	return res;
}
