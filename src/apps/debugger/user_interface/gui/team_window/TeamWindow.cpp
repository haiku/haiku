/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2017, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <StringView.h>
#include <TabView.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <TextView.h>
#include <VolumeRoster.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "Breakpoint.h"
#include "BreakpointEditWindow.h"
#include "ConsoleOutputView.h"
#include "CppLanguage.h"
#include "CpuState.h"
#include "DisassembledCode.h"
#include "BreakpointEditWindow.h"
#include "ExpressionEvaluationWindow.h"
#include "ExpressionPromptWindow.h"
#include "FileSourceCode.h"
#include "GuiSettingsUtils.h"
#include "GuiTeamUiSettings.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "InspectorWindow.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "RegistersView.h"
#include "StackTrace.h"
#include "StackTraceView.h"
#include "TeamSettingsWindow.h"
#include "Tracing.h"
#include "TypeComponentPath.h"
#include "UiUtils.h"
#include "UserInterface.h"
#include "ValueNodeManager.h"
#include "Value.h"
#include "Variable.h"
#include "WatchPromptWindow.h"


enum {
	MAIN_TAB_INDEX_THREADS	= 0,
	MAIN_TAB_INDEX_IMAGES	= 1
};


enum {
	MSG_CHOOSE_DEBUG_REPORT_LOCATION	= 'ccrl',
	MSG_DEBUG_REPORT_SAVED				= 'drsa',
	MSG_LOCATE_SOURCE_IF_NEEDED			= 'lsin',
	MSG_SOURCE_ENTRY_QUERY_COMPLETE		= 'seqc',
	MSG_CLEAR_STACK_TRACE				= 'clst',
	MSG_HANDLE_LOAD_SETTINGS			= 'hlst',
	MSG_UPDATE_STATUS_BAR				= 'upsb'
};


// #pragma mark - ThreadStackFrameSelectionKey


struct TeamWindow::ThreadStackFrameSelectionKey {
	::Thread*	thread;

	ThreadStackFrameSelectionKey(::Thread* thread)
		:
		thread(thread)
	{
		thread->AcquireReference();
	}

	~ThreadStackFrameSelectionKey()
	{
		thread->ReleaseReference();
	}

	uint32 HashValue() const
	{
		return (uint32)thread->ID();
	}

	bool operator==(const ThreadStackFrameSelectionKey& other) const
	{
		return thread == other.thread;
	}
};


// #pragma mark - ThreadStackFrameSelectionEntry


struct TeamWindow::ThreadStackFrameSelectionEntry
	: ThreadStackFrameSelectionKey {
	ThreadStackFrameSelectionEntry* next;
	StackFrame* selectedFrame;

	ThreadStackFrameSelectionEntry(::Thread* thread, StackFrame* frame)
		:
		ThreadStackFrameSelectionKey(thread),
		selectedFrame(frame)
	{
	}

	inline StackFrame* SelectedFrame() const
	{
		return selectedFrame;
	}

	void SetSelectedFrame(StackFrame* frame)
	{
		selectedFrame = frame;
	}
};


// #pragma mark - ThreadStackFrameSelectionEntryHashDefinition


struct TeamWindow::ThreadStackFrameSelectionEntryHashDefinition {
	typedef ThreadStackFrameSelectionKey 	KeyType;
	typedef ThreadStackFrameSelectionEntry	ValueType;

	size_t HashKey(const ThreadStackFrameSelectionKey& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const ThreadStackFrameSelectionKey* value) const
	{
		return value->HashValue();
	}

	bool Compare(const ThreadStackFrameSelectionKey& key,
		const ThreadStackFrameSelectionKey* value) const
	{
		return key == *value;
	}

	ThreadStackFrameSelectionEntry*& GetLink(
		ThreadStackFrameSelectionEntry* value) const
	{
		return value->next;
	}
};


// #pragma mark - PathViewMessageFilter


class PathViewMessageFilter : public BMessageFilter {
public:
		PathViewMessageFilter(BMessenger teamWindow)
			:
			BMessageFilter(B_MOUSE_UP),
			fTeamWindowMessenger(teamWindow)
		{
		}

		virtual filter_result Filter(BMessage*, BHandler**)
		{
			fTeamWindowMessenger.SendMessage(MSG_LOCATE_SOURCE_IF_NEEDED);

			return B_DISPATCH_MESSAGE;
		}

private:
		BMessenger fTeamWindowMessenger;
};


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(::Team* team, UserInterfaceListener* listener)
	:
	BWindow(BRect(100, 100, 899, 699), "Team", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTeam(team),
	fActiveThread(NULL),
	fActiveImage(NULL),
	fActiveStackTrace(NULL),
	fActiveStackFrame(NULL),
	fThreadSelectionInfoTable(NULL),
	fActiveBreakpoint(NULL),
	fActiveFunction(NULL),
	fActiveSourceCode(NULL),
	fActiveSourceObject(ACTIVE_SOURCE_NONE),
	fListener(listener),
	fTraceUpdateRunner(NULL),
	fTabView(NULL),
	fLocalsTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL),
	fImageFunctionsView(NULL),
	fBreakpointsView(NULL),
	fVariablesView(NULL),
	fRegistersView(NULL),
	fStackTraceView(NULL),
	fSourceView(NULL),
	fRunButton(NULL),
	fStepOverButton(NULL),
	fStepIntoButton(NULL),
	fStepOutButton(NULL),
	fMenuBar(NULL),
	fSourcePathView(NULL),
	fStatusBarView(NULL),
	fConsoleOutputView(NULL),
	fFunctionSplitView(NULL),
	fSourceSplitView(NULL),
	fImageSplitView(NULL),
	fThreadSplitView(NULL),
	fConsoleSplitView(NULL),
	fTeamSettingsWindow(NULL),
	fBreakpointEditWindow(NULL),
	fInspectorWindow(NULL),
	fExpressionEvalWindow(NULL),
	fExpressionPromptWindow(NULL),
	fFilePanel(NULL),
	fActiveSourceWorker(-1)
{
	_UpdateTitle();

	fTeam->AddListener(this);
}


TeamWindow::~TeamWindow()
{
	if (fThreadListView != NULL)
		fThreadListView->UnsetListener();
	if (fStackTraceView != NULL)
		fStackTraceView->UnsetListener();
	if (fSourceView != NULL)
		fSourceView->UnsetListener();
	if (fInspectorWindow != NULL) {
		if (fInspectorWindow->Lock())
			fInspectorWindow->Quit();
	}
	if (fExpressionEvalWindow != NULL) {
		if (fExpressionEvalWindow->Lock())
			fExpressionEvalWindow->Quit();
	}
	if (fExpressionPromptWindow != NULL) {
		if (fExpressionPromptWindow->Lock())
			fExpressionPromptWindow->Quit();
	}

	fTeam->RemoveListener(this);

	_SetActiveSourceCode(NULL);
	_SetActiveFunction(NULL);
	_SetActiveBreakpoint(NULL);
	_SetActiveStackFrame(NULL);
	_SetActiveStackTrace(NULL);
	_SetActiveImage(NULL);
	_SetActiveThread(NULL);

	delete fFilePanel;

	ThreadStackFrameSelectionEntry* entry
		= fThreadSelectionInfoTable->Clear(true);

	while (entry != NULL) {
		ThreadStackFrameSelectionEntry* next = entry->next;
		delete entry;
		entry = next;
	}

	delete fThreadSelectionInfoTable;

	if (fActiveSourceWorker > 0)
		wait_for_thread(fActiveSourceWorker, NULL);
}


