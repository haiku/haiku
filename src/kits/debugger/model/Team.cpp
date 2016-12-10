/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "Team.h"

#include <new>

#include <AutoLocker.h>

#include "Breakpoint.h"
#include "DisassembledCode.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "ImageDebugInfo.h"
#include "SignalDispositionTypes.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "Statement.h"
#include "TeamDebugInfo.h"
#include "Tracing.h"
#include "Value.h"
#include "Watchpoint.h"


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


// #pragma mark - WatchpointByAddressPredicate


struct Team::WatchpointByAddressPredicate
	: UnaryPredicate<Watchpoint> {
	WatchpointByAddressPredicate(target_addr_t address)
		:
		fAddress(address)
	{
	}

	virtual int operator()(const Watchpoint* watchpoint) const
	{
		return -Watchpoint::CompareAddressWatchpoint(&fAddress, watchpoint);
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
	fDebugInfo(debugInfo),
	fStopOnImageLoad(false),
	fStopImageNameListEnabled(false),
	fDefaultSignalDisposition(SIGNAL_DISPOSITION_IGNORE)
{
	fDebugInfo->AcquireReference();
}


Team::~Team()
{
	while (UserBreakpoint* userBreakpoint = fUserBreakpoints.RemoveHead())
		userBreakpoint->ReleaseReference();

	for (int32 i = 0; Breakpoint* breakpoint = fBreakpoints.ItemAt(i); i++)
		breakpoint->ReleaseReference();

	for (int32 i = 0; Watchpoint* watchpoint = fWatchpoints.ItemAt(i); i++)
		watchpoint->ReleaseReference();

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
	_NotifyTeamRenamed();
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


void
Team::ClearImages()
{
	while (!fImages.IsEmpty())
		RemoveImage(fImages.First());
}


bool
Team::AddStopImageName(const BString& name)
{
	if (!fStopImageNames.Add(name))
		return false;

	fStopImageNames.Sort();

	NotifyStopImageNameAdded(name);
	return true;
}


void
Team::RemoveStopImageName(const BString& name)
{
	fStopImageNames.Remove(name);
	NotifyStopImageNameRemoved(name);
}


void
Team::SetStopOnImageLoad(bool enabled, bool useImageNameList)
{
	fStopOnImageLoad = enabled;
	fStopImageNameListEnabled = useImageNameList;
	NotifyStopOnImageLoadChanged(enabled, useImageNameList);
}


const BStringList&
Team::StopImageNames() const
{
	return fStopImageNames;
}


void
Team::SetDefaultSignalDisposition(int32 disposition)
{
	if (disposition != fDefaultSignalDisposition) {
		fDefaultSignalDisposition = disposition;
		NotifyDefaultSignalDispositionChanged(disposition);
	}
}


bool
Team::SetCustomSignalDisposition(int32 signal, int32 disposition)
{
	SignalDispositionMappings::iterator it = fCustomSignalDispositions.find(
		signal);
	if (it != fCustomSignalDispositions.end() && it->second == disposition)
		return true;

	try {
		fCustomSignalDispositions[signal] = disposition;
	} catch (...) {
		return false;
	}

	NotifyCustomSignalDispositionChanged(signal, disposition);

	return true;
}


void
Team::RemoveCustomSignalDisposition(int32 signal)
{
	SignalDispositionMappings::iterator it = fCustomSignalDispositions.find(
		signal);
	if (it == fCustomSignalDispositions.end())
		return;

	fCustomSignalDispositions.erase(it);

	NotifyCustomSignalDispositionRemoved(signal);
}


int32
Team::SignalDispositionFor(int32 signal) const
{
	SignalDispositionMappings::const_iterator it
		= fCustomSignalDispositions.find(signal);
	if (it != fCustomSignalDispositions.end())
		return it->second;

	return fDefaultSignalDisposition;
}


const SignalDispositionMappings&
Team::GetSignalDispositionMappings() const
{
	return fCustomSignalDispositions;
}


void
Team::ClearSignalDispositionMappings()
{
	fCustomSignalDispositions.clear();
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


bool
Team::AddWatchpoint(Watchpoint* watchpoint)
{
	if (fWatchpoints.BinaryInsert(watchpoint, &Watchpoint::CompareWatchpoints))
		return true;

	watchpoint->ReleaseReference();
	return false;
}


void
Team::RemoveWatchpoint(Watchpoint* watchpoint)
{
	int32 index = fWatchpoints.BinarySearchIndex(*watchpoint,
		&Watchpoint::CompareWatchpoints);
	if (index < 0)
		return;

	fWatchpoints.RemoveItemAt(index);
	watchpoint->ReleaseReference();
}


int32
Team::CountWatchpoints() const
{
	return fWatchpoints.CountItems();
}


Watchpoint*
Team::WatchpointAt(int32 index) const
{
	return fWatchpoints.ItemAt(index);
}


Watchpoint*
Team::WatchpointAtAddress(target_addr_t address) const
{
	return fWatchpoints.BinarySearchByKey(address,
		&Watchpoint::CompareAddressWatchpoint);
}


void
Team::GetWatchpointsInAddressRange(TargetAddressRange range,
	BObjectList<Watchpoint>& watchpoints) const
{
	int32 index = fWatchpoints.FindBinaryInsertionIndex(
		WatchpointByAddressPredicate(range.Start()));
	for (; Watchpoint* watchpoint = fWatchpoints.ItemAt(index); index++) {
		if (watchpoint->Address() > range.End())
			break;

		watchpoints.AddItem(watchpoint);
	}
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
Team::NotifyStopOnImageLoadChanged(bool enabled, bool useImageNameList)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->StopOnImageLoadSettingsChanged(
			ImageLoadEvent(TEAM_EVENT_IMAGE_LOAD_SETTINGS_CHANGED, this,
				enabled, useImageNameList));
	}
}


void
Team::NotifyStopImageNameAdded(const BString& name)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->StopOnImageLoadNameAdded(
			ImageLoadNameEvent(TEAM_EVENT_IMAGE_LOAD_NAME_ADDED, this, name));
	}
}


void
Team::NotifyStopImageNameRemoved(const BString& name)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->StopOnImageLoadNameRemoved(
			ImageLoadNameEvent(TEAM_EVENT_IMAGE_LOAD_NAME_REMOVED, this,
				name));
	}
}


