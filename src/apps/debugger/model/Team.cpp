/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Team.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "Breakpoint.h"
#include "DisassembledCode.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "ImageDebugInfo.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "Statement.h"
#include "TeamDebugInfo.h"
#include "Tracing.h"


// #pragma mark - BreakpointByAddressPredicate


struct Team::BreakpointByAddressPredicate
	: UnaryPredicate<Breakpoint> {
	BreakpointByAddressPredicate(target_addr_t address)
		:
		fAddress(address)
	{
	}

	virtual int operator()(const Breakpoint* breakpoint) const
	{
		return -Breakpoint::CompareAddressBreakpoint(&fAddress, breakpoint);
	}

private:
	target_addr_t	fAddress;
};



// #pragma mark - Team


Team::Team(team_id teamID, TeamMemory* teamMemory, Architecture* architecture,
	TeamDebugInfo* debugInfo, TeamTypeInformation* typeInformation)
	:
	fLock("team lock"),
	fID(teamID),
	fTeamMemory(teamMemory),
	fTypeInformation(typeInformation),
	fArchitecture(architecture),
	fDebugInfo(debugInfo)
{
	fDebugInfo->AcquireReference();
}


Team::~Team()
{
	while (UserBreakpoint* userBreakpoint = fUserBreakpoints.RemoveHead())
		userBreakpoint->ReleaseReference();

	for (int32 i = 0; Breakpoint* breakpoint = fBreakpoints.ItemAt(i); i++)
		breakpoint->ReleaseReference();

	while (Image* image = fImages.RemoveHead())
		image->ReleaseReference();

	while (Thread* thread = fThreads.RemoveHead())
		thread->ReleaseReference();

	fDebugInfo->ReleaseReference();
}


status_t
Team::Init()
{
	return fLock.InitCheck();
}


void
Team::SetName(const BString& name)
{
	fName = name;
}


void
Team::AddThread(Thread* thread)
{
	fThreads.Add(thread);
	_NotifyThreadAdded(thread);
}



status_t
Team::AddThread(const ThreadInfo& threadInfo, Thread** _thread)
{
	Thread* thread = new(std::nothrow) Thread(this, threadInfo.ThreadID());
	if (thread == NULL)
		return B_NO_MEMORY;

	status_t error = thread->Init();
	if (error != B_OK) {
		delete thread;
		return error;
	}

	thread->SetName(threadInfo.Name());
	AddThread(thread);

	if (_thread != NULL)
		*_thread = thread;

	return B_OK;
}


void
Team::RemoveThread(Thread* thread)
{
	fThreads.Remove(thread);
	_NotifyThreadRemoved(thread);
}


bool
Team::RemoveThread(thread_id threadID)
{
	Thread* thread = ThreadByID(threadID);
	if (thread == NULL)
		return false;

	RemoveThread(thread);
	thread->ReleaseReference();
	return true;
}


Thread*
Team::ThreadByID(thread_id threadID) const
{
	for (ThreadList::ConstIterator it = fThreads.GetIterator();
			Thread* thread = it.Next();) {
		if (thread->ID() == threadID)
			return thread;
	}

	return NULL;
}


const ThreadList&
Team::Threads() const
{
	return fThreads;
}


status_t
Team::AddImage(const ImageInfo& imageInfo, LocatableFile* imageFile,
	Image** _image)
{
	Image* image = new(std::nothrow) Image(this, imageInfo, imageFile);
	if (image == NULL)
		return B_NO_MEMORY;

	status_t error = image->Init();
	if (error != B_OK) {
		delete image;
		return error;
	}

	if (image->Type() == B_APP_IMAGE)
		SetName(image->Name());

	fImages.Add(image);
	_NotifyImageAdded(image);

	if (_image != NULL)
		*_image = image;

	return B_OK;
}


void
Team::RemoveImage(Image* image)
{
	fImages.Remove(image);
	_NotifyImageRemoved(image);
}


