
#include "OggFrameInfo.h"

OggFrameInfo::OggFrameInfo(uint page, uint packetonpage, uint packet)
{
	this->page = page;
	this->packetonpage = packetonpage;
	this->packet = packet;
}

void
OggFrameInfo::SetNextIsNewPage(bool newpage)
{
	this->newpage = newpage;
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
	return (newpage ? page + 1 : page);
}

uint
OggFrameInfo::GetNextPacketOnPage() const
{
	return (newpage ? 0 : packetonpage + 1);
}

uint
OggFrameInfo::GetNextPacket() const
{
	return packet + 1;
}

