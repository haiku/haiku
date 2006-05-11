/* 
 * Copyright 2003-2005, Haiku Inc.
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it).
 *		Carwyn Jones (turok2@currantbun.com)
 * 
 * Distributed under the terms of the MIT License.
 */


#include <DirectWindow.h>
#include <Screen.h>

#include <clipping.h>
#include <AppServerLink.h>
#include <ClipRegion.h>
#include <DirectWindowPrivate.h>
#include <ServerProtocol.h>


// We don't need this kind of locking, since the directDaemonFunc 
// doesn't access critical shared data.
#define DW_NEEDS_LOCKING 0

enum dw_status_bits {
	DW_STATUS_AREA_CLONED	 = 0x1,
	DW_STATUS_THREAD_STARTED = 0x2,
	DW_STATUS_SEM_CREATED	 = 0x4
};


BDirectWindow::BDirectWindow(BRect frame, const char *title, window_type type,
	uint32 flags, uint32 workspace)
	: BWindow(frame, title, type, flags, workspace)
{
	InitData();
}


BDirectWindow::BDirectWindow(BRect frame, const char *title, window_look look,
	window_feel feel, uint32 flags, uint32 workspace)
	: BWindow(frame, title, look, feel, flags, workspace)
{
	InitData();
}


BDirectWindow::~BDirectWindow()
{
	DisposeData();
}


// start of regular BWindow API
BArchivable *
BDirectWindow::Instantiate(BMessage *data)
{
	return NULL;
}


status_t
BDirectWindow::Archive(BMessage *data, bool deep) const
{
	return inherited::Archive(data, deep);
}


void
BDirectWindow::Quit()
{
	inherited::Quit();
}


void
BDirectWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	inherited::DispatchMessage(message, handler);
}


void
BDirectWindow::MessageReceived(BMessage *message)
{
	inherited::MessageReceived(message);
}


void
BDirectWindow::FrameMoved(BPoint new_position)
{
	inherited::FrameMoved(new_position);
}


void
BDirectWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	inherited::WorkspacesChanged(old_ws, new_ws);
}


void
BDirectWindow::WorkspaceActivated(int32 ws, bool state)
{
	inherited::WorkspaceActivated(ws, state);
}


void
BDirectWindow::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
}


void
BDirectWindow::Minimize(bool minimize)
{
	inherited::Minimize(minimize);
}


void
BDirectWindow::Zoom(BPoint rec_position, float rec_width, float rec_height)
{
	inherited::Zoom(rec_position, rec_width, rec_height);
}


void
BDirectWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	inherited::ScreenChanged(screen_size, depth);
}


void
BDirectWindow::MenusBeginning()
{
	inherited::MenusBeginning();
}


void
BDirectWindow::MenusEnded()
{
	inherited::MenusEnded();
}


void
BDirectWindow::WindowActivated(bool state)
{
	inherited::WindowActivated(state);	
}


void
BDirectWindow::Show()
{
	inherited::Show();
}


void
BDirectWindow::Hide()
{
	inherited::Hide();
}


