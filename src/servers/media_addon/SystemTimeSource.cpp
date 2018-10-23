/*
 * Copyright (c) 2003-2004, Marcus Overhagen <marcus@overhagen.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "MediaDebug.h"
#include "SystemTimeSource.h"


SystemTimeSource::SystemTimeSource()
 :	BMediaNode("System clock"),
 	BTimeSource(),
 	fControlThread(-1)
{
	TRACE("SystemTimeSource::SystemTimeSource\n");
}

SystemTimeSource::~SystemTimeSource()
{
	TRACE("SystemTimeSource::~SystemTimeSource enter\n");
	
	close_port(ControlPort());
	if (fControlThread != -1) {
		status_t err;
		wait_for_thread(fControlThread, &err);
	}
	TRACE("SystemTimeSource::~SystemTimeSource exit\n");
}

BMediaAddOn*
SystemTimeSource::AddOn(int32 * internal_id) const
{
	return NULL;
}
	
status_t
SystemTimeSource::TimeSourceOp(const time_source_op_info & op, void * _reserved)
{
	TRACE("######## SystemTimeSource::TimeSourceOp\n");
	bigtime_t real = RealTime();
	PublishTime(real, real, 1.0);
	return B_OK;
}


/* virtual */ void
SystemTimeSource::NodeRegistered()
{
	ASSERT(fControlThread == -1);
	fControlThread = spawn_thread(_ControlThreadStart, "System clock control", 12, this);
	resume_thread(fControlThread);
}

int32
SystemTimeSource::_ControlThreadStart(void *arg)
{
	reinterpret_cast<SystemTimeSource *>(arg)->ControlThread();
	return 0;
}

void
SystemTimeSource::ControlThread()
{
	TRACE("SystemTimeSource::ControlThread() enter\n");
	status_t err;
	do {
		err = WaitForMessage(B_INFINITE_TIMEOUT);
	} while (err == B_OK || err == B_ERROR);
	TRACE("SystemTimeSource::ControlThread() exit\n");
}
