
#include "OggFrameInfo.h"

OggFrameInfo::OggFrameInfo(uint page, uint packetonpage, uint packet)
{
	this->page = page;
	this->packetonpage = packetonpage;
	this->packet = packet;
}

void
OggFrameInfo::SetNext(uint page, uint packetonpage, uint packet)
{
	nextpage = page;
	nextpacketonpage = packetonpage;
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

uint
OggFrameInfo::GetNextPage() const
{
	return nextpage;
}

uint
OggFrameInfo::GetNextPacketOnPage() const
{
	return nextpacketonpage;
}

uint
OggFrameInfo::GetNextPacket() const
{
	return packet+1;
}