BHandler *
BDirectWindow::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *specifier, int32 form, const char *property)
{
	return inherited::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BDirectWindow::GetSupportedSuites(BMessage *data)
{
	return inherited::GetSupportedSuites(data);
}


status_t
BDirectWindow::Perform(perform_code d, void *arg)
{
	return inherited::Perform(d, arg);
}


void
BDirectWindow::task_looper()
{
	inherited::task_looper();
}


BMessage *
BDirectWindow::ConvertToMessage(void *raw, int32 code)
{
	return inherited::ConvertToMessage(raw, code);
}


//	#pragma mark - BDirectWindow specific API


void
BDirectWindow::DirectConnected(direct_buffer_info *info)
{
	// implemented in subclasses
}


status_t
BDirectWindow::GetClippingRegion(BRegion *region, BPoint *origin) const
{
	if (region == NULL)
		return B_BAD_VALUE;

	if (IsLocked() || !LockDirect())
		return B_ERROR;
	
	if (in_direct_connect) {
		UnlockDirect();
		return B_ERROR;
	}
		
	// BPoint's coordinates are floats. We can only work
	// with integers.
	int32 originX, originY;
	if (origin == NULL) {
		originX = 0;
		originY = 0;
	} else {
		originX = (int32)origin->x;
		originY = (int32)origin->y;
	}

#ifndef HAIKU_TARGET_PLATFORM_DANO
	// Since we are friend of BRegion, we can access its private members.
	// Otherwise, we would need to call BRegion::Include(clipping_rect)
	// for every clipping_rect in our clip_list, and that would be much
	// more overkill than this (tested ).
	region->fRegion->Set(buffer_desc->clip_list, buffer_desc->clip_list_count);
	
	// adjust bounds by the given origin point 
	region->OffsetBy(-originX, -originY);		
#endif

	UnlockDirect();

	return B_OK;

}


status_t
BDirectWindow::SetFullScreen(bool enable)
{
	status_t status = B_ERROR;
	if (Lock()) {
		fLink->StartMessage(AS_DIRECT_WINDOW_SET_FULLSCREEN);
		fLink->Attach<bool>(enable);

		if (fLink->FlushWithReply(status) == B_OK
			&& status == B_OK)
			full_screen_enable = enable;
		Unlock();
	}
	return status;
}


bool
BDirectWindow::IsFullScreen() const
{
	return full_screen_enable;
}


/*static*/
bool
BDirectWindow::SupportsWindowMode(screen_id id)
{
	display_mode mode;
	status_t status = BScreen(id).GetMode(&mode);
	if (status == B_OK)
		return mode.flags & B_PARALLEL_ACCESS;

	return false;
}


//	#pragma mark - Private methods


int32
BDirectWindow::DirectDaemonFunc(void *arg)
{
	BDirectWindow *object = static_cast<BDirectWindow *>(arg);

	while (!object->daemon_killer) {
		// This sem is released by the app_server when our
		// clipping region changes, or when our window is moved,
		// resized, etc. etc.
		status_t status;
		do {
			status = acquire_sem(object->disable_sem);
		} while (status == B_INTERRUPTED);

		if (status < B_OK)
			return -1;

		if (object->LockDirect()) {
			if ((object->buffer_desc->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_START)
				object->connection_enable = true;

			object->in_direct_connect = true;
			object->DirectConnected(object->buffer_desc);	
			object->in_direct_connect = false;

			if ((object->buffer_desc->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP)
				object->connection_enable = false;

			object->UnlockDirect();	
		}

		// The app_server then waits (with a timeout) on this sem.
		// If we aren't quick enough to release this sem, our app
		// will be terminated by the app_server
		if (release_sem(object->disable_sem_ack) != B_OK)
			return -1;
	}

	return 0;
}


// LockDirect() and UnlockDirect() are no-op on R5. I tried to call (R5's) LockDirect()
// repeatedly, from the same thread and from different threads, nothing happened.
// I implemented them anyway, as they were the first methods I wrote
// in this class (As you can see, I even needed to cast away their constness
// to make them do something useful). 
// They're not needed though, as the direct_daemon_thread doesn't change
// any shared data. They are probably here for future enhancements (see also the 
// comment in DriverSetup()
bool
BDirectWindow::LockDirect() const
{
	status_t status = B_OK;

#if DW_NEEDS_LOCKING
	BDirectWindow *casted = const_cast<BDirectWindow *>(this);
	
	if (atomic_add(&casted->direct_lock, 1) > 0) {
		do {
			status = acquire_sem(direct_sem);
		} while (status == B_INTERRUPTED);
	}
		
	if (status == B_OK) {
		casted->direct_lock_owner = find_thread(NULL);
		casted->direct_lock_count++;
	}
#endif

	return status == B_OK;
}


void
BDirectWindow::UnlockDirect() const
{
#if DW_NEEDS_LOCKING
	BDirectWindow *casted = const_cast<BDirectWindow *>(this);
	
	if (atomic_add(&casted->direct_lock, -1) > 1)
		release_sem(direct_sem);
	
	casted->direct_lock_count--;
#endif
}


void
BDirectWindow::InitData()
{
	connection_enable = false;
	full_screen_enable = false;
	in_direct_connect = false;
	
	dw_init_status = 0;
	
	direct_driver_ready = false;
	direct_driver_type = 0;
	direct_driver_token = 0;	
	direct_driver = NULL;

	if (!Lock())
		return;

	struct direct_window_sync_data syncData;
	
	fLink->StartMessage(AS_DIRECT_WINDOW_GET_SYNC_DATA);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK
		&& status == B_OK) {
		fLink->Read<direct_window_sync_data>(&syncData);
	}

	Unlock();

	if (status < B_OK)
		return;

#if DW_NEEDS_LOCKING	
	direct_lock = 0;
	direct_lock_count = 0;
	direct_lock_owner = -1;
	direct_lock_stack = NULL;
	direct_sem = create_sem(1, "direct sem");
	if (direct_sem > 0)
		dw_init_status |= DW_STATUS_SEM_CREATED;
#endif		

	source_clipping_area = syncData.area;
	disable_sem = syncData.disable_sem;
	disable_sem_ack = syncData.disable_sem_ack;

	cloned_clipping_area = clone_area("Clone direct area", (void**)&buffer_desc,
		B_ANY_ADDRESS, B_READ_AREA, source_clipping_area);		

	if (cloned_clipping_area > 0) {			
		dw_init_status |= DW_STATUS_AREA_CLONED;

		direct_daemon_id = spawn_thread(DirectDaemonFunc, "direct daemon",
				B_DISPLAY_PRIORITY, this);

		if (direct_daemon_id > 0) {
			daemon_killer = false;
			if (resume_thread(direct_daemon_id) == B_OK)
				dw_init_status |= DW_STATUS_THREAD_STARTED;
			else
				kill_thread(direct_daemon_id);
		}
	}
}


void
BDirectWindow::DisposeData()
{
	// wait until the connection terminates: we can't destroy
	// the object until the client receives the B_DIRECT_STOP
	// notification, or bad things will happen
	while (connection_enable)
		snooze(50000);
	
	LockDirect();
	
	if (dw_init_status & DW_STATUS_THREAD_STARTED) {
		daemon_killer = true;
		// Release this sem, otherwise the Direct daemon thread
		// will wait forever on it
		release_sem(disable_sem);
		status_t retVal;
		wait_for_thread(direct_daemon_id, &retVal);
	}
	
#if DW_NEEDS_LOCKING
	if (dw_init_status & DW_STATUS_SEM_CREATED)
		delete_sem(direct_sem);
#endif	

	if (dw_init_status & DW_STATUS_AREA_CLONED)
		delete_area(cloned_clipping_area);
}


status_t
BDirectWindow::DriverSetup() const
{
	// Unimplemented in R5.
	// This function is probably here because they wanted, in a future time,
	// to implement graphic acceleration within BDirectWindow
	// (in fact, there is also a BDirectDriver member in BDirectWindow,
	// though it's not used).

	return B_OK;
}


void BDirectWindow::_ReservedDirectWindow1() {}
void BDirectWindow::_ReservedDirectWindow2() {}
void BDirectWindow::_ReservedDirectWindow3() {}
void BDirectWindow::_ReservedDirectWindow4() {}
