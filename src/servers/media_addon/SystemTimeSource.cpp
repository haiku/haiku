
#include "debug.h"
#include "SystemTimeSource.h"


SystemTimeSource::SystemTimeSource()
 :	BMediaNode("System Clock"),
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
	fControlThread = spawn_thread(_ControlThreadStart, "System Clock control", 12, this);
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