/*static*/ TeamWindow*
TeamWindow::Create(::Team* team, UserInterfaceListener* listener)
{
	TeamWindow* self = new TeamWindow(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	// Handle function key shortcuts for stepping
	switch (message->what) {
		case B_KEY_DOWN:
			if (fActiveThread != NULL && fTraceUpdateRunner == NULL) {
				int32 key;
				uint32 modifiers;
				if (message->FindInt32("key", &key) == B_OK
					&& message->FindInt32("modifiers", (int32*)&modifiers)
					== B_OK) {
					switch (key) {
						case B_F5_KEY:
							fListener->ThreadActionRequested(
								fActiveThread->ID(), MSG_THREAD_RUN);
							break;
						case B_F10_KEY:
							fListener->ThreadActionRequested(
								fActiveThread->ID(), MSG_THREAD_STEP_OVER);
							break;
						case B_F11_KEY:
							if ((modifiers & B_SHIFT_KEY) != 0) {
								fListener->ThreadActionRequested(
									fActiveThread->ID(), MSG_THREAD_STEP_OUT);
							} else {
								fListener->ThreadActionRequested(
									fActiveThread->ID(), MSG_THREAD_STEP_INTO);
							}
							break;
						default:
							break;
					}
				}
			}
			break;

		case B_COPY:
		case B_SELECT_ALL:
			BView* focusView = CurrentFocus();
			if (focusView != NULL) {
				focusView->MessageReceived(message);
				return;
			}
			break;
	}
	BWindow::DispatchMessage(message, handler);
}


void
TeamWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TEAM_RESTART_REQUESTED:
		{
			fListener->TeamRestartRequested();
			break;
		}
		case MSG_CHOOSE_DEBUG_REPORT_LOCATION:
		{
			try {
				char filename[B_FILE_NAME_LENGTH];
				UiUtils::ReportNameForTeam(fTeam, filename, sizeof(filename));
				BMessenger msgr(this);
				fFilePanel = new BFilePanel(B_SAVE_PANEL, &msgr,
					NULL, 0, false, new BMessage(MSG_GENERATE_DEBUG_REPORT));
				fFilePanel->SetSaveText(filename);
				fFilePanel->Show();
			} catch (...) {
				delete fFilePanel;
				fFilePanel = NULL;
			}
			break;
		}
		case MSG_GENERATE_DEBUG_REPORT:
		{
			delete fFilePanel;
			fFilePanel = NULL;

			BPath path;
			entry_ref ref;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->HasString("name")) {
				path.SetTo(&ref);
				path.Append(message->FindString("name"));
				if (get_ref_for_path(path.Path(), &ref) == B_OK)
					fListener->DebugReportRequested(&ref);
			}
			break;
		}
		case MSG_DEBUG_REPORT_SAVED:
		{
			status_t finalStatus = message->GetInt32("status", B_OK);
			BString data;
			if (finalStatus == B_OK) {
				data.SetToFormat("Debug report successfully saved to '%s'",
					message->FindString("path"));
			} else {
				data.SetToFormat("Failed to save debug report: '%s'",
					strerror(finalStatus));
			}

			BAlert *alert = new(std::nothrow) BAlert("Report saved",
				data.String(), "Close");
			if (alert == NULL)
				break;

			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			break;
		}
		case MSG_SHOW_INSPECTOR_WINDOW:
		{
			if (fInspectorWindow) {
				AutoLocker<BWindow> lock(fInspectorWindow);
				if (lock.IsLocked())
					fInspectorWindow->Activate(true);
			} else {
				try {
					fInspectorWindow = InspectorWindow::Create(fTeam,
						fListener, this);
					if (fInspectorWindow != NULL) {
						fInspectorWindow->LoadSettings(fUiSettings);
						fInspectorWindow->Show();
					}
				} catch (...) {
					// TODO: notify user
				}
			}

			target_addr_t address;
			if (message->FindUInt64("address", &address) == B_OK) {
				BMessage addressMessage(MSG_INSPECT_ADDRESS);
				addressMessage.AddUInt64("address", address);
				fInspectorWindow->PostMessage(&addressMessage);
			}
			break;
		}
		case MSG_INSPECTOR_WINDOW_CLOSED:
		{
			_SaveInspectorSettings(CurrentMessage());
			fInspectorWindow = NULL;
			break;

		}
		case MSG_SHOW_EXPRESSION_WINDOW:
		{
			if (fExpressionEvalWindow != NULL) {
				AutoLocker<BWindow> lock(fExpressionEvalWindow);
				if (lock.IsLocked())
					fExpressionEvalWindow->Activate(true);
			} else {
				try {
					fExpressionEvalWindow = ExpressionEvaluationWindow::Create(
						this, fTeam, fListener);
					if (fExpressionEvalWindow != NULL)
						fExpressionEvalWindow->Show();
				} catch (...) {
					// TODO: notify user
				}
			}
			break;
		}
		case MSG_EXPRESSION_WINDOW_CLOSED:
		{
			fExpressionEvalWindow = NULL;
			break;
		}
		case MSG_SHOW_EXPRESSION_PROMPT_WINDOW:
		{
			if (fExpressionPromptWindow != NULL) {
				AutoLocker<BWindow> lock(fExpressionPromptWindow);
				if (lock.IsLocked())
					fExpressionPromptWindow->Activate(true);
			} else {
				try {
					fExpressionPromptWindow = ExpressionPromptWindow::Create(
						fVariablesView, this);
					if (fExpressionPromptWindow != NULL)
						fExpressionPromptWindow->Show();
				} catch (...) {
					// TODO: notify user
				}
			}
			break;
		}
		case MSG_EXPRESSION_PROMPT_WINDOW_CLOSED:
		{
			fExpressionPromptWindow = NULL;
			break;
		}
		case MSG_SHOW_TEAM_SETTINGS_WINDOW:
		{
			if (fTeamSettingsWindow != NULL) {
				AutoLocker<BWindow> lock(fTeamSettingsWindow);
				if (lock.IsLocked())
					fTeamSettingsWindow->Activate(true);
			} else {
				try {
					fTeamSettingsWindow
						= TeamSettingsWindow::Create(
						fTeam, fListener, this);
					if (fTeamSettingsWindow != NULL)
						fTeamSettingsWindow->Show();
				} catch (...) {
					// TODO: notify user
				}
			}
			break;
		}
		case MSG_TEAM_SETTINGS_WINDOW_CLOSED:
		{
			fTeamSettingsWindow = NULL;
			break;
		}
		case MSG_SHOW_BREAKPOINT_EDIT_WINDOW:
		{
			if (fBreakpointEditWindow != NULL) {
				AutoLocker<BWindow> lock(fBreakpointEditWindow);
				if (lock.IsLocked())
					fBreakpointEditWindow->Activate(true);
			} else {
				UserBreakpoint* breakpoint;
				if (message->FindPointer("breakpoint",
					reinterpret_cast<void**>(&breakpoint)) != B_OK) {
					break;
				}

				try {
					fBreakpointEditWindow
						= BreakpointEditWindow::Create(
						fTeam, breakpoint, fListener, this);
					if (fBreakpointEditWindow != NULL)
						fBreakpointEditWindow->Show();
				} catch (...) {
					// TODO: notify user
				}
			}
			break;
		}
		case MSG_BREAKPOINT_EDIT_WINDOW_CLOSED:
		{
			fBreakpointEditWindow = NULL;
			break;
		}
		case MSG_SHOW_WATCH_VARIABLE_PROMPT:
		{
			target_addr_t address;
			uint32 type;
			int32 length;

			if (message->FindUInt64("address", &address) != B_OK
				|| message->FindUInt32("type", &type) != B_OK
				|| message->FindInt32("length", &length) != B_OK) {
				break;
			}

			try {
				WatchPromptWindow* window = WatchPromptWindow::Create(
					fTeam->GetArchitecture(), address, type, length,
					fListener);
				window->Show();
			} catch (...) {
				// TODO: notify user
			}
			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref locatedPath;
			if (message->FindRef("refs", &locatedPath) != B_OK)
				break;

			_HandleResolveMissingSourceFile(locatedPath);
			delete fFilePanel;
			fFilePanel = NULL;
			break;
		}
		case MSG_LOCATE_SOURCE_IF_NEEDED:
		{
			_HandleLocateSourceRequest();
			break;
		}
		case MSG_SOURCE_ENTRY_QUERY_COMPLETE:
		{
			BStringList* entries;
			if (message->FindPointer("entries", (void**)&entries) == B_OK) {
				ObjectDeleter<BStringList> entryDeleter(entries);
				_HandleLocateSourceRequest(entries);
			}
			fActiveSourceWorker = -1;
			break;
		}
		case MSG_THREAD_RUN:
		case MSG_THREAD_STOP:
		case MSG_THREAD_STEP_OVER:
		case MSG_THREAD_STEP_INTO:
		case MSG_THREAD_STEP_OUT:
			if (fActiveThread != NULL && fTraceUpdateRunner == NULL) {
				fListener->ThreadActionRequested(fActiveThread->ID(),
					message->what);
			}
			break;

		case MSG_CLEAR_STACK_TRACE:
		{
			if (fTraceUpdateRunner != NULL) {
				_SetActiveStackTrace(NULL);
				_UpdateRunButtons();
			}
			break;
		}
		case MSG_HANDLE_LOAD_SETTINGS:
		{
			GuiTeamUiSettings* settings;
			if (message->FindPointer("settings",
				reinterpret_cast<void**>(&settings)) != B_OK) {
				break;
			}

			_LoadSettings(settings);
			break;
		}
		case MSG_UPDATE_STATUS_BAR:
		{
			const char* messageText;
			if (message->FindString("message", &messageText) == B_OK)
				fStatusBarView->SetText(messageText);
			break;
		}
		case MSG_TEAM_RENAMED:
		{
			_UpdateTitle();
			break;
		}
		case MSG_THREAD_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleThreadStateChanged(threadID);
			break;
		}
		case MSG_THREAD_CPU_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleCpuStateChanged(threadID);
			break;
		}

		case MSG_THREAD_STACK_TRACE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleStackTraceChanged(threadID);
			break;
		}

		case MSG_IMAGE_DEBUG_INFO_CHANGED:
		{
			int32 imageID;
			if (message->FindInt32("image", &imageID) != B_OK)
				break;

			_HandleImageDebugInfoChanged(imageID);
			break;
		}

		case MSG_CONSOLE_OUTPUT_RECEIVED:
		{
			int32 fd;
			BString output;
			if (message->FindInt32("fd", &fd) != B_OK
					|| message->FindString("output", &output) != B_OK) {
				break;
			}
			fConsoleOutputView->ConsoleOutputReceived(fd, output);
			break;
		}

		case MSG_USER_BREAKPOINT_CHANGED:
		{
			UserBreakpoint* breakpoint;
			if (message->FindPointer("breakpoint", (void**)&breakpoint) != B_OK)
				break;
			BReference<UserBreakpoint> breakpointReference(breakpoint, true);

			_HandleUserBreakpointChanged(breakpoint);
			break;
		}

		case MSG_WATCHPOINT_CHANGED:
		{
			Watchpoint* watchpoint;
			if (message->FindPointer("watchpoint", (void**)&watchpoint) != B_OK)
				break;
			BReference<Watchpoint> watchpointReference(watchpoint, true);

			_HandleWatchpointChanged(watchpoint);
			break;

		}

		case MSG_FUNCTION_SOURCE_CODE_CHANGED:
		{
			_HandleSourceCodeChanged();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamWindow::QuitRequested()
{
	fListener->UserInterfaceQuitRequested();

	return false;
}


status_t
TeamWindow::LoadSettings(const GuiTeamUiSettings* settings)
{
	BMessage message(MSG_HANDLE_LOAD_SETTINGS);
	message.AddPointer("settings", settings);
	return PostMessage(&message);
}


status_t
TeamWindow::SaveSettings(GuiTeamUiSettings* settings)
{
	AutoLocker<BWindow> lock(this);
	if (!lock.IsLocked())
		return B_ERROR;

	BMessage inspectorSettings;
	if (fUiSettings.Settings("inspectorWindow", inspectorSettings) == B_OK) {
		if (!settings->AddSettings("inspectorWindow", inspectorSettings))
			return B_NO_MEMORY;
	}

	BMessage archive;
	BMessage teamWindowSettings;
	if (teamWindowSettings.AddRect("frame", Frame()) != B_OK)
		return B_NO_MEMORY;

	if (GuiSettingsUtils::ArchiveSplitView(archive, fSourceSplitView) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("sourceSplit", &archive) != B_OK)
		return B_NO_MEMORY;

	if (GuiSettingsUtils::ArchiveSplitView(archive, fFunctionSplitView) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("functionSplit", &archive) != B_OK)
		return B_NO_MEMORY;

	if (GuiSettingsUtils::ArchiveSplitView(archive, fImageSplitView) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("imageSplit", &archive))
		return B_NO_MEMORY;

	if (GuiSettingsUtils::ArchiveSplitView(archive, fThreadSplitView) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("threadSplit", &archive))
		return B_NO_MEMORY;

	if (GuiSettingsUtils::ArchiveSplitView(archive, fConsoleSplitView) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("consoleSplit", &archive))
		return B_NO_MEMORY;

	if (fImageListView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("imageListView", &archive))
		return B_NO_MEMORY;

	if (fImageFunctionsView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("imageFunctionsView", &archive))
		return B_NO_MEMORY;

	if (fThreadListView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("threadListView", &archive))
		return B_NO_MEMORY;

	if (fVariablesView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("variablesView", &archive))
		return B_NO_MEMORY;

	if (fRegistersView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("registersView", &archive))
		return B_NO_MEMORY;

	if (fStackTraceView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("stackTraceView", &archive))
		return B_NO_MEMORY;

	if (fBreakpointsView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("breakpointsView", &archive))
		return B_NO_MEMORY;

	if (fConsoleOutputView->SaveSettings(archive) != B_OK)
		return B_NO_MEMORY;
	if (teamWindowSettings.AddMessage("consoleOutputView", &archive))
		return B_NO_MEMORY;

	if (!settings->AddSettings("teamWindow", teamWindowSettings))
		return B_NO_MEMORY;

	return B_OK;
}


void
TeamWindow::DisplayBackgroundStatus(const char* message)
{
	BMessage updateMessage(MSG_UPDATE_STATUS_BAR);
	updateMessage.AddString("message", message);
	PostMessage(&updateMessage);
}


void
TeamWindow::ThreadSelectionChanged(::Thread* thread)
{
	_SetActiveThread(thread);
}


void
TeamWindow::ImageSelectionChanged(Image* image)
{
	_SetActiveImage(image);
}


void
TeamWindow::StackFrameSelectionChanged(StackFrame* frame)
{
	_SetActiveStackFrame(frame);
}


void
TeamWindow::FunctionSelectionChanged(FunctionInstance* function)
{
	// If the function wasn't already active, it was just selected by the user.
	if (function != NULL && function != fActiveFunction)
		fActiveSourceObject = ACTIVE_SOURCE_FUNCTION;

	_SetActiveFunction(function);
}


void
TeamWindow::BreakpointSelectionChanged(BreakpointProxyList &proxies)
{
	if (proxies.CountItems() == 0 && fActiveBreakpoint != NULL) {
		fActiveBreakpoint->ReleaseReference();
		fActiveBreakpoint = NULL;
	} else if (proxies.CountItems() == 1) {
		BreakpointProxy* proxy = proxies.ItemAt(0);
		if (proxy->Type() == BREAKPOINT_PROXY_TYPE_BREAKPOINT)
			_SetActiveBreakpoint(proxy->GetBreakpoint());
	}
	// if more than one item is selected, do nothing.
}


void
TeamWindow::SetBreakpointEnabledRequested(UserBreakpoint* breakpoint,
	bool enabled)
{
	fListener->SetBreakpointEnabledRequested(breakpoint, enabled);
}


void
TeamWindow::ClearBreakpointRequested(UserBreakpoint* breakpoint)
{
	fListener->ClearBreakpointRequested(breakpoint);
}


void
TeamWindow::SetBreakpointRequested(target_addr_t address, bool enabled)
{
	fListener->SetBreakpointRequested(address, enabled);
}


void
TeamWindow::ClearBreakpointRequested(target_addr_t address)
{
	fListener->ClearBreakpointRequested(address);
}


void
TeamWindow::ThreadActionRequested(::Thread* thread, uint32 action,
	target_addr_t address)
{
	if (fTraceUpdateRunner == NULL)
		fListener->ThreadActionRequested(thread->ID(), action, address);
}


void
TeamWindow::FunctionSourceCodeRequested(FunctionInstance* function,
	bool forceDisassembly)
{
	fListener->FunctionSourceCodeRequested(function, forceDisassembly);
}


void
TeamWindow::SetWatchpointEnabledRequested(Watchpoint* watchpoint,
	bool enabled)
{
	fListener->SetWatchpointEnabledRequested(watchpoint, enabled);
}


void
TeamWindow::ClearWatchpointRequested(Watchpoint* watchpoint)
{
	fListener->ClearWatchpointRequested(watchpoint);
}


void
TeamWindow::ValueNodeValueRequested(CpuState* cpuState,
	ValueNodeContainer* container, ValueNode* valueNode)
{
	fListener->ValueNodeValueRequested(cpuState, container, valueNode);
}


void
TeamWindow::ExpressionEvaluationRequested(ExpressionInfo* info,
	StackFrame* frame, ::Thread* thread)
{
	SourceLanguage* language;
	if (_GetActiveSourceLanguage(language) != B_OK)
		return;

	BReference<SourceLanguage> languageReference(language, true);
	fListener->ExpressionEvaluationRequested(language, info, frame, thread);
}


void
TeamWindow::ValueNodeWriteRequested(ValueNode* node, CpuState* state,
	Value* newValue)
{
	fListener->ValueNodeWriteRequested(node, state, newValue);
}


void
TeamWindow::TeamRenamed(const Team::Event& event)
{
	PostMessage(MSG_TEAM_RENAMED);
}


void
TeamWindow::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ThreadCpuStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_CPU_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ThreadStackTraceChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ImageDebugInfoChanged(const Team::ImageEvent& event)
{
	BMessage message(MSG_IMAGE_DEBUG_INFO_CHANGED);
	message.AddInt32("image", event.GetImage()->ID());
	PostMessage(&message);
}


void
TeamWindow::ConsoleOutputReceived(const Team::ConsoleOutputEvent& event)
{
	BMessage message(MSG_CONSOLE_OUTPUT_RECEIVED);
	message.AddInt32("fd", event.Descriptor());
	message.AddString("output", event.Output());
	PostMessage(&message);
}


void
TeamWindow::UserBreakpointChanged(const Team::UserBreakpointEvent& event)
{
	BMessage message(MSG_USER_BREAKPOINT_CHANGED);
	BReference<UserBreakpoint> breakpointReference(event.GetBreakpoint());
	if (message.AddPointer("breakpoint", event.GetBreakpoint()) == B_OK
		&& PostMessage(&message) == B_OK) {
		breakpointReference.Detach();
	}
}


void
TeamWindow::WatchpointChanged(const Team::WatchpointEvent& event)
{
	BMessage message(MSG_WATCHPOINT_CHANGED);
	BReference<Watchpoint> watchpointReference(event.GetWatchpoint());
	if (message.AddPointer("watchpoint", event.GetWatchpoint()) == B_OK
		&& PostMessage(&message) == B_OK) {
		watchpointReference.Detach();
	}
}


void
TeamWindow::DebugReportChanged(const Team::DebugReportEvent& event)
{
	BMessage message(MSG_DEBUG_REPORT_SAVED);
	message.AddString("path", event.GetReportPath());
	message.AddInt32("status", event.GetFinalStatus());
	PostMessage(&message);
}


void
TeamWindow::FunctionSourceCodeChanged(Function* function)
{
	TRACE_GUI("TeamWindow::FunctionSourceCodeChanged(%p): source: %p, "
		"state: %d\n", function, function->GetSourceCode(),
		function->SourceCodeState());

	PostMessage(MSG_FUNCTION_SOURCE_CODE_CHANGED);
}


void
TeamWindow::_Init()
{
	fThreadSelectionInfoTable = new ThreadStackFrameSelectionInfoTable;
	if (fThreadSelectionInfoTable->Init() != B_OK) {
		delete fThreadSelectionInfoTable;
		fThreadSelectionInfoTable = NULL;
		throw std::bad_alloc();
	}

	BScrollView* sourceScrollView;

	const float splitSpacing = 3.0f;

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(fMenuBar = new BMenuBar("Menu"))
		.AddSplit(B_VERTICAL, splitSpacing)
			.GetSplitView(&fFunctionSplitView)
			.SetInsets(B_USE_SMALL_INSETS)
			.Add(fTabView = new BTabView("tab view"), 0.4f)
			.AddSplit(B_HORIZONTAL, splitSpacing)
				.GetSplitView(&fSourceSplitView)
				.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
					.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
						.Add(fRunButton = new BButton("Run"))
						.Add(fStepOverButton = new BButton("Step over"))
						.Add(fStepIntoButton = new BButton("Step into"))
						.Add(fStepOutButton = new BButton("Step out"))
						.AddGlue()
					.End()
					.Add(fSourcePathView = new BStringView(
						"source path",
						"Source path unavailable."), 4.0f)
					.Add(sourceScrollView = new BScrollView("source scroll",
						NULL, 0, true, true), splitSpacing)
				.End()
				.Add(fLocalsTabView = new BTabView("locals view"))
			.End()
			.AddSplit(B_VERTICAL, splitSpacing)
				.GetSplitView(&fConsoleSplitView)
				.SetInsets(0.0)
				.Add(fConsoleOutputView = ConsoleOutputView::Create())
			.End()
		.End()
		.Add(fStatusBarView = new BStringView("status", "Ready."));

	fStatusBarView->SetExplicitMinSize(BSize(50.0, B_SIZE_UNSET));
	fStatusBarView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// add source view
	sourceScrollView->SetTarget(fSourceView = SourceView::Create(fTeam, this));

	// add threads tab
	BSplitView* threadGroup = new BSplitView(B_HORIZONTAL, splitSpacing);
	threadGroup->SetName("Threads");
	fTabView->AddTab(threadGroup);
	BLayoutBuilder::Split<>(threadGroup)
		.GetSplitView(&fThreadSplitView)
		.Add(fThreadListView = ThreadListView::Create(fTeam, this))
		.Add(fStackTraceView = StackTraceView::Create(this));

	// add images tab
	BSplitView* imagesGroup = new BSplitView(B_HORIZONTAL, splitSpacing);
	imagesGroup->SetName("Images");
	fTabView->AddTab(imagesGroup);
	BLayoutBuilder::Split<>(imagesGroup)
		.GetSplitView(&fImageSplitView)
		.Add(fImageListView = ImageListView::Create(fTeam, this))
		.Add(fImageFunctionsView = ImageFunctionsView::Create(this));

	// add breakpoints tab
	BGroupView* breakpointsGroup = new BGroupView(B_HORIZONTAL,
		B_USE_SMALL_SPACING);
	breakpointsGroup->SetName("Breakpoints");
	fTabView->AddTab(breakpointsGroup);
	BLayoutBuilder::Group<>(breakpointsGroup)
//		.SetInsets(0.0f)
		.Add(fBreakpointsView = BreakpointsView::Create(fTeam, this));

	ValueNodeManager* manager = new ValueNodeManager;

	// add local variables tab
	BView* tab = fVariablesView = VariablesView::Create(this, manager);
	fLocalsTabView->AddTab(tab);

	// add registers tab
	tab = fRegistersView = RegistersView::Create(fTeam->GetArchitecture());
	fLocalsTabView->AddTab(tab);

	fRunButton->SetMessage(new BMessage(MSG_THREAD_RUN));
	fStepOverButton->SetMessage(new BMessage(MSG_THREAD_STEP_OVER));
	fStepIntoButton->SetMessage(new BMessage(MSG_THREAD_STEP_INTO));
	fStepOutButton->SetMessage(new BMessage(MSG_THREAD_STEP_OUT));
	fRunButton->SetTarget(this);
	fStepOverButton->SetTarget(this);
	fStepIntoButton->SetTarget(this);
	fStepOutButton->SetTarget(this);

	fSourcePathView->SetExplicitMinSize(BSize(100.0, B_SIZE_UNSET));
	fSourcePathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BMessageFilter* filter = new(std::nothrow) PathViewMessageFilter(
		BMessenger(this));
	if (filter != NULL)
		fSourcePathView->AddFilter(filter);

	// add menus and menu items
	BMenu* menu = new BMenu("Debugger");
	fMenuBar->AddItem(menu);
	BMenuItem* item = new BMenuItem("Start new team" B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_START_TEAM_WINDOW));
	menu->AddItem(item);
	item->SetTarget(be_app);
	item = new BMenuItem("Show Teams window" B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_TEAMS_WINDOW));
	menu->AddItem(item);
	item->SetTarget(be_app);
	menu = new BMenu("Team");
	fMenuBar->AddItem(menu);
	item = new BMenuItem("Restart", new BMessage(
		MSG_TEAM_RESTART_REQUESTED), 'R', B_SHIFT_KEY);
	menu->AddItem(item);
	item->SetTarget(this);
	item = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED),
		'W');
	menu->AddItem(item);
	item->SetTarget(this);
	menu->AddSeparatorItem();
	item = new BMenuItem("Settings" B_UTF8_ELLIPSIS, new BMessage(
		MSG_SHOW_TEAM_SETTINGS_WINDOW));
	menu->AddItem(item);
	item->SetTarget(this);
	menu = new BMenu("Edit");
	fMenuBar->AddItem(menu);
	item = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	menu->AddItem(item);
	item->SetTarget(this);
	item = new BMenuItem("Select all", new BMessage(B_SELECT_ALL), 'A');
	menu->AddItem(item);
	item->SetTarget(this);
	menu = new BMenu("Tools");
	fMenuBar->AddItem(menu);
	item = new BMenuItem("Save debug report",
		new BMessage(MSG_CHOOSE_DEBUG_REPORT_LOCATION));
	menu->AddItem(item);
	item->SetTarget(this);
	item = new BMenuItem("Inspect memory",
		new BMessage(MSG_SHOW_INSPECTOR_WINDOW), 'I');
	menu->AddItem(item);
	item->SetTarget(this);
	item = new BMenuItem("Evaluate expression",
		new BMessage(MSG_SHOW_EXPRESSION_WINDOW), 'E');
	menu->AddItem(item);
	item->SetTarget(this);

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();
}


