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

#ifndef __PACKET_H
#define __PACKET_H

#include <SupportDefs.h>

class Packet
{
public:
					Packet(size_t init_size = 8192);
					Packet(const Packet &clone);
					Packet(const void *data, size_t size, bigtime_t time_stamp = 0);
					~Packet();
					
	void			AddData(const void *data, size_t size);
	
	void			MakeEmpty();

	const uint8 *	Data() const;
	size_t			Size() const;
	
	void			SetTimeStamp(bigtime_t time_stamp);
	bigtime_t		TimeStamp() const;


private:
					// not implemented
					Packet & operator= (const Packet &clone);

private:
	void *			fBuffer;
	size_t			fBufferSize;
	size_t			fBufferSizeMax;
	bigtime_t		fTimeStamp;
};


inline const uint8 *
Packet::Data() const
{
	return (uint8 *)fBuffer;
}


inline size_t
Packet::Size() const
{
	return fBufferSize;
}


inline void
Packet::SetTimeStamp(bigtime_t time_stamp)
{
	fTimeStamp = time_stamp;
}


inline bigtime_t
Packet::TimeStamp() const
{
	return fTimeStamp;
}


inline void
Packet::MakeEmpty()
{
	fBufferSize = 0;
	fTimeStamp = 0;
}


#endif
