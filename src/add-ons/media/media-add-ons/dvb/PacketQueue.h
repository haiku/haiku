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

#ifndef __PACKET_QUEUE_H
#define __PACKET_QUEUE_H

#include <OS.h>

class Packet;

class PacketQueue
{
public:
					PacketQueue(int max_packets = 5);
					~PacketQueue();
			
	status_t		Insert(Packet *packet);
	status_t		Remove(Packet **packet);

	// peeks into queue and delivers a copy
	status_t		Peek(Packet **packet);
	
	void			Flush(bigtime_t timeout);
	
	void			Terminate();
	void			Restart();

private:
	void			Empty();

					// not implemented
					PacketQueue(const PacketQueue & clone);
					PacketQueue & operator=(PacketQueue & clone);

private:
	
	Packet			**fQueue;
	sem_id			fSem;
	volatile bool	fTerminate;
	
	int				fWriteIndex;
	int				fReadIndex;
	int				fMaxPackets;
	int32			fPacketCount;
};


#endif