void
TeamWindow::_LoadSettings(const GuiTeamUiSettings* settings)
{
	BMessage teamWindowSettings;
	// no settings stored yet
	if (settings->Settings("teamWindow", teamWindowSettings) != B_OK)
		return;

	BRect frame;
	if (teamWindowSettings.FindRect("frame", &frame) == B_OK) {
		ResizeTo(frame.Width(), frame.Height());
		MoveTo(frame.left, frame.top);
	}

	BMessage archive;
	if (teamWindowSettings.FindMessage("sourceSplit", &archive) == B_OK)
		GuiSettingsUtils::UnarchiveSplitView(archive, fSourceSplitView);

	if (teamWindowSettings.FindMessage("functionSplit", &archive) == B_OK)
		GuiSettingsUtils::UnarchiveSplitView(archive, fFunctionSplitView);

	if (teamWindowSettings.FindMessage("imageSplit", &archive) == B_OK)
		GuiSettingsUtils::UnarchiveSplitView(archive, fImageSplitView);

	if (teamWindowSettings.FindMessage("threadSplit", &archive) == B_OK)
		GuiSettingsUtils::UnarchiveSplitView(archive, fThreadSplitView);

	if (teamWindowSettings.FindMessage("consoleSplit", &archive) == B_OK)
		GuiSettingsUtils::UnarchiveSplitView(archive, fConsoleSplitView);

	if (teamWindowSettings.FindMessage("imageListView", &archive) == B_OK)
		fImageListView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("imageFunctionsView", &archive) == B_OK)
		fImageFunctionsView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("threadListView", &archive) == B_OK)
		fThreadListView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("variablesView", &archive) == B_OK)
		fVariablesView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("registersView", &archive) == B_OK)
		fRegistersView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("stackTraceView", &archive) == B_OK)
		fStackTraceView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("breakpointsView", &archive) == B_OK)
		fBreakpointsView->LoadSettings(archive);

	if (teamWindowSettings.FindMessage("consoleOutputView", &archive) == B_OK)
		fConsoleOutputView->LoadSettings(archive);

	fUiSettings = *settings;
}


