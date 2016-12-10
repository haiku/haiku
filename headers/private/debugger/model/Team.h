/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H

#include <map>

#include <Locker.h>
#include <StringList.h>

#include <ObjectList.h>

#include "Image.h"
#include "ImageInfo.h"
#include "TargetAddressRange.h"
#include "Thread.h"
#include "ThreadInfo.h"
#include "UserBreakpoint.h"
#include "Watchpoint.h"


// team event types
enum {
	TEAM_EVENT_TEAM_RENAMED,

	TEAM_EVENT_THREAD_ADDED,
	TEAM_EVENT_THREAD_REMOVED,
	TEAM_EVENT_IMAGE_ADDED,
	TEAM_EVENT_IMAGE_REMOVED,

	TEAM_EVENT_THREAD_STATE_CHANGED,
	TEAM_EVENT_THREAD_CPU_STATE_CHANGED,
	TEAM_EVENT_THREAD_STACK_TRACE_CHANGED,

	TEAM_EVENT_IMAGE_DEBUG_INFO_CHANGED,

	TEAM_EVENT_IMAGE_LOAD_SETTINGS_CHANGED,
	TEAM_EVENT_IMAGE_LOAD_NAME_ADDED,
	TEAM_EVENT_IMAGE_LOAD_NAME_REMOVED,

	TEAM_EVENT_DEFAULT_SIGNAL_DISPOSITION_CHANGED,
	TEAM_EVENT_CUSTOM_SIGNAL_DISPOSITION_CHANGED,
	TEAM_EVENT_CUSTOM_SIGNAL_DISPOSITION_REMOVED,

	TEAM_EVENT_CONSOLE_OUTPUT_RECEIVED,

	TEAM_EVENT_BREAKPOINT_ADDED,
	TEAM_EVENT_BREAKPOINT_REMOVED,
	TEAM_EVENT_USER_BREAKPOINT_CHANGED,

	TEAM_EVENT_WATCHPOINT_ADDED,
	TEAM_EVENT_WATCHPOINT_REMOVED,
	TEAM_EVENT_WATCHPOINT_CHANGED,

	TEAM_EVENT_DEBUG_REPORT_CHANGED,

	TEAM_EVENT_CORE_FILE_CHANGED,

	TEAM_EVENT_MEMORY_CHANGED
};


class Architecture;
class Breakpoint;
class BStringList;
class Function;
class FunctionID;
class FunctionInstance;
class LocatableFile;
class SourceCode;
class SourceLocation;
class Statement;
class TeamDebugInfo;
class TeamMemory;
class TeamTypeInformation;
class UserBreakpoint;
class Value;


typedef std::map<int32, int32> SignalDispositionMappings;


class Team {
public:
			class Event;
			class BreakpointEvent;
			class ConsoleOutputEvent;
			class DebugReportEvent;
			class MemoryChangedEvent;
			class ImageEvent;
			class ImageLoadEvent;
			class ImageLoadNameEvent;
			class DefaultSignalDispositionEvent;
			class CustomSignalDispositionEvent;
			class ThreadEvent;
			class UserBreakpointEvent;
			class WatchpointEvent;
			class CoreFileChangedEvent;
			class Listener;

public:
								Team(team_id teamID, TeamMemory* teamMemory,
									Architecture* architecture,
									TeamDebugInfo* debugInfo,
									TeamTypeInformation* typeInformation);
								~Team();

			status_t			Init();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			team_id				ID() const			{ return fID; }
			TeamMemory*			GetTeamMemory() const
									{ return fTeamMemory; }
			Architecture*		GetArchitecture() const
									{ return fArchitecture; }
			TeamDebugInfo*		DebugInfo() const	{ return fDebugInfo; }
			TeamTypeInformation*
								GetTeamTypeInformation() const
									{ return fTypeInformation; }

			const char*			Name() const	{ return fName.String(); }
			void				SetName(const BString& name);

			void				AddThread(::Thread* thread);
			status_t			AddThread(const ThreadInfo& threadInfo,
									::Thread** _thread = NULL);
			void				RemoveThread(::Thread* thread);
			bool				RemoveThread(thread_id threadID);
			::Thread*			ThreadByID(thread_id threadID) const;
			const ThreadList&	Threads() const;