bool
Team::RemoveImage(image_id imageID)
{
	Image* image = ImageByID(imageID);
	if (image == NULL)
		return false;

	RemoveImage(image);
	image->ReleaseReference();
	return true;
}


Image*
Team::ImageByID(image_id imageID) const
{
	for (ImageList::ConstIterator it = fImages.GetIterator();
			Image* image = it.Next();) {
		if (image->ID() == imageID)
			return image;
	}

	return NULL;
}


Image*
Team::ImageByAddress(target_addr_t address) const
{
	for (ImageList::ConstIterator it = fImages.GetIterator();
			Image* image = it.Next();) {
		if (image->ContainsAddress(address))
			return image;
	}

	return NULL;
}


const ImageList&
Team::Images() const
{
	return fImages;
}


bool
Team::AddBreakpoint(Breakpoint* breakpoint)
{
	if (fBreakpoints.BinaryInsert(breakpoint, &Breakpoint::CompareBreakpoints))
		return true;

	breakpoint->ReleaseReference();
	return false;
}


void
Team::RemoveBreakpoint(Breakpoint* breakpoint)
{
	int32 index = fBreakpoints.BinarySearchIndex(*breakpoint,
		&Breakpoint::CompareBreakpoints);
	if (index < 0)
		return;

	fBreakpoints.RemoveItemAt(index);
	breakpoint->ReleaseReference();
}


int32
Team::CountBreakpoints() const
{
	return fBreakpoints.CountItems();
}


Breakpoint*
Team::BreakpointAt(int32 index) const
{
	return fBreakpoints.ItemAt(index);
}


Breakpoint*
Team::BreakpointAtAddress(target_addr_t address) const
{
	return fBreakpoints.BinarySearchByKey(address,
		&Breakpoint::CompareAddressBreakpoint);
}


void
Team::GetBreakpointsInAddressRange(TargetAddressRange range,
	BObjectList<UserBreakpoint>& breakpoints) const
{
	int32 index = fBreakpoints.FindBinaryInsertionIndex(
		BreakpointByAddressPredicate(range.Start()));
	for (; Breakpoint* breakpoint = fBreakpoints.ItemAt(index); index++) {
		if (breakpoint->Address() > range.End())
			break;

		for (UserBreakpointInstanceList::ConstIterator it
				= breakpoint->UserBreakpoints().GetIterator();
			UserBreakpointInstance* instance = it.Next();) {
			breakpoints.AddItem(instance->GetUserBreakpoint());
		}
	}

	// TODO: Avoid duplicates!
}


void
Team::GetBreakpointsForSourceCode(SourceCode* sourceCode,
	BObjectList<UserBreakpoint>& breakpoints) const
{
	if (DisassembledCode* disassembledCode
			= dynamic_cast<DisassembledCode*>(sourceCode)) {
		GetBreakpointsInAddressRange(disassembledCode->StatementAddressRange(),
			breakpoints);
		return;
	}

	LocatableFile* sourceFile = sourceCode->GetSourceFile();
	if (sourceFile == NULL)
		return;

	// TODO: This can probably be optimized. Maybe by registering the user
	// breakpoints with the team and sorting them by source code.
	for (int32 i = 0; Breakpoint* breakpoint = fBreakpoints.ItemAt(i); i++) {
		UserBreakpointInstance* userBreakpointInstance
			= breakpoint->FirstUserBreakpoint();
		if (userBreakpointInstance == NULL)
			continue;

		UserBreakpoint* userBreakpoint
			= userBreakpointInstance->GetUserBreakpoint();
		if (userBreakpoint->Location().SourceFile() == sourceFile)
			breakpoints.AddItem(userBreakpoint);
	}
}


void
Team::AddUserBreakpoint(UserBreakpoint* userBreakpoint)
{
	fUserBreakpoints.Add(userBreakpoint);
	userBreakpoint->AcquireReference();
}