void
Team::NotifyDefaultSignalDispositionChanged(int32 disposition)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->DefaultSignalDispositionChanged(
			DefaultSignalDispositionEvent(
				TEAM_EVENT_DEFAULT_SIGNAL_DISPOSITION_CHANGED, this,
				disposition));
	}
}


void
Team::NotifyCustomSignalDispositionChanged(int32 signal, int32 disposition)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->CustomSignalDispositionChanged(
			CustomSignalDispositionEvent(
				TEAM_EVENT_CUSTOM_SIGNAL_DISPOSITION_CHANGED, this,
				signal, disposition));
	}
}


void
Team::NotifyCustomSignalDispositionRemoved(int32 signal)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->CustomSignalDispositionRemoved(
			CustomSignalDispositionEvent(
				TEAM_EVENT_CUSTOM_SIGNAL_DISPOSITION_REMOVED, this,
				signal, SIGNAL_DISPOSITION_IGNORE));
	}
}


void
Team::NotifyConsoleOutputReceived(int32 fd, const BString& output)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ConsoleOutputReceived(
			ConsoleOutputEvent(TEAM_EVENT_CONSOLE_OUTPUT_RECEIVED, this,
				fd, output));
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
Team::NotifyWatchpointChanged(Watchpoint* watchpoint)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->WatchpointChanged(WatchpointEvent(
			TEAM_EVENT_WATCHPOINT_CHANGED, this, watchpoint));
	}
}


void
Team::NotifyDebugReportChanged(const char* reportPath, status_t result)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->DebugReportChanged(DebugReportEvent(
			TEAM_EVENT_DEBUG_REPORT_CHANGED, this, reportPath, result));
	}
}


void
Team::NotifyCoreFileChanged(const char* targetPath)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->CoreFileChanged(CoreFileChangedEvent(
			TEAM_EVENT_CORE_FILE_CHANGED, this, targetPath));
	}
}


void
Team::NotifyMemoryChanged(target_addr_t address, target_size_t size)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->MemoryChanged(MemoryChangedEvent(
			TEAM_EVENT_MEMORY_CHANGED, this, address, size));
	}
}