			status_t			AddImage(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									Image** _image = NULL);
			void				RemoveImage(Image* image);
			bool				RemoveImage(image_id imageID);
			Image*				ImageByID(image_id imageID) const;
			Image*				ImageByAddress(target_addr_t address) const;
			const ImageList&	Images() const;
			void				ClearImages();

			bool				AddStopImageName(const BString& name);
			void				RemoveStopImageName(const BString& name);
			const BStringList&	StopImageNames() const;

			void				SetStopOnImageLoad(bool enabled,
									bool useImageNameList);
			bool				StopOnImageLoad() const
									{ return fStopOnImageLoad; }
			bool				StopImageNameListEnabled() const
									{ return fStopImageNameListEnabled; }

			void				SetDefaultSignalDisposition(int32 disposition);
			int32				DefaultSignalDisposition() const
									{ return fDefaultSignalDisposition; }
			bool				SetCustomSignalDisposition(int32 signal,
									int32 disposition);
			void				RemoveCustomSignalDisposition(int32 signal);
			int32				SignalDispositionFor(int32 signal) const;
									// if no custom disposition is found,
									// returns default
			const SignalDispositionMappings&
								GetSignalDispositionMappings() const;

			void				ClearSignalDispositionMappings();

			bool				AddBreakpoint(Breakpoint* breakpoint);
									// takes over reference (also on error)
			void				RemoveBreakpoint(Breakpoint* breakpoint);
									// releases its own reference
			int32				CountBreakpoints() const;
			Breakpoint*			BreakpointAt(int32 index) const;
			Breakpoint*			BreakpointAtAddress(
									target_addr_t address) const;
			void				GetBreakpointsInAddressRange(
									TargetAddressRange range,
									BObjectList<UserBreakpoint>& breakpoints)
										const;
			void				GetBreakpointsForSourceCode(
									SourceCode* sourceCode,
									BObjectList<UserBreakpoint>& breakpoints)
										const;

			void				AddUserBreakpoint(
									UserBreakpoint* userBreakpoint);
			void				RemoveUserBreakpoint(
									UserBreakpoint* userBreakpoint);
			const UserBreakpointList& UserBreakpoints() const
									{ return fUserBreakpoints; }

			bool				AddWatchpoint(Watchpoint* watchpoint);
									// takes over reference (also on error)
			void				RemoveWatchpoint(Watchpoint* watchpoint);
									// releases its own reference
			int32				CountWatchpoints() const;
			Watchpoint*			WatchpointAt(int32 index) const;
			Watchpoint*			WatchpointAtAddress(
									target_addr_t address) const;
			void				GetWatchpointsInAddressRange(
									TargetAddressRange range,
									BObjectList<Watchpoint>& watchpoints)
										const;
			const WatchpointList& Watchpoints() const
									{ return fWatchpoints; }

			status_t			GetStatementAtAddress(target_addr_t address,
									FunctionInstance*& _function,
									Statement*& _statement);
									// returns a reference to the statement,
									// not to the functions instance, though,
									// caller must lock
			status_t			GetStatementAtSourceLocation(
									SourceCode* sourceCode,
									const SourceLocation& location,
									Statement*& _statement);
									// returns a reference to the statement
									// (any matching statement!),
									// caller must lock,

			Function*			FunctionByID(FunctionID* functionID) const;

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// service methods for Thread
			void				NotifyThreadStateChanged(::Thread* thread);
			void				NotifyThreadCpuStateChanged(::Thread* thread);
			void				NotifyThreadStackTraceChanged(
									::Thread* thread);

			// service methods for Image
			void				NotifyImageDebugInfoChanged(Image* image);

			// service methods for Image load settings
			void				NotifyStopOnImageLoadChanged(bool enabled,
									bool useImageNameList);
			void				NotifyStopImageNameAdded(const BString& name);
			void				NotifyStopImageNameRemoved(
									const BString& name);

			// service methods for Signal Disposition settings
			void				NotifyDefaultSignalDispositionChanged(
									int32 newDisposition);
			void				NotifyCustomSignalDispositionChanged(
									int32 signal, int32 disposition);
			void				NotifyCustomSignalDispositionRemoved(
									int32 signal);

