/*
 * Copyright 2003-2006, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ScreenSaver.h"


BScreenSaver::BScreenSaver(BMessage *archive, image_id thisImage)
	:
	fTickSize(50000),
	fLoopOnCount(0),
	fLoopOffCount(0)
{
}


BScreenSaver::~BScreenSaver()
{
}


status_t
BScreenSaver::InitCheck()
{
    // This method is meant to be overridden
    return B_OK;
}


status_t
BScreenSaver::StartSaver(BView *view, bool preview)
{
    // This method is meant to be overridden
    return B_OK;
}


void
BScreenSaver::StopSaver()
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::Draw(BView *view, int32 frame)
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::DirectConnected(direct_buffer_info *info)
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::DirectDraw(int32 frame)
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::StartConfig(BView *configView)
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::StopConfig()
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::SupplyInfo(BMessage* info) const
{
	// This method is meant to be overridden
	return;
}


void
BScreenSaver::ModulesChanged(const BMessage* info)
{
	// This method is meant to be overridden
	return;
}


status_t
BScreenSaver::SaveState(BMessage *into) const
{
    // This method is meant to be overridden
    return B_ERROR;
}


void
BScreenSaver::SetTickSize(bigtime_t tickSize)
{
	fTickSize = tickSize;
}


bigtime_t
BScreenSaver::TickSize() const
{
    return fTickSize;
}


void
BScreenSaver::SetLoop(int32 onCount, int32 offCount)
{
	fLoopOnCount = onCount;
	fLoopOffCount = offCount;
}


int32
BScreenSaver::LoopOnCount() const
{
    return fLoopOnCount;
}


int32
BScreenSaver::LoopOffCount() const
{
    return fLoopOffCount;
}


void BScreenSaver::_ReservedScreenSaver1() {}
void BScreenSaver::_ReservedScreenSaver2() {}
void BScreenSaver::_ReservedScreenSaver3() {}
void BScreenSaver::_ReservedScreenSaver4() {}
void BScreenSaver::_ReservedScreenSaver5() {}
void BScreenSaver::_ReservedScreenSaver6() {}
void BScreenSaver::_ReservedScreenSaver7() {}
void BScreenSaver::_ReservedScreenSaver8() {}

// for compatibility with older BeOS versions
extern "C" {
void ReservedScreenSaver1__12BScreenSaver() {}
void ReservedScreenSaver2__12BScreenSaver() {}
void ReservedScreenSaver3__12BScreenSaver() {}
void ReservedScreenSaver4__12BScreenSaver() {}
void ReservedScreenSaver5__12BScreenSaver() {}
void ReservedScreenSaver6__12BScreenSaver() {}
void ReservedScreenSaver7__12BScreenSaver() {}
void ReservedScreenSaver8__12BScreenSaver() {}
}
