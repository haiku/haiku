#ifndef _OGG_FRAME_INFO_H
#define _OGG_FRAME_INFO_H

#include <sys/types.h>

class OggFrameInfo {
public:
				OggFrameInfo(uint page, uint packetonpage, uint packet);
	virtual		~OggFrameInfo();

	uint		GetPage() const;
	uint		GetPacketOnPage() const;
	uint		GetPacket() const;
private:
	uint	page;	// the page the frame started on
	uint	packetonpage; // of all the packets on that page which packet is this
	uint	packet;	// the packet the frame started on
};

#endif _OGG_FRAME_INFO_H
