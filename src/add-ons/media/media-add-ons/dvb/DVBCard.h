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

#ifndef __DVB_CARD_H
#define __DVB_CARD_H

#include "dvb.h"

class DVBCard
{
public:
				DVBCard(const char *path);
				~DVBCard();

	status_t	InitCheck();

	status_t	GetCardType(dvb_type_t *type);
	status_t	GetCardInfo(char *name, int max_name_len, char *info, int max_info_len);
	status_t	GetSignalInfo(uint32 *ss, uint32 *ber, uint32 *snr);
				
	status_t	SetTuningParameter(const dvb_tuning_parameters_t& param);
	status_t	GetTuningParameter(dvb_tuning_parameters_t *param);
			
	status_t	CaptureStart();
	status_t	CaptureStop();
	status_t	Capture(void **data, size_t *size, bigtime_t *end_time);

private:

	int			do_ioctl(int fDev, ulong op, void *arg);

	status_t	fInitStatus;
	int			fDev;

	int64		fFreqMin;
	int64		fFreqMax;
	int64		fFreqStep;
	
	bool		fCaptureActive;

	dvb_tuning_parameters_t params; // XXX temporary cache
};


#endif
