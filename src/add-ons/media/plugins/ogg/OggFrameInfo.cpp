
#include "OggFrameInfo.h"

OggFrameInfo::OggFrameInfo(uint page, uint packet)
{
	this->page = page;
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
OggFrameInfo::GetPacket() const
{
	return packet;
}