void
TeamWindow::_UpdateTitle()
{
	AutoLocker< ::Team> lock(fTeam);
	BString name = fTeam->Name();
	if (fTeam->ID() >= 0)
		name << " (" << fTeam->ID() << ")";
	SetTitle(name.String());
}


void
TeamWindow::_SetActiveThread(::Thread* thread)
{
	if (thread == fActiveThread)
		return;

	if (fActiveThread != NULL)
		fActiveThread->ReleaseReference();

	fActiveThread = thread;

	if (fActiveThread != NULL)
		fActiveThread->AcquireReference();

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	BReference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until we've set it

	locker.Unlock();

	fThreadListView->SetThread(fActiveThread);

	_SetActiveStackTrace(stackTrace);
	_UpdateCpuState();
}


void
TeamWindow::_SetActiveImage(Image* image)
{
	if (image == fActiveImage)
		return;

	if (fActiveImage != NULL)
		fActiveImage->ReleaseReference();

	fActiveImage = image;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = NULL;
	BReference<ImageDebugInfo> imageDebugInfoReference;

	if (fActiveImage != NULL) {
		fActiveImage->AcquireReference();

		imageDebugInfo = fActiveImage->GetImageDebugInfo();
		imageDebugInfoReference.SetTo(imageDebugInfo);

		// If the debug info is not loaded yet, request it.
		if (fActiveImage->ImageDebugInfoState() == IMAGE_DEBUG_INFO_NOT_LOADED)
			fListener->ImageDebugInfoRequested(fActiveImage);
	}

	locker.Unlock();

	fImageListView->SetImage(fActiveImage);
	fImageFunctionsView->SetImageDebugInfo(imageDebugInfo);
}