void
Team::RemoveUserBreakpoint(UserBreakpoint* userBreakpoint)
{
	fUserBreakpoints.Remove(userBreakpoint);
	userBreakpoint->ReleaseReference();
}


status_t
Team::GetStatementAtAddress(target_addr_t address, FunctionInstance*& _function,
	Statement*& _statement)
{
	TRACE_CODE("Team::GetStatementAtAddress(%#" B_PRIx64 ")\n", address);

	// get the image at the address
	Image* image = ImageByAddress(address);
	if (image == NULL) {
		TRACE_CODE("  -> no image\n");
		return B_ENTRY_NOT_FOUND;
	}

	ImageDebugInfo* imageDebugInfo = image->GetImageDebugInfo();
	if (imageDebugInfo == NULL) {
		TRACE_CODE("  -> no image debug info\n");
		return B_ENTRY_NOT_FOUND;
	}

	// get the function
	FunctionInstance* functionInstance
		= imageDebugInfo->FunctionAtAddress(address);
	if (functionInstance == NULL) {
		TRACE_CODE("  -> no function instance\n");
		return B_ENTRY_NOT_FOUND;
	}

	// If the function instance has disassembled code attached, we can get the
	// statement directly.
	if (DisassembledCode* code = functionInstance->GetSourceCode()) {
		Statement* statement = code->StatementAtAddress(address);
		if (statement == NULL)
			return B_ENTRY_NOT_FOUND;

		statement->AcquireReference();
		_statement = statement;
		_function = functionInstance;
		return B_OK;
	}

	// get the statement from the image debug info
	FunctionDebugInfo* functionDebugInfo
		= functionInstance->GetFunctionDebugInfo();
	status_t error = functionDebugInfo->GetSpecificImageDebugInfo()
		->GetStatement(functionDebugInfo, address, _statement);
	if (error != B_OK) {
		TRACE_CODE("  -> no statement from the specific image debug info\n");
		return error;
	}

	_function = functionInstance;
	return B_OK;
}


status_t
Team::GetStatementAtSourceLocation(SourceCode* sourceCode,
	const SourceLocation& location, Statement*& _statement)
{
	TRACE_CODE("Team::GetStatementAtSourceLocation(%p, (%" B_PRId32 ", %"
		B_PRId32 "))\n", sourceCode, location.Line(), location.Column());

	// If we're lucky the source code can provide us with a statement.
	if (DisassembledCode* code = dynamic_cast<DisassembledCode*>(sourceCode)) {
		Statement* statement = code->StatementAtLocation(location);
		if (statement == NULL)
			return B_ENTRY_NOT_FOUND;

		statement->AcquireReference();
		_statement = statement;
		return B_OK;
	}

	// Go the long and stony way over the source file and the team debug info.
	// get the source file for the source code
	LocatableFile* sourceFile = sourceCode->GetSourceFile();
	if (sourceFile == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the function at the source location
	Function* function = fDebugInfo->FunctionAtSourceLocation(sourceFile,
		location);
	if (function == NULL)
		return B_ENTRY_NOT_FOUND;

	// Get some function instance and ask its image debug info to provide us
	// with a statement.
	FunctionInstance* functionInstance = function->FirstInstance();
	if (functionInstance == NULL)
		return B_ENTRY_NOT_FOUND;

	FunctionDebugInfo* functionDebugInfo
		= functionInstance->GetFunctionDebugInfo();
	return functionDebugInfo->GetSpecificImageDebugInfo()
		->GetStatementAtSourceLocation(functionDebugInfo, location, _statement);
}


Function*
Team::FunctionByID(FunctionID* functionID) const
{
	return fDebugInfo->FunctionByID(functionID);
}


void
Team::AddListener(Listener* listener)
{
	AutoLocker<Team> locker(this);
	fListeners.Add(listener);
}


void
Team::RemoveListener(Listener* listener)
{
	AutoLocker<Team> locker(this);
	fListeners.Remove(listener);
}


void
Team::NotifyThreadStateChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadStateChanged(
			ThreadEvent(TEAM_EVENT_THREAD_STATE_CHANGED, thread));
	}
}