			// service methods for console output
			void				NotifyConsoleOutputReceived(
									int32 fd, const BString& output);

			// breakpoint related service methods
			void				NotifyUserBreakpointChanged(
									UserBreakpoint* breakpoint);

			// watchpoint related service methods
			void				NotifyWatchpointChanged(
									Watchpoint* watchpoint);

			// debug report related service methods
			void				NotifyDebugReportChanged(
									const char* reportPath, status_t result);

			// core file related service methods
			void				NotifyCoreFileChanged(
									const char* targetPath);

			// memory write related service methods
			void				NotifyMemoryChanged(target_addr_t address,
									target_size_t size);

private:
			struct BreakpointByAddressPredicate;
			struct WatchpointByAddressPredicate;

			typedef BObjectList<Breakpoint> BreakpointList;
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_NotifyTeamRenamed();
			void				_NotifyThreadAdded(::Thread* thread);
			void				_NotifyThreadRemoved(::Thread* thread);
			void				_NotifyImageAdded(Image* image);
			void				_NotifyImageRemoved(Image* image);

private:
			BLocker				fLock;
			team_id				fID;
			TeamMemory*			fTeamMemory;
			TeamTypeInformation*
								fTypeInformation;
			Architecture*		fArchitecture;
			TeamDebugInfo*		fDebugInfo;
			BString				fName;
			ThreadList			fThreads;
			ImageList			fImages;
			bool				fStopOnImageLoad;
			bool				fStopImageNameListEnabled;
			BStringList			fStopImageNames;
			int32				fDefaultSignalDisposition;
			SignalDispositionMappings
								fCustomSignalDispositions;
			BreakpointList		fBreakpoints;
			WatchpointList		fWatchpoints;
			UserBreakpointList	fUserBreakpoints;
			ListenerList		fListeners;
};


class Team::Event {
public:
								Event(uint32 type, Team* team);

			uint32				EventType() const	{ return fEventType; }
			Team*				GetTeam() const		{ return fTeam; }

protected:
			uint32				fEventType;
			Team*				fTeam;
};


class Team::ThreadEvent : public Event {
public:
								ThreadEvent(uint32 type, ::Thread* thread);

			::Thread*			GetThread() const	{ return fThread; }

protected:
			::Thread*			fThread;
};


class Team::ImageEvent : public Event {
public:
								ImageEvent(uint32 type, Image* image);

			Image*				GetImage() const	{ return fImage; }

protected:
			Image*				fImage;
};


class Team::ImageLoadEvent : public Event {
public:
								ImageLoadEvent(uint32 type, Team* team,
									bool stopOnImageLoad,
									bool stopImageNameListEnabled);

			bool				StopOnImageLoad() const
									{ return fStopOnImageLoad; }
			bool				StopImageNameListEnabled() const
									{ return fStopImageNameListEnabled; }

private:
			bool				fStopOnImageLoad;
			bool				fStopImageNameListEnabled;
};


class Team::ImageLoadNameEvent : public Event {
public:
								ImageLoadNameEvent(uint32 type, Team* team,
									const BString& name);

			const BString&		ImageName() const { return fImageName; }

private:
			BString				fImageName;
};


class Team::DefaultSignalDispositionEvent : public Event {
public:
								DefaultSignalDispositionEvent(uint32 type,
									Team* team, int32 disposition);

			int32				DefaultDisposition() const
									{ return fDefaultDisposition; }

private:
			int32				fDefaultDisposition;
};


class Team::CustomSignalDispositionEvent : public Event {
public:
								CustomSignalDispositionEvent(uint32 type,
									Team* team, int32 signal,
									int32 disposition);

			int32				Signal() const { return fSignal; }
			int32				Disposition() const { return fDisposition; }

private:
			int32				fSignal;
			int32				fDisposition;
};


class Team::BreakpointEvent : public Event {
public:
								BreakpointEvent(uint32 type, Team* team,
									Breakpoint* breakpoint);

			Breakpoint*			GetBreakpoint() const	{ return fBreakpoint; }

protected:
			Breakpoint*			fBreakpoint;
};