void
TeamWindow::_SetActiveStackTrace(StackTrace* stackTrace)
{
	delete fTraceUpdateRunner;
	fTraceUpdateRunner = NULL;

	if (stackTrace == fActiveStackTrace)
		return;

	if (fActiveStackTrace != NULL)
		fActiveStackTrace->ReleaseReference();

	fActiveStackTrace = stackTrace;

	if (fActiveStackTrace != NULL)
		fActiveStackTrace->AcquireReference();

	fStackTraceView->SetStackTrace(fActiveStackTrace);
	fSourceView->SetStackTrace(fActiveStackTrace, fActiveThread);

	StackFrame* frame = NULL;
	if (fActiveStackTrace != NULL) {
		ThreadStackFrameSelectionEntry* entry
			= fThreadSelectionInfoTable->Lookup(fActiveThread);
		if (entry != NULL)
			frame = entry->SelectedFrame();
		else
			frame = fActiveStackTrace->FrameAt(0);
	}

	_SetActiveStackFrame(frame);
}


void
TeamWindow::_SetActiveStackFrame(StackFrame* frame)
{
	if (frame == fActiveStackFrame)
		return;

	if (fActiveStackFrame != NULL) {
		AutoLocker< ::Team> locker(fTeam);
		fActiveStackFrame->RemoveListener(this);
		locker.Unlock();

		fActiveStackFrame->ReleaseReference();
	}

	fActiveStackFrame = frame;

	if (fActiveStackFrame != NULL) {
		fActiveStackFrame->AcquireReference();

		AutoLocker< ::Team> locker(fTeam);
		fActiveStackFrame->AddListener(this);
		locker.Unlock();

		fActiveSourceObject = ACTIVE_SOURCE_STACK_FRAME;

		ThreadStackFrameSelectionEntry* entry
			= fThreadSelectionInfoTable->Lookup(fActiveThread);
		if (entry == NULL) {
			entry = new(std::nothrow) ThreadStackFrameSelectionEntry(
				fActiveThread, fActiveStackFrame);
			if (entry == NULL)
				return;

			ObjectDeleter<ThreadStackFrameSelectionEntry> entryDeleter(entry);
			if (fThreadSelectionInfoTable->Insert(entry) == B_OK)
				entryDeleter.Detach();
		} else
			entry->SetSelectedFrame(fActiveStackFrame);

		_SetActiveFunction(fActiveStackFrame->Function(), false);
	}

	_UpdateCpuState();

	fStackTraceView->SetStackFrame(fActiveStackFrame);
	if (fActiveStackFrame != NULL)
		fVariablesView->SetStackFrame(fActiveThread, fActiveStackFrame);
	else
		fVariablesView->SetStackFrame(NULL, NULL);
	fSourceView->SetStackFrame(fActiveStackFrame);
}


