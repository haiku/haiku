
#include "OggFrameInfo.h"

OggFrameInfo::OggFrameInfo(uint page, uint packetonpage, uint packet)
{
	this->page = page;
	this->packetonpage = packetonpage;
	this->packet = packet;
}


/* virtual */
OggFrameInfo::~OggFrameInfo()
{
}

uint
OggFrameInfo::GetPage() const
{
	return page;
}

uint
OggFrameInfo::GetPacketOnPage() const
{
	return packetonpage;
}

uint
OggFrameInfo::GetPacket() const
{
	return packet;
}
