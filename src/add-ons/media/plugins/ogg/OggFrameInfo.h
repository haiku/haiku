#ifndef _OGG_FRAME_INFO_H
#define _OGG_FRAME_INFO_H

#include <sys/types.h>

class OggFrameInfo {
public:
				OggFrameInfo(uint page, uint packet);
	virtual		~OggFrameInfo();

	uint		GetPage() const;
	uint		GetPacket() const;
private:
	uint	page;	// the page the packet started on
	uint	packet;	// of all the packets on that page which packet is this
};

#endif _OGG_FRAME_INFO_H