void
TeamWindow::_SetActiveBreakpoint(UserBreakpoint* breakpoint)
{
	if (breakpoint == fActiveBreakpoint)
		return;

	if (fActiveBreakpoint != NULL)
		fActiveBreakpoint->ReleaseReference();

	fActiveBreakpoint = breakpoint;

	if (fActiveBreakpoint != NULL) {
		fActiveBreakpoint->AcquireReference();

		// get the breakpoint's function (more exactly: some function instance)
		AutoLocker< ::Team> locker(fTeam);

		Function* function = fTeam->FunctionByID(
			breakpoint->Location().GetFunctionID());
		FunctionInstance* functionInstance = function != NULL
			? function->FirstInstance() : NULL;
		BReference<FunctionInstance> functionInstanceReference(
			functionInstance);

		locker.Unlock();

		fActiveSourceObject = ACTIVE_SOURCE_BREAKPOINT;

		_SetActiveFunction(functionInstance);

		// scroll to the breakpoint's source code line number (it is not done
		// automatically, if the active function remains the same)
		_ScrollToActiveFunction();
	}
}


void
TeamWindow::_SetActiveFunction(FunctionInstance* functionInstance,
	bool searchForFrame)
{
	if (functionInstance == fActiveFunction)
		return;

	AutoLocker< ::Team> locker(fTeam);

	if (fActiveFunction != NULL) {
		fActiveFunction->GetFunction()->RemoveListener(this);
		fActiveFunction->ReleaseReference();
	}

	// to avoid listener feedback problems, first unset the active function and
	// set the new image, if any
	locker.Unlock();

	fActiveFunction = NULL;

	if (functionInstance != NULL)
		_SetActiveImage(fTeam->ImageByAddress(functionInstance->Address()));

	fActiveFunction = functionInstance;

	locker.Lock();

	SourceCode* sourceCode = NULL;
	BReference<SourceCode> sourceCodeReference;

	if (fActiveFunction != NULL) {
		fActiveFunction->AcquireReference();
		fActiveFunction->GetFunction()->AddListener(this);

		Function* function = fActiveFunction->GetFunction();
		sourceCode = function->GetSourceCode();
		if (sourceCode == NULL)
			sourceCode = fActiveFunction->GetSourceCode();
		sourceCodeReference.SetTo(sourceCode);

		// If the source code is not loaded yet, request it.
		if (function->SourceCodeState() == FUNCTION_SOURCE_NOT_LOADED)
			fListener->FunctionSourceCodeRequested(fActiveFunction);
	}

	locker.Unlock();

	_SetActiveSourceCode(sourceCode);

	fImageFunctionsView->SetFunction(fActiveFunction);

	locker.Lock();

	if (!searchForFrame || fActiveStackTrace == NULL)
		return;

	// look if our current stack trace has a frame matching the selected
	// function. If so, set it to match.
	StackFrame* matchingFrame = NULL;
	BReference<StackFrame> frameRef;

	for (int32 i = 0; i < fActiveStackTrace->CountFrames(); i++) {
		StackFrame* frame = fActiveStackTrace->FrameAt(i);
		if (frame->Function() == fActiveFunction) {
			matchingFrame = frame;
			frameRef.SetTo(frame);
			break;
		}
	}

	locker.Unlock();

	if (matchingFrame != NULL)
		_SetActiveStackFrame(matchingFrame);
}


void
TeamWindow::_SetActiveSourceCode(SourceCode* sourceCode)
{
	if (sourceCode == fActiveSourceCode) {
		_ScrollToActiveFunction();
		return;
	}

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->ReleaseReference();

	fActiveSourceCode = sourceCode;

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->AcquireReference();

	fSourceView->SetSourceCode(fActiveSourceCode);

	_UpdateSourcePathState();
	_ScrollToActiveFunction();
}

void
TeamWindow::_UpdateCpuState()
{
	// get the CPU state
	CpuState* cpuState = NULL;
	BReference<CpuState> cpuStateReference;
		// hold a reference until the register view has one

	if (fActiveThread != NULL) {
		// Get the CPU state from the active stack frame or the thread directly.
		if (fActiveStackFrame == NULL) {
			AutoLocker< ::Team> locker(fTeam);
			cpuState = fActiveThread->GetCpuState();
			cpuStateReference.SetTo(cpuState);
			locker.Unlock();
		} else
			cpuState = fActiveStackFrame->GetCpuState();
	}

	fRegistersView->SetCpuState(cpuState);
}


