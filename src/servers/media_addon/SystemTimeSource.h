
#include <TimeSource.h>

class SystemTimeSource : public BTimeSource
{
public:
	SystemTimeSource();
	~SystemTimeSource();
	
	void NodeRegistered();
	
	BMediaAddOn* AddOn(int32 * internal_id) const;
	status_t TimeSourceOp(const time_source_op_info & op, void * _reserved);
	
	
	static int32 _ControlThreadStart(void *arg);
	void ControlThread();
	
	thread_id fControlThread;
};
