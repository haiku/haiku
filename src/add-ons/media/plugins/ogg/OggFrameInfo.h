#ifndef _OGG_FRAME_INFO_H
#define _OGG_FRAME_INFO_H

#include <sys/types.h>

class OggFrameInfo {
public:
				OggFrameInfo(uint page, uint packetonpage, uint packet);
	virtual		~OggFrameInfo();
	void		SetNext(uint page, uint packetonpage, uint packet);

	uint		GetPage() const;
	uint		GetPacketOnPage() const;
	uint		GetPacket() const;
	uint		GetNextPage() const;
	uint		GetNextPacketOnPage() const;
	uint		GetNextPacket() const;
private:
	uint	page;	// the page the frame started on
	uint	packetonpage; // of all the packets on that page which packet is this
	uint	packet;	// the packet the frame started on
	uint	nextpage;	// the next page after this page
	uint	nextpacketonpage; // of all the packets on next page which packet is the next
	uint	nextpacket;	// the next packet after this packet
};

#endif _OGG_FRAME_INFO_H