class Team::ConsoleOutputEvent : public Event {
public:
								ConsoleOutputEvent(uint32 type, Team* team,
									int32 fd, const BString& output);

			int32				Descriptor() const	{ return fDescriptor; }
			const BString&		Output() const		{ return fOutput; }

protected:
			int32				fDescriptor;
			BString				fOutput;
};


class Team::DebugReportEvent : public Event {
public:
								DebugReportEvent(uint32 type, Team* team,
									const char* reportPath,
									status_t finalStatus);

			const char*			GetReportPath() const	{ return fReportPath; }
			status_t			GetFinalStatus() const	{ return fFinalStatus; }
protected:
			const char*			fReportPath;
			status_t			fFinalStatus;
};


class Team::CoreFileChangedEvent : public Event {
public:
								CoreFileChangedEvent(uint32 type, Team* team,
									const char* targetPath);
			const char*			GetTargetPath() const	{ return fTargetPath; }
protected:
			const char*			fTargetPath;
};


class Team::MemoryChangedEvent : public Event {
public:
								MemoryChangedEvent(uint32 type, Team* team,
									target_addr_t address, target_size_t size);

			target_addr_t		GetTargetAddress() const
									{ return fTargetAddress; }

			target_size_t		GetSize() const	{ return fSize; }
protected:
			target_addr_t		fTargetAddress;
			target_size_t		fSize;
};


class Team::WatchpointEvent : public Event {
public:
								WatchpointEvent(uint32 type, Team* team,
									Watchpoint* watchpoint);

			Watchpoint*			GetWatchpoint() const	{ return fWatchpoint; }

protected:
			Watchpoint*			fWatchpoint;
};


class Team::UserBreakpointEvent : public Event {
public:
								UserBreakpointEvent(uint32 type, Team* team,
									UserBreakpoint* breakpoint);

			UserBreakpoint*		GetBreakpoint() const	{ return fBreakpoint; }

protected:
			UserBreakpoint*		fBreakpoint;
};


class Team::Listener : public DoublyLinkedListLinkImpl<Team::Listener> {
public:
	virtual						~Listener();

	virtual	void				TeamRenamed(
									const Team::Event& event);

	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadRemoved(const Team::ThreadEvent& event);

	virtual	void				ImageAdded(const Team::ImageEvent& event);
	virtual	void				ImageRemoved(const Team::ImageEvent& event);

	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	virtual	void				ImageDebugInfoChanged(
									const Team::ImageEvent& event);

	virtual	void				StopOnImageLoadSettingsChanged(
									const Team::ImageLoadEvent& event);
	virtual	void				StopOnImageLoadNameAdded(
									const Team::ImageLoadNameEvent& event);
	virtual	void				StopOnImageLoadNameRemoved(
									const Team::ImageLoadNameEvent& event);

	virtual	void				DefaultSignalDispositionChanged(
									const Team::DefaultSignalDispositionEvent&
										event);
	virtual	void				CustomSignalDispositionChanged(
									const Team::CustomSignalDispositionEvent&
										event);
	virtual	void				CustomSignalDispositionRemoved(
									const Team::CustomSignalDispositionEvent&
										event);

	virtual	void				ConsoleOutputReceived(
									const Team::ConsoleOutputEvent& event);

	virtual	void				BreakpointAdded(
									const Team::BreakpointEvent& event);
	virtual	void				BreakpointRemoved(
									const Team::BreakpointEvent& event);
	virtual	void				UserBreakpointChanged(
									const Team::UserBreakpointEvent& event);

	virtual	void				WatchpointAdded(
									const Team::WatchpointEvent& event);
	virtual	void				WatchpointRemoved(
									const Team::WatchpointEvent& event);
	virtual	void				WatchpointChanged(
									const Team::WatchpointEvent& event);

	virtual void				DebugReportChanged(
									const Team::DebugReportEvent& event);

	virtual	void				CoreFileChanged(
									const Team::CoreFileChangedEvent& event);

	virtual	void				MemoryChanged(
									const Team::MemoryChangedEvent& event);
};


#endif	// TEAM_H
