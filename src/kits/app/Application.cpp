//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Application.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BApplication class is the center of the application
//					universe.  The global be_app and be_app_messenger 
//					variables are defined here as well.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Application.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
BApplication*	be_app = NULL;
BMessenger		be_app_messenger;

BResources*	BApplication::_app_resources = NULL;
BLocker		BApplication::_app_resources_lock("_app_resources_lock");


//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature)
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature, status_t* error)
{
}
//------------------------------------------------------------------------------
BApplication::~BApplication()
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(BMessage* data)
{
}
//------------------------------------------------------------------------------
BArchivable* BApplication::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BApplication"))
	{
		return NULL;
	}

	return new BApplication(data);
}
//------------------------------------------------------------------------------
status_t BApplication::Archive(BMessage* data, bool deep) const
{
}
//------------------------------------------------------------------------------
status_t BApplication::InitCheck() const
{
	return fInitError;
}
//------------------------------------------------------------------------------
thread_id BApplication::Run()
{
}
//------------------------------------------------------------------------------
void BApplication::Quit()
{
}
//------------------------------------------------------------------------------
bool BApplication::QuitRequested()
{
}
//------------------------------------------------------------------------------
void BApplication::Pulse()
{
}
//------------------------------------------------------------------------------
void BApplication::ReadyToRun()
{
}
//------------------------------------------------------------------------------
void BApplication::MessageReceived(BMessage* msg)
{
}
//------------------------------------------------------------------------------
void BApplication::ArgvReceived(int32 argc, char** argv)
{
}
//------------------------------------------------------------------------------
void BApplication::AppActivated(bool active)
{
}
//------------------------------------------------------------------------------
void BApplication::RefsReceived(BMessage* a_message)
{
}
//------------------------------------------------------------------------------
void BApplication::AboutRequested()
{
}
//------------------------------------------------------------------------------
BHandler* BApplication::ResolveSpecifier(BMessage* msg, int32 index,
										 BMessage* specifier, int32 form,
										 const char* property)
{
}
//------------------------------------------------------------------------------
void BApplication::ShowCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
void BApplication::HideCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
void BApplication::ObscureCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
bool BApplication::IsCursorHidden() const
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const void* cursor)
{
	// BeBook sez: If you want to call SetCursor() without forcing an immediate
	//				sync of the Application Server, you have to use a BCursor.
	// By deductive reasoning, this function forces a sync. =)
	BCursor Cursor(cursor);
	SetCursor(&Cursor, true);
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const BCursor* cursor, bool sync)
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
int32 BApplication::CountWindows() const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return count_windows(false);
}
//------------------------------------------------------------------------------
BWindow* BApplication::WindowAt(int32 index) const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return window_at(index, false);
}
//------------------------------------------------------------------------------
int32 BApplication::CountLoopers() const
{
	// Tough nut to crack; not documented *anywhere*.  Dug down into BLooper and
	// found its private sLooperCount var
}
//------------------------------------------------------------------------------
BLooper* BApplication::LooperAt(int32 index) const
{
}
//------------------------------------------------------------------------------
bool BApplication::IsLaunching() const
{
	return !fReadyToRunCalled;
}
//------------------------------------------------------------------------------
status_t BApplication::GetAppInfo(app_info* info) const
{
	return be_roster->GetRunningAppInfo(be_app->Team(), info);
}
//------------------------------------------------------------------------------
BResources* BApplication::AppResources()
{
}
//------------------------------------------------------------------------------
void BApplication::DispatchMessage(BMessage* an_event, BHandler* handler)
{
}
//------------------------------------------------------------------------------
void BApplication::SetPulseRate(bigtime_t rate)
{
	fPulseRate = rate;
}
//------------------------------------------------------------------------------
status_t BApplication::GetSupportedSuites(BMessage* data)
{
}
//------------------------------------------------------------------------------
status_t BApplication::Perform(perform_code d, void* arg)
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(uint32 signature)
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(const BApplication& rhs)
{
}
//------------------------------------------------------------------------------
BApplication& BApplication::operator=(const BApplication& rhs)
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication1()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication2()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication3()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication4()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication5()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication6()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication7()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication8()
{
}
//------------------------------------------------------------------------------
bool BApplication::ScriptReceived(BMessage* msg, int32 index, BMessage* specifier, int32 form, const char* property)
{
}
//------------------------------------------------------------------------------
void BApplication::run_task()
{
}
//------------------------------------------------------------------------------
void BApplication::InitData(const char* signature, status_t* error)
{
}
//------------------------------------------------------------------------------
void BApplication::BeginRectTracking(BRect r, bool trackWhole)
{
}
//------------------------------------------------------------------------------
void BApplication::EndRectTracking()
{
}
//------------------------------------------------------------------------------
void BApplication::get_scs()
{
}
//------------------------------------------------------------------------------
void BApplication::setup_server_heaps()
{
}
//------------------------------------------------------------------------------
void* BApplication::rw_offs_to_ptr(uint32 offset)
{
}
//------------------------------------------------------------------------------
void* BApplication::ro_offs_to_ptr(uint32 offset)
{
}
//------------------------------------------------------------------------------
void* BApplication::global_ro_offs_to_ptr(uint32 offset)
{
}
//------------------------------------------------------------------------------
void BApplication::connect_to_app_server()
{
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, BRect drag_rect, BHandler* reply_to)
{
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, int32 bitmap_token, drawing_mode dragMode, BHandler* reply_to)
{
}
//------------------------------------------------------------------------------
void BApplication::write_drag(_BSession_* session, BMessage* a_message)
{
}
//------------------------------------------------------------------------------
bool BApplication::quit_all_windows(bool force)
{
}
//------------------------------------------------------------------------------
bool BApplication::window_quit_loop(bool, bool)
{
}
//------------------------------------------------------------------------------
void BApplication::do_argv(BMessage* msg)
{
}
//------------------------------------------------------------------------------
uint32 BApplication::InitialWorkspace()
{
}
//------------------------------------------------------------------------------
int32 BApplication::count_windows(bool incl_menus) const
{
}
//------------------------------------------------------------------------------
BWindow* BApplication::window_at(uint32 index, bool incl_menus) const
{
}
//------------------------------------------------------------------------------
status_t BApplication::get_window_list(BList* list, bool incl_menus) const
{
}
//------------------------------------------------------------------------------
int32 BApplication::async_quit_entry(void* data)
{
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

