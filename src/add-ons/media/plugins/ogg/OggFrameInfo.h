#ifndef _OGG_FRAME_INFO_H
#define _OGG_FRAME_INFO_H

#include <SupportDefs.h>

class OggFrameInfo {
public:
				OggFrameInfo(uint page, uint packetonpage, uint packet);
	virtual		~OggFrameInfo();
	void		SetNextIsNewPage(bool newpage = true);

	uint		GetPage() const;
	uint		GetPacketOnPage() const;
	uint		GetPacket() const;

	uint		GetNextPage() const;
	uint		GetNextPacketOnPage() const;
	uint		GetNextPacket() const;
private:
	uint	page;	// the page the frame started on
	uint16	packetonpage; // of all the packets on that page which packet is this
	uint	packet;	// the packet the frame started on
	bool	newpage;
};

#endif _OGG_FRAME_INFO_H
