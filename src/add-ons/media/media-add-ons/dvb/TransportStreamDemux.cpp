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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "TransportStreamDemux.h"
#include "Packet.h"
#include "PacketQueue.h"

#define TRACE_TS_DEMUX
#ifdef TRACE_TS_DEMUX
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define CLOCK_TO_USEC(clck)		(bigtime_t((clck) + 13) / 27)
#define USEC_TO_CLOCK(usec)		(bigtime_t((usec) * 27))


TransportStreamDemux::TransportStreamDemux(
	PacketQueue *vid_queue, PacketQueue *aud_queue, 
	PacketQueue *vid_queue2, PacketQueue *aud_queue2,
	PacketQueue *mpeg_ts_queue)
 :	fCount(0)
 ,	fSystemTime(0)
 ,	fPerformanceTime(0)
 ,	fLastEndTime(0)
 ,	fVidPid(-1)
 ,	fAudPid(-1)
 ,	fPcrPid(-1)
 ,	fVidQueue(vid_queue)
 ,	fVidQueue2(vid_queue2)
 ,	fAudQueue(aud_queue)
 ,	fAudQueue2(aud_queue2)
 ,	fMpegTsQueue(mpeg_ts_queue)
 ,	fVidPacket(new Packet)
 ,	fAudPacket(new Packet)
 ,	fVidPacketValid(false)
 ,	fAudPacketValid(false)
{
}


TransportStreamDemux::~TransportStreamDemux()
{
	delete fVidPacket;
	delete fAudPacket;
}


void
TransportStreamDemux::SetPIDs(int vid_pid, int aud_pid, int pcr_pid)
{
	fVidPacket->MakeEmpty();
	fAudPacket->MakeEmpty();
	fVidPacketValid = false;
	fAudPacketValid = false;
	fVidPid = vid_pid; 
	fAudPid = aud_pid;
	fPcrPid = pcr_pid;
}


void
TransportStreamDemux::ProcessPacket(const mpeg_ts_packet *pkt, bigtime_t start_time)
{
	fCount++;
	
//	printf("ProcessPacket %Ld\n", fCount);
	
	if (pkt->SyncByte() != 0x47) {
//		fCountInvalid++;
		printf("packet %Ld sync byte error "
			   "%02x %02x %02x %02x %02x %02x %02x %02x "
			   "%02x %02x %02x %02x %02x %02x %02x %02x\n", fCount,
		pkt->header[0], pkt->header[1], pkt->header[2], pkt->header[3],
		pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3],
		pkt->data[4], pkt->data[5], pkt->data[6], pkt->data[7],
		pkt->data[8], pkt->data[9], pkt->data[10], pkt->data[11]);
		return;
	}

//	if (pkt->TransportError()) {
//		fCountInvalid++;
//		printf("invalid packet %Ld (transport_error_indicator)\n", fCount);
//		return;
//	}

    int pid = pkt->PID();
    
    if (pid == 0) {
    	ProcessPAT(pkt);
    	return;
    }

    if (pid == fPcrPid) {
    	ProcessPCR(pkt, start_time);
    }

    if (pid == fAudPid) {
    	ProcessAUD(pkt);
    }
    
    if (pid == fVidPid) {
    	ProcessVID(pkt);
    }
}


void
TransportStreamDemux::ProcessPAT(const mpeg_ts_packet *pkt)
{
}


void
TransportStreamDemux::ProcessPCR(const mpeg_ts_packet *pkt, bigtime_t start_time)
{
	if (!(pkt->AdaptationFieldControl() & 0x2)) // adaptation field present?
		return;
	if (pkt->data[0] < 7) // long enough?
		return;
	if (!(pkt->data[1] & 0x10)) // PCR present?
		return;
	int64 pcr;
	pcr = (uint64(pkt->data[2]) << 25) 
		| (uint32(pkt->data[3]) << 17)
		| (uint32(pkt->data[4]) << 9)
		| (uint32(pkt->data[5]) << 1)
		| (pkt->data[6] >> 7);
	pcr *= 300;
	pcr += (pkt->data[6] & 1) | pkt->data[7];
//	printf("    pcr = %Ld\n", pcr);

//	bigtime_t now = system_time();
//	printf("system_time %.3f, PCR-time %.3f, delta %.06f, PCR %Ld\n", now / 1e6, CLOCK_TO_USEC(pcr) / 1e6, (CLOCK_TO_USEC(pcr) - now) / 1e6, pcr);

//	atomic_set64(&fCurrentTime, CLOCK_TO_USEC(pcr));

	fTimeSourceLocker.Lock();
	fSystemTime = start_time;
 	fPerformanceTime = CLOCK_TO_USEC(pcr);
	fTimeSourceLocker.Unlock();
}