void
TeamWindow::_UpdateRunButtons()
{
	uint32 threadState = fActiveThread != NULL
		? fActiveThread->State() : THREAD_STATE_UNKNOWN;

	switch (threadState) {
		case THREAD_STATE_UNKNOWN:
			fRunButton->SetEnabled(false);
			fStepOverButton->SetEnabled(false);
			fStepIntoButton->SetEnabled(false);
			fStepOutButton->SetEnabled(false);
			break;
		case THREAD_STATE_RUNNING:
			if (fTraceUpdateRunner == NULL) {
				fRunButton->SetLabel("Debug");
				fRunButton->SetMessage(new BMessage(MSG_THREAD_STOP));
				fRunButton->SetEnabled(true);
				fStepOverButton->SetEnabled(false);
				fStepIntoButton->SetEnabled(false);
				fStepOutButton->SetEnabled(false);
			}
			break;
		case THREAD_STATE_STOPPED:
			fRunButton->SetLabel("Run");
			fRunButton->SetMessage(new BMessage(MSG_THREAD_RUN));
			fRunButton->SetEnabled(true);
			fStepOverButton->SetEnabled(true);
			fStepIntoButton->SetEnabled(true);
			fStepOutButton->SetEnabled(true);
			break;
	}
}


void
TeamWindow::_UpdateSourcePathState()
{
	LocatableFile* sourceFile = NULL;
	BString sourceText = "Source file unavailable.";
	BString truncatedText;

	if (fActiveSourceCode != NULL) {
		sourceFile = fActiveFunction->GetFunctionDebugInfo()->SourceFile();

		if (sourceFile != NULL && !sourceFile->GetLocatedPath(sourceText))
			sourceFile->GetPath(sourceText);

		function_source_state state = fActiveFunction->GetFunction()
			->SourceCodeState();
		if (state == FUNCTION_SOURCE_SUPPRESSED)
			sourceText.Prepend("Disassembly for: ");
		else if (state != FUNCTION_SOURCE_NOT_LOADED
			&& fActiveSourceCode->GetSourceFile() == NULL
			&& sourceFile != NULL) {
			sourceText.Prepend("Click to locate source file '");
			sourceText += "'";
			truncatedText = sourceText;
			fSourcePathView->TruncateString(&truncatedText, B_TRUNCATE_MIDDLE,
				fSourcePathView->Bounds().Width());
		} else if (sourceFile != NULL)
			sourceText.Prepend("File: ");
	}

	if (!truncatedText.IsEmpty() && truncatedText != sourceText) {
		fSourcePathView->SetToolTip(sourceText);
		fSourcePathView->SetText(truncatedText);
	} else {
		fSourcePathView->SetText(sourceText);
		fSourcePathView->SetToolTip((const char*)NULL);
	}
}


void
TeamWindow::_ScrollToActiveFunction()
{
	// Scroll to the active function, if it has been selected manually.
	if (fActiveFunction == NULL || fActiveSourceCode == NULL)
		return;

	switch (fActiveSourceObject) {
		case ACTIVE_SOURCE_FUNCTION:
			fSourceView->ScrollToAddress(fActiveFunction->Address());
			break;
		case ACTIVE_SOURCE_BREAKPOINT:
		{
			if (fActiveBreakpoint == NULL)
				break;

			const UserBreakpointLocation& location
				= fActiveBreakpoint->Location();
			int32 line = location.GetSourceLocation().Line();

			if (location.SourceFile() != NULL && line >= 0
				&& fActiveSourceCode->GetSourceFile()
					== location.SourceFile()) {
				fSourceView->ScrollToLine(line);
			} else {
				fSourceView->ScrollToAddress(
					fActiveFunction->Address()
						+ location.RelativeAddress());
			}
			break;
		}
		case ACTIVE_SOURCE_NONE:
		case ACTIVE_SOURCE_STACK_FRAME:
			break;
	}
}


void
TeamWindow::_HandleThreadStateChanged(thread_id threadID)
{
	AutoLocker< ::Team> locker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	if (thread->State() != THREAD_STATE_STOPPED) {
		ThreadStackFrameSelectionEntry* entry
			= fThreadSelectionInfoTable->Lookup(thread);
		if (entry != NULL) {
			fThreadSelectionInfoTable->Remove(entry);
			delete entry;
		}
	}

	// If the thread has been stopped and we don't have an active thread yet
	// (or it isn't stopped), switch to this thread. Otherwise ignore the event.
	if (thread->State() == THREAD_STATE_STOPPED
		&& (fActiveThread == NULL
			|| (thread != fActiveThread
				&& fActiveThread->State() != THREAD_STATE_STOPPED))) {
		_SetActiveThread(thread);
	} else if (thread != fActiveThread) {
		// otherwise ignore the event, if the thread is not the active one
		return;
	}

	// Switch to the threads tab view when the thread has stopped.
	if (thread->State() == THREAD_STATE_STOPPED) {
		fTabView->Select(MAIN_TAB_INDEX_THREADS);

		// if we hit a breakpoint or exception, raise the window to the
		// foreground, since if this occurs while e.g. debugging a GUI
		// app, it might not be immediately obvious that such an event
		// occurred as the app may simply appear to hang.
		Activate();
	}

	_UpdateRunButtons();
}


void
TeamWindow::_HandleCpuStateChanged(thread_id threadID)
{
	// We're only interested in the currently selected thread
	if (fActiveThread == NULL || threadID != fActiveThread->ID())
		return;

	_UpdateCpuState();
}


void
TeamWindow::_HandleStackTraceChanged(thread_id threadID)
{
	// We're only interested in the currently selected thread
	if (fActiveThread == NULL || threadID != fActiveThread->ID())
		return;

	AutoLocker< ::Team> locker(fTeam);

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	BReference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until the register view has one

	locker.Unlock();

	if (stackTrace == NULL) {
		if (fTraceUpdateRunner != NULL)
			return;

		BMessage message(MSG_CLEAR_STACK_TRACE);
		fTraceUpdateRunner = new(std::nothrow) BMessageRunner(this,
			message, 250000, 1);
		if (fTraceUpdateRunner != NULL
			&& fTraceUpdateRunner->InitCheck() == B_OK) {
			fStackTraceView->SetStackTraceClearPending();
			fVariablesView->SetStackFrameClearPending();
			return;
		}
	}

	_SetActiveStackTrace(stackTrace);
}


void
TeamWindow::_HandleImageDebugInfoChanged(image_id imageID)
{
	TRACE_GUI("TeamWindow::_HandleImageDebugInfoChanged(%" B_PRId32 ")\n",
		imageID);

	// We're only interested in the currently selected thread
	if (fActiveImage == NULL || imageID != fActiveImage->ID())
		return;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = fActiveImage != NULL
		? fActiveImage->GetImageDebugInfo() : NULL;

	TRACE_GUI("  image debug info: %p\n", imageDebugInfo);

	BReference<ImageDebugInfo> imageDebugInfoReference(imageDebugInfo);
		// hold a reference until we've set it

	locker.Unlock();

	fImageFunctionsView->SetImageDebugInfo(imageDebugInfo);
}


void
TeamWindow::_HandleSourceCodeChanged()
{
	// If we don't have an active function anymore, the message is obsolete.
	if (fActiveFunction == NULL)
		return;

	// get a reference to the source code
	AutoLocker< ::Team> locker(fTeam);

	SourceCode* sourceCode = NULL;
	if (fActiveFunction->GetFunction()->SourceCodeState()
		== FUNCTION_SOURCE_LOADED) {
		sourceCode = fActiveFunction->GetFunction()->GetSourceCode();
	} else
		sourceCode = fActiveFunction->GetSourceCode();

	BReference<SourceCode> sourceCodeReference(sourceCode);

	locker.Unlock();

	_SetActiveSourceCode(sourceCode);
}


