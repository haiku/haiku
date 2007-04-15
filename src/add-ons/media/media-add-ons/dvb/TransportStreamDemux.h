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

#ifndef __TRANSPORT_STREAM_DEMUX_H
#define __TRANSPORT_STREAM_DEMUX_H

#include <Locker.h>
#include "mpeg_ts_packet.h"

class PacketQueue;
class Packet;

class TransportStreamDemux
{
public:
					TransportStreamDemux(
						PacketQueue *vid_queue, PacketQueue *aud_queue, 
						PacketQueue *vid_queue2, PacketQueue *aud_queue2,
						PacketQueue *mpeg_ts_queue);
	virtual 		~TransportStreamDemux();
	
	void			SetPIDs(int vid_pid, int aud_pid, int pcr_pid);
	
	void			AddData(Packet *packet);
	
	void			TimesourceInfo(bigtime_t *perf_time, bigtime_t *sys_time);

private:
	void			ProcessData(const void *data, int size, bigtime_t start_time, bigtime_t delta);
	void			ProcessPacket(const mpeg_ts_packet *pkt, bigtime_t start_time);

protected:
	virtual void	ProcessPCR(const mpeg_ts_packet *pkt, bigtime_t start_time);
	virtual void	ProcessPAT(const mpeg_ts_packet *pkt);
	virtual void	ProcessAUD(const mpeg_ts_packet *pkt);
	virtual void	ProcessVID(const mpeg_ts_packet *pkt);

private:
	int64			fCount;
	bigtime_t		fSystemTime;
	bigtime_t		fPerformanceTime;
	bigtime_t		fLastEndTime;
	int 			fVidPid;
	int 			fAudPid;
	int 			fPcrPid;
	PacketQueue *	fVidQueue;
	PacketQueue *	fVidQueue2;
	PacketQueue *	fAudQueue;
	PacketQueue *	fAudQueue2;
	PacketQueue *	fMpegTsQueue;
	Packet *		fVidPacket;
	Packet *		fAudPacket;
	bool			fVidPacketValid;
	bool			fAudPacketValid;
	BLocker			fTimeSourceLocker;
};


inline void 
TransportStreamDemux::TimesourceInfo(bigtime_t *perf_time, bigtime_t *sys_time)
{
	fTimeSourceLocker.Lock();
	*perf_time = fPerformanceTime;
	*sys_time = fSystemTime;
	fTimeSourceLocker.Unlock();
}


#endif