void
TransportStreamDemux::ProcessAUD(const mpeg_ts_packet *pkt)
{
	// flush old packet
	if (pkt->PayloadUnitStart()) {
		if (fAudPacketValid) {
			Packet *clone = new Packet(*fAudPacket);
			status_t err = fAudQueue->Insert(fAudPacket);
			if (err != B_OK) {
				delete fAudPacket;
				if (err == B_WOULD_BLOCK) {
					printf("fAudQueue->Insert failed (would block)\n");
				}
			}
			err = fAudQueue2->Insert(clone);
			if (err != B_OK) {
				delete clone;
				if (err == B_WOULD_BLOCK) {
					printf("fAudQueue2->Insert failed (would block)\n");
				}
			}
			fAudPacket = new Packet;
		} else {
			fAudPacket->MakeEmpty();
			fAudPacketValid = true;
		}
	}
	
	int skip;
	switch (pkt->AdaptationFieldControl()) {
		case 0:	// illegal
			skip = 184;
			break;
		case 1: // payload only
			skip = 0;
			break;
		case 2:	// adaptation field only
			skip = 184;
			break;
		case 3: // adaptation field followed by payload
			skip = pkt->data[0] + 1;
			if (skip > 184)
				skip = 184;
			break;
		default:
			skip = 0; // stupid compiler, impossible case
	}
	
	const uint8 *data = pkt->data + skip;
	int size = 184 - skip;
	
//	if (skip != 0)
//		printf("aud skip %d\n", skip);

	if (pkt->PayloadUnitStart()) {

		if (pkt->TransportError()) {
			printf("error in audio packet %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
			fAudPacketValid = false;
			return;
		}

		if (data[0] || data[1] || data[2] != 0x01 || data[3] <= 0xbf || data[3] >= 0xf0) {
			printf("invalid audio packet %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
			fAudPacketValid = false;
			return;
		}
		
		if (data[7] & 0x80) { // PTS
			int64 pts;
			int64 dts;
		
			pts = (uint64((data[9] >> 1) & 0x7) << 30)
				| (data[10] << 22) | ((data[11] >> 1) << 15)
				| (data[12] << 7) | (data[13] >> 1);
			pts *= 300;
		       
//		    printf("vid pts = %Ld\n", pts);

			if (data[7] & 0x40) { // DTS
		
				dts = (uint64((data[14] >> 1) & 0x7) << 30)
					| (data[15] << 22) | ((data[16] >> 1) << 15)
					| (data[17] << 7) | (data[18] >> 1);
				dts *= 300;

//				printf("aud dts = %Ld\n", dts);
			} else {
				dts = pts;
			}
			
			fAudPacket->SetTimeStamp(CLOCK_TO_USEC(dts));
		}


//		printf("aud %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		data[0], data[1], data[2], data[3], data[4],
//		data[5], data[6], data[7], data[8], data[9]);
	}

	fAudPacket->AddData(data, size);
}


void
TransportStreamDemux::ProcessVID(const mpeg_ts_packet *pkt)
{
	// flush old packet
//	if (pkt->PayloadUnitStart() || pkt->TransportError()) {
	if (pkt->PayloadUnitStart()) {
		if (fVidPacketValid) {
			Packet *clone = new Packet(*fVidPacket);
			status_t err = fVidQueue->Insert(fVidPacket);
			if (err != B_OK) {
				delete fVidPacket;
				if (err == B_WOULD_BLOCK) {
					printf("fVidQueue->Insert failed (would block)\n");
				}
			}
			err = fVidQueue2->Insert(clone);
			if (err != B_OK) {
				delete clone;
				if (err == B_WOULD_BLOCK) {
					printf("fVidQueue2->Insert failed (would block)\n");
				}
			}
			fVidPacket = new Packet;
		} else {
			fVidPacket->MakeEmpty();
			fVidPacketValid = true;
		}
	}
	
//	if (pkt->TransportError()) {
//		printf("transport error\n");
//		fVidPacketValid = false;
//		return;
//	}
	
	int skip;
	switch (pkt->AdaptationFieldControl()) {
		case 0:	// illegal
			skip = 184;
			break;
		case 1: // payload only
			skip = 0;
			break;
		case 2:	// adaptation field only
			skip = 184;
			break;
		case 3: // adaptation field followed by payload
			skip = pkt->data[0] + 1;
			if (skip > 184)
				skip = 184;
			break;
		default:
			skip = 0; // stupid compiler, impossible case
	}
	
	const uint8 *data = pkt->data + skip;
	int size = 184 - skip;
	
//	if (skip != 0)
//		printf("vid skip %d\n", skip);

	if (pkt->PayloadUnitStart()) {

		if (pkt->TransportError()) {
			printf("error in video packet %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
			fVidPacketValid = false;
			return;
		}
		
		if (data[0] || data[1] || data[2] != 0x01 || data[3] <= 0xbf || data[3] >= 0xf0) {
			printf("invalid video packet %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
			fVidPacketValid = false;
			return;
		}
		
		if (data[7] & 0x80) { // PTS
			int64 pts;
			int64 dts;
		
			pts = (uint64((data[9] >> 1) & 0x7) << 30)
				| (data[10] << 22) | ((data[11] >> 1) << 15)
				| (data[12] << 7) | (data[13] >> 1);
			pts *= 300;
		       
//		    printf("vid pts = %Ld\n", pts);

			if (data[7] & 0x40) { // DTS
		
				dts = (uint64((data[14] >> 1) & 0x7) << 30)
					| (data[15] << 22) | ((data[16] >> 1) << 15)
					| (data[17] << 7) | (data[18] >> 1);
				dts *= 300;

//				printf("vid dts = %Ld\n", dts);
//				printf("dts = %Ld, pts = %Ld, delta %Ld ### dts = %Ld, pts = %Ld, delta %Ld\n", 
//						dts, pts, pts - dts, CLOCK_TO_USEC(dts), CLOCK_TO_USEC(pts), CLOCK_TO_USEC(pts - dts));
			} else {
				dts = pts;
			}

//			printf("clocks: dts = %14Ld, pts = %14Ld, delta %8Ld ### usec: dts = %14Ld, pts = %14Ld, delta %8Ld\n", 
//					dts, pts, pts - dts, CLOCK_TO_USEC(dts), CLOCK_TO_USEC(pts), CLOCK_TO_USEC(pts - dts));
			
			fVidPacket->SetTimeStamp(CLOCK_TO_USEC(dts));
		}
		
//		printf("vid %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		data[0], data[1], data[2], data[3], data[4],
//		data[5], data[6], data[7], data[8], data[9]);
	}

	fVidPacket->AddData(data, size);
}


inline void
TransportStreamDemux::ProcessData(const void *data, int size, bigtime_t start_time, bigtime_t delta)
{
	const uint8 *d = (const uint8 *)data;
	if (d[0] != 0x47 && size > 376 && d[188] != 0x47 && d[376] != 0x47) {
		printf("TransportStreamDemux::ProcessData: input sync error: "
			   "%02x %02x %02x %02x %02x %02x %02x %02x "
			   "%02x %02x %02x %02x %02x %02x %02x %02x\n",
			   d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], 
			   d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
		return;
	}

	const mpeg_ts_packet *pkt = (const mpeg_ts_packet *)data;
	int count = size / 188;
	for (int i = 0; i < count; i++) {
		ProcessPacket(pkt++, start_time + (i * delta) / count);
	}
}


void
TransportStreamDemux::AddData(Packet *packet)
{
	bigtime_t end_time = packet->TimeStamp();
	
	if (fLastEndTime == 0) {
		#define DEFAULT_DELTA 36270
		// need something at the start, 36270 usec is the packet length
		// in Germany, and should be ok for other countries, too
		fLastEndTime = end_time - DEFAULT_DELTA;
	}

	bigtime_t delta = end_time - fLastEndTime;
	
	// sanity check
	if (delta > (3 * DEFAULT_DELTA)) {
		printf("TransportStreamDemux::ProcessData: delta %.6f is too large\n", delta / 1E6);
		fLastEndTime = end_time - DEFAULT_DELTA;
		delta = DEFAULT_DELTA;
	}

	ProcessData(packet->Data(), packet->Size(), fLastEndTime, delta);
	
	packet->SetTimeStamp(fLastEndTime);

	fLastEndTime = end_time;

	status_t err = fMpegTsQueue->Insert(packet);
	if (err != B_OK) {
		delete packet;
		if (err == B_WOULD_BLOCK) {
			printf("fMpegTsQueue->Insert failed (would block)\n");
		}
	}
}