void
Team::_NotifyTeamRenamed()
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamRenamed(Event(TEAM_EVENT_TEAM_RENAMED, this));
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


// #pragma mark - ImageLoadEvent


Team::ImageLoadEvent::ImageLoadEvent(uint32 type, Team* team,
	bool stopOnImageLoad, bool stopImageNameListEnabled)
	:
	Event(type, team),
	fStopOnImageLoad(stopOnImageLoad),
	fStopImageNameListEnabled(stopImageNameListEnabled)
{
}


// #pragma mark - ImageLoadNameEvent


Team::ImageLoadNameEvent::ImageLoadNameEvent(uint32 type, Team* team,
	const BString& name)
	:
	Event(type, team),
	fImageName(name)
{
}


// #pragma mark - DefaultSignalDispositionEvent


Team::DefaultSignalDispositionEvent::DefaultSignalDispositionEvent(uint32 type,
	Team* team, int32 disposition)
	:
	Event(type, team),
	fDefaultDisposition(disposition)
{
}


// #pragma mark - CustomSignalDispositionEvent


Team::CustomSignalDispositionEvent::CustomSignalDispositionEvent(uint32 type,
	Team* team, int32 signal, int32 disposition)
	:
	Event(type, team),
	fSignal(signal),
	fDisposition(disposition)
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


// #pragma mark - ConsoleOutputEvent


Team::ConsoleOutputEvent::ConsoleOutputEvent(uint32 type, Team* team,
	int32 fd, const BString& output)
	:
	Event(type, team),
	fDescriptor(fd),
	fOutput(output)
{
}


// #pragma mark - DebugReportEvent


Team::DebugReportEvent::DebugReportEvent(uint32 type, Team* team,
	const char* reportPath, status_t finalStatus)
	:
	Event(type, team),
	fReportPath(reportPath),
	fFinalStatus(finalStatus)
{
}


// #pragma mark - CoreFileChangedEvent


Team::CoreFileChangedEvent::CoreFileChangedEvent(uint32 type, Team* team,
	const char* targetPath)
	:
	Event(type, team),
	fTargetPath(targetPath)
{
}


// #pragma mark - MemoryChangedEvent


Team::MemoryChangedEvent::MemoryChangedEvent(uint32 type, Team* team,
	target_addr_t address, target_size_t size)
	:
	Event(type, team),
	fTargetAddress(address),
	fSize(size)
{
}


// #pragma mark - WatchpointEvent


Team::WatchpointEvent::WatchpointEvent(uint32 type, Team* team,
	Watchpoint* watchpoint)
	:
	Event(type, team),
	fWatchpoint(watchpoint)
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
Team::Listener::TeamRenamed(const Team::Event& event)
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
Team::Listener::StopOnImageLoadSettingsChanged(
	const Team::ImageLoadEvent& event)
{
}


void
Team::Listener::StopOnImageLoadNameAdded(const Team::ImageLoadNameEvent& event)
{
}


void
Team::Listener::StopOnImageLoadNameRemoved(
	const Team::ImageLoadNameEvent& event)
{
}


void
Team::Listener::DefaultSignalDispositionChanged(
	const Team::DefaultSignalDispositionEvent& event)
{
}


void
Team::Listener::CustomSignalDispositionChanged(
	const Team::CustomSignalDispositionEvent& event)
{
}


void
Team::Listener::CustomSignalDispositionRemoved(
	const Team::CustomSignalDispositionEvent& event)
{
}


void
Team::Listener::ConsoleOutputReceived(const Team::ConsoleOutputEvent& event)
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


void
Team::Listener::WatchpointAdded(const Team::WatchpointEvent& event)
{
}


void
Team::Listener::WatchpointRemoved(const Team::WatchpointEvent& event)
{
}


void
Team::Listener::WatchpointChanged(const Team::WatchpointEvent& event)
{
}


void
Team::Listener::DebugReportChanged(const Team::DebugReportEvent& event)
{
}


void
Team::Listener::CoreFileChanged(const Team::CoreFileChangedEvent& event)
{
}


void
Team::Listener::MemoryChanged(const Team::MemoryChangedEvent& event)
{
}