void
TeamWindow::_HandleUserBreakpointChanged(UserBreakpoint* breakpoint)
{
	fSourceView->UserBreakpointChanged(breakpoint);
	fBreakpointsView->UserBreakpointChanged(breakpoint);
}


void
TeamWindow::_HandleWatchpointChanged(Watchpoint* watchpoint)
{
	fBreakpointsView->WatchpointChanged(watchpoint);
}


status_t
TeamWindow::_RetrieveMatchingSourceWorker(void* arg)
{
	TeamWindow* window = (TeamWindow*)arg;

	BStringList* entries = new(std::nothrow) BStringList();
	if (entries == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BStringList> stringListDeleter(entries);

	if (!window->Lock())
		return B_BAD_VALUE;

	BString path;
	window->fActiveFunction->GetFunctionDebugInfo()->SourceFile()
		->GetPath(path);
	window->Unlock();

	status_t error = window->_RetrieveMatchingSourceEntries(path, entries);

	entries->Sort();
	BMessenger messenger(window);
	if (messenger.IsValid() && messenger.LockTarget()) {
		if (window->fActiveSourceWorker == find_thread(NULL)) {
			BMessage message(MSG_SOURCE_ENTRY_QUERY_COMPLETE);
			message.AddInt32("error", error);
			message.AddPointer("entries", entries);
			if (messenger.SendMessage(&message) == B_OK)
				stringListDeleter.Detach();
		}
		window->Unlock();
	}

	return B_OK;
}


void
TeamWindow::_HandleResolveMissingSourceFile(entry_ref& locatedPath)
{
	if (fActiveFunction != NULL) {
		LocatableFile* sourceFile = fActiveFunction->GetFunctionDebugInfo()
			->SourceFile();
		if (sourceFile != NULL) {
			BString sourcePath;
			sourceFile->GetPath(sourcePath);
			BString sourceFileName(sourcePath);
			int32 index = sourcePath.FindLast('/');
			if (index >= 0)
				sourceFileName.Remove(0, index + 1);

			BPath targetFilePath(&locatedPath);
			if (targetFilePath.InitCheck() != B_OK)
				return;

			if (strcmp(sourceFileName.String(), targetFilePath.Leaf()) != 0) {
				BString message;
				message.SetToFormat("The names of source file '%s' and located"
					" file '%s' differ. Use file anyway?",
					sourceFileName.String(), targetFilePath.Leaf());
				BAlert* alert = new(std::nothrow) BAlert(
					"Source path mismatch", message.String(), "Cancel", "Use");
				if (alert == NULL)
					return;

				int32 choice = alert->Go();
				if (choice <= 0)
					return;
			}

			LocatableFile* foundSourceFile = fActiveSourceCode
				->GetSourceFile();
			if (foundSourceFile != NULL)
				fListener->SourceEntryInvalidateRequested(foundSourceFile);
			fListener->SourceEntryLocateRequested(sourcePath,
				targetFilePath.Path());
			fListener->FunctionSourceCodeRequested(fActiveFunction);
		}
	}
}


void
TeamWindow::_HandleLocateSourceRequest(BStringList* entries)
{
	if (fActiveFunction == NULL)
		return;
	else if (fActiveFunction->GetFunctionDebugInfo()->SourceFile() == NULL)
		return;
	else if (fActiveSourceCode == NULL)
		return;
	else if (fActiveFunction->GetFunction()->SourceCodeState()
		== FUNCTION_SOURCE_NOT_LOADED) {
		return;
	}

	if (entries == NULL) {
		if (fActiveSourceWorker < 0) {
			fActiveSourceWorker = spawn_thread(&_RetrieveMatchingSourceWorker,
				"source file query worker", B_NORMAL_PRIORITY, this);
			if (fActiveSourceWorker > 0)
				resume_thread(fActiveSourceWorker);
		}
		return;
	}

	int32 count = entries->CountStrings();
	if (count > 0) {
		BPopUpMenu* menu = new(std::nothrow) BPopUpMenu("");
		if (menu == NULL)
			return;

		BPrivate::ObjectDeleter<BPopUpMenu> menuDeleter(menu);
		BMenuItem* item = NULL;
		for (int32 i = 0; i < count; i++) {
			item = new(std::nothrow) BMenuItem(entries->StringAt(i).String(),
				NULL);
			if (item == NULL || !menu->AddItem(item)) {
				delete item;
				return;
			}
		}

		menu->AddSeparatorItem();
		BMenuItem* manualItem = new(std::nothrow) BMenuItem(
			"Locate manually" B_UTF8_ELLIPSIS, NULL);
		if (manualItem == NULL || !menu->AddItem(manualItem)) {
			delete manualItem;
			return;
		}

		BPoint point;
		fSourcePathView->GetMouse(&point, NULL, false);
		fSourcePathView->ConvertToScreen(&point);
		item = menu->Go(point, false, true);
		if (item == NULL)
			return;
		else if (item != manualItem) {
			// if the user picks to locate the entry manually,
			// then fall through to the usual file panel logic
			// as if we'd found no matches at all.
			entry_ref ref;
			if (get_ref_for_path(item->Label(), &ref) == B_OK) {
				_HandleResolveMissingSourceFile(ref);
				return;
			}
		}
	}

	try {
		if (fFilePanel == NULL) {
			fFilePanel = new BFilePanel(B_OPEN_PANEL,
				new BMessenger(this));
		}
		fFilePanel->Show();
	} catch (...) {
		delete fFilePanel;
		fFilePanel = NULL;
	}
}


status_t
TeamWindow::_RetrieveMatchingSourceEntries(const BString& path,
	BStringList* _entries)
{
	BPath filePath(path);
	status_t error = filePath.InitCheck();
	if (error != B_OK)
		return error;

	_entries->MakeEmpty();

	BQuery query;
	BString predicate;
	query.PushAttr("name");
	query.PushString(filePath.Leaf());
	query.PushOp(B_EQ);

	error = query.GetPredicate(&predicate);
	if (error != B_OK)
		return error;

	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (!volume.KnowsQuery())
			continue;

		if (query.SetVolume(&volume) != B_OK)
			continue;

		error = query.SetPredicate(predicate.String());
		if (error != B_OK)
			continue;

		if (query.Fetch() != B_OK)
			continue;

		entry_ref ref;
		while (query.GetNextRef(&ref) == B_OK) {
			filePath.SetTo(&ref);
			_entries->Add(filePath.Path());
		}

		query.Clear();
	}

	return B_OK;
}


status_t
TeamWindow::_SaveInspectorSettings(const BMessage* settings)
{
	if (fUiSettings.AddSettings("inspectorWindow", *settings) != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
TeamWindow::_GetActiveSourceLanguage(SourceLanguage*& _language)
{
	AutoLocker< ::Team> locker(fTeam);

	if (!locker.IsLocked())
		return B_ERROR;

	if (fActiveSourceCode != NULL) {
		_language = fActiveSourceCode->GetSourceLanguage();
		_language->AcquireReference();
		return B_OK;
	}

	// if we made it this far, we were unable to acquire a source
	// language corresponding to the active function. As such,
	// try to fall back to the C++-style parser.
	_language = new(std::nothrow) CppLanguage();
	if (_language == NULL)
		return B_NO_MEMORY;

	return B_OK;
}
