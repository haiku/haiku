#include "ScreenSaver.h"

BScreenSaver::BScreenSaver(BMessage *archive,
                           image_id)
{
        ticksize = 50000;
        looponcount = 0;
        loopoffcount = 0;
}


BScreenSaver::~BScreenSaver()
{
}


status_t
BScreenSaver::InitCheck()
{
    return B_OK; // This method is meant to be overridden
}


status_t
BScreenSaver::StartSaver(BView *view,
                         bool preview)
{
    return B_OK; // This method is meant to be overridden
}

void
BScreenSaver::StopSaver()
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::Draw(BView *view,
                   int32 frame)
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::DirectConnected(direct_buffer_info *info)
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::DirectDraw(int32 frame)
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::StartConfig(BView *configView)
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::StopConfig()
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::SupplyInfo(BMessage *info) const
{
	return; // This method is meant to be overridden
}

void
BScreenSaver::ModulesChanged(const BMessage *info)
{
	return; // This method is meant to be overridden
}

status_t
BScreenSaver::SaveState(BMessage *into) const
{
    return -1;
}

void
BScreenSaver::SetTickSize(bigtime_t ts)
{
        ticksize = ts;
}

bigtime_t
BScreenSaver::TickSize() const
{
    return ticksize;
}

void
BScreenSaver::SetLoop(int32 on_count,
                      int32 off_count)
{
        looponcount = on_count;
        loopoffcount = off_count;
}

int32
BScreenSaver::LoopOnCount() const
{
    return looponcount;
}

int32
BScreenSaver::LoopOffCount() const
{
    return loopoffcount;
}

void
BScreenSaver::_ReservedScreenSaver1()
{
}

void
BScreenSaver::_ReservedScreenSaver2()
{
}

void
BScreenSaver::_ReservedScreenSaver3()
{
}

void
BScreenSaver::_ReservedScreenSaver4()
{
}

void
BScreenSaver::_ReservedScreenSaver5()
{
}

void
BScreenSaver::_ReservedScreenSaver6()
{
}

void
BScreenSaver::_ReservedScreenSaver7()
{
}

void
BScreenSaver::_ReservedScreenSaver8()
{
}

