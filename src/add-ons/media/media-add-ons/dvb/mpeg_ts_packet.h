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

#ifndef _MPEG_TS_PACKET_H
#define _MPEG_TS_PACKET_H

#include <SupportDefs.h>

class mpeg_ts_packet
{
public:
	int			SyncByte() const;
	int			TransportError() const;
	int			PayloadUnitStart() const;
	int			PID() const;
	int			AdaptationFieldControl() const;
	int			ContinuityCounter() const;
	
public:
	uint8		header[4];
	uint8		data[184];
} _PACKED;


inline int mpeg_ts_packet::SyncByte() const
{
	return header[0];
}

inline int mpeg_ts_packet::TransportError() const
{
	return header[1] & 0x80;
}

inline int mpeg_ts_packet::PayloadUnitStart() const
{
	return header[1] & 0x40;
}

inline int mpeg_ts_packet::PID() const
{
	return ((header[1] << 8) | header[2]) & 0x1fff;
}

inline int mpeg_ts_packet::AdaptationFieldControl() const
{
    return (header[3] >> 4) & 0x3; 
}

inline int mpeg_ts_packet::ContinuityCounter() const
{
    return header[3] & 0x0f;
}

#endif
