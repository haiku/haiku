
#include <BeOSBuildCompatibility.h>

#include <OS.h>

static const thread_id kMainThreadID = 3;


// kill_thread
status_t
kill_thread(thread_id thread)
{
	return B_BAD_VALUE;
}

// resume_thread
status_t
resume_thread(thread_id thread)
{
	return B_BAD_VALUE;
}

// suspend_thread
status_t
suspend_thread(thread_id thread)
{
	return B_BAD_VALUE;
}

// find_thread
thread_id
find_thread(const char *name)
{
	if (name != NULL)
		return B_ENTRY_NOT_FOUND;
	
	return kMainThreadID;
}

// _get_thread_info
// status_t
// _get_thread_info(thread_id id, thread_info *info, size_t size)
// {
// 	return B_ERROR;
// }

// _get_next_thread_info
// status_t
// _get_next_thread_info(team_id team, int32 *cookie, thread_info *info,
// 	size_t size)
// {
// 	return B_ERROR;
// }