void
Team::NotifyThreadCpuStateChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadCpuStateChanged(
			ThreadEvent(TEAM_EVENT_THREAD_CPU_STATE_CHANGED, thread));
	}
}


void
Team::NotifyThreadStackTraceChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadStackTraceChanged(
			ThreadEvent(TEAM_EVENT_THREAD_STACK_TRACE_CHANGED, thread));
	}
}


void
Team::NotifyImageDebugInfoChanged(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageDebugInfoChanged(
			ImageEvent(TEAM_EVENT_IMAGE_DEBUG_INFO_CHANGED, image));
	}
}


void
Team::NotifyUserBreakpointChanged(UserBreakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->UserBreakpointChanged(UserBreakpointEvent(
			TEAM_EVENT_USER_BREAKPOINT_CHANGED, this, breakpoint));
	}
}


void
Team::_NotifyThreadAdded(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadAdded(ThreadEvent(TEAM_EVENT_THREAD_ADDED, thread));
	}
}


void
Team::_NotifyThreadRemoved(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadRemoved(ThreadEvent(TEAM_EVENT_THREAD_REMOVED, thread));
	}
}


void
Team::_NotifyImageAdded(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageAdded(ImageEvent(TEAM_EVENT_IMAGE_ADDED, image));
	}
}


void
Team::_NotifyImageRemoved(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageRemoved(ImageEvent(TEAM_EVENT_IMAGE_REMOVED, image));
	}
}


void
Team::_NotifyBreakpointAdded(Breakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->BreakpointAdded(BreakpointEvent(
			TEAM_EVENT_BREAKPOINT_ADDED, this, breakpoint));
	}
}


void
Team::_NotifyBreakpointRemoved(Breakpoint* breakpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->BreakpointRemoved(BreakpointEvent(
			TEAM_EVENT_BREAKPOINT_REMOVED, this, breakpoint));
	}
}


// #pragma mark - Event


Team::Event::Event(uint32 type, Team* team)
	:
	fEventType(type),
	fTeam(team)
{
}


// #pragma mark - ThreadEvent


Team::ThreadEvent::ThreadEvent(uint32 type, Thread* thread)
	:
	Event(type, thread->GetTeam()),
	fThread(thread)
{
}


// #pragma mark - ImageEvent


Team::ImageEvent::ImageEvent(uint32 type, Image* image)
	:
	Event(type, image->GetTeam()),
	fImage(image)
{
}


// #pragma mark - BreakpointEvent


Team::BreakpointEvent::BreakpointEvent(uint32 type, Team* team,
	Breakpoint* breakpoint)
	:
	Event(type, team),
	fBreakpoint(breakpoint)
{
}


// #pragma mark - UserBreakpointEvent


Team::UserBreakpointEvent::UserBreakpointEvent(uint32 type, Team* team,
	UserBreakpoint* breakpoint)
	:
	Event(type, team),
	fBreakpoint(breakpoint)
{
}


// #pragma mark - Listener


Team::Listener::~Listener()
{
}


void
Team::Listener::ThreadAdded(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadRemoved(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ImageAdded(const Team::ImageEvent& event)
{
}


void
Team::Listener::ImageRemoved(const Team::ImageEvent& event)
{
}


void
Team::Listener::ThreadStateChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadCpuStateChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadStackTraceChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ImageDebugInfoChanged(const Team::ImageEvent& event)
{
}


void
Team::Listener::BreakpointAdded(const Team::BreakpointEvent& event)
{
}


void
Team::Listener::BreakpointRemoved(const Team::BreakpointEvent& event)
{
}


void
Team::Listener::UserBreakpointChanged(const Team::UserBreakpointEvent& event)
{
}
