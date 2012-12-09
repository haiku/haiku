/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DebugReportGenerator.h"

#include <sys/utsname.h>

#include <cpu_type.h>

#include <AutoLocker.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <StringForSize.h>

#include "Architecture.h"
#include "CpuState.h"
#include "Image.h"
#include "MessageCodes.h"
#include "Register.h"
#include "StackFrame.h"
#include "StackTrace.h"
#include "StringUtils.h"
#include "Team.h"
#include "Thread.h"
#include "Type.h"
#include "UiUtils.h"
#include "Value.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"


DebugReportGenerator::DebugReportGenerator(::Team* team)
	:
	BLooper("DebugReportGenerator"),
	fTeam(team),
	fArchitecture(team->GetArchitecture()),
	fTeamDataSem(-1),
	fNodeManager(NULL)
{
	fTeam->AddListener(this);
	fArchitecture->AcquireReference();
}


DebugReportGenerator::~DebugReportGenerator()
{
	fTeam->RemoveListener(this);
	fArchitecture->ReleaseReference();
	if (fNodeManager != NULL)
		fNodeManager->ReleaseReference();
}


status_t
DebugReportGenerator::Init()
{
	fTeamDataSem = create_sem(0, "debug_controller_data_wait");
	if (fTeamDataSem < B_OK)
		return fTeamDataSem;

	fNodeManager = new(std::nothrow) ValueNodeManager();
	if (fNodeManager == NULL)
		return B_NO_MEMORY;

	Run();

	return B_OK;
}


DebugReportGenerator*
DebugReportGenerator::Create(::Team* team)
{
	DebugReportGenerator* self = new DebugReportGenerator(team);

	try {
		self->Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


status_t
DebugReportGenerator::_GenerateReport(const entry_ref& outputPath)
{
	BFile file(&outputPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	BString output;
	result = _GenerateReportHeader(output);
	if (result != B_OK)
		return result;

	result = _DumpLoadedImages(output);
	if (result != B_OK)
		return result;

	result = _DumpRunningThreads(output);
	if (result != B_OK)
		return result;

	result = file.Write(output.String(), output.Length());
	if (result < 0)
		return result;

	BPath path(&outputPath);

	fTeam->NotifyDebugReportChanged(path.Path());

	return B_OK;
}


void
DebugReportGenerator::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_GENERATE_DEBUG_REPORT:
		{
			entry_ref ref;
			if (message->FindRef("target", &ref) == B_OK)
				_GenerateReport(ref);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
DebugReportGenerator::ThreadStackTraceChanged(const ::Team::ThreadEvent& event)
{
	release_sem(fTeamDataSem);
}


status_t
DebugReportGenerator::_GenerateReportHeader(BString& _output)
{
	AutoLocker< ::Team> locker(fTeam);

	BString data;
	data.SetToFormat("Debug information for team %s (%" B_PRId32 "):\n",
		fTeam->Name(), fTeam->ID());
	_output << data;

	// TODO: this information should probably be requested via the debugger
	// interface, since e.g. in the case of a remote team, the report should
	// include data about the target, not the debugging host
	system_info info;
	if (get_system_info(&info) == B_OK) {
		data.SetToFormat("CPU(s): %" B_PRId32 "x %s %s\n",
			info.cpu_count, get_cpu_vendor_string(info.cpu_type),
			get_cpu_model_string(&info));
		_output << data;
		char maxSize[32];
		char usedSize[32];

		data.SetToFormat("Memory: %s total, %s used\n",
			BPrivate::string_for_size((int64)info.max_pages * B_PAGE_SIZE,
				maxSize, sizeof(maxSize)),
			BPrivate::string_for_size((int64)info.used_pages * B_PAGE_SIZE,
				usedSize, sizeof(usedSize)));
		_output << data;
	}
	utsname name;
	uname(&name);
	data.SetToFormat("Haiku revision: %s (%s)\n", name.version, name.machine);
	_output << data;

	return B_OK;
}


status_t
DebugReportGenerator::_DumpLoadedImages(BString& _output)
{
	AutoLocker< ::Team> locker(fTeam);

	_output << "\nLoaded Images:\n";
	BString data;
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		 Image* image = it.Next();) {
		const ImageInfo& info = image->Info();
		char buffer[32];
		try {
			target_addr_t textBase = info.TextBase();
			target_addr_t dataBase = info.DataBase();

			data.SetToFormat("\t%s (%" B_PRId32 ", %s) "
				"Text: %#08" B_PRIx64 " - %#08" B_PRIx64 ", Data: %#08"
				B_PRIx64 " - %#08" B_PRIx64 "\n", info.Name().String(),
				info.ImageID(), UiUtils::ImageTypeToString(info.Type(),
					buffer, sizeof(buffer)), textBase,
				textBase + info.TextSize(), dataBase,
				dataBase + info.DataSize());

			_output << data;
		} catch (...) {
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DebugReportGenerator::_DumpRunningThreads(BString& _output)
{
	AutoLocker< ::Team> locker(fTeam);

	_output << "\nActive Threads:\n";
	BString data;
	status_t result = B_OK;
	for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
		 ::Thread* thread = it.Next();) {
		try {
			data.SetToFormat("\t%s %s, id: %" B_PRId32", state: %s",
					thread->Name(), thread->IsMainThread()
						? "(main)" : "", thread->ID(),
					UiUtils::ThreadStateToString(thread->State(),
							thread->StoppedReason()));

			if (thread->State() == THREAD_STATE_STOPPED) {
				const BString& stoppedInfo = thread->StoppedReasonInfo();
				if (stoppedInfo.Length() != 0)
					data << " (" << stoppedInfo << ")";
			}

			_output << data << "\n";

			if (thread->State() == THREAD_STATE_STOPPED) {
				// we need to release our lock on the team here
				// since we might need to block and wait
				// on the stack trace.
				BReference< ::Thread> threadRef(thread);
				locker.Unlock();
				result = _DumpDebuggedThreadInfo(_output, thread);
				locker.Lock();
			}
			if (result != B_OK)
				return result;
		} catch (...) {
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DebugReportGenerator::_DumpDebuggedThreadInfo(BString& _output,
	::Thread* thread)
{
	AutoLocker< ::Team> locker;
	if (thread->State() != THREAD_STATE_STOPPED)
		return B_OK;

	StackTrace* trace = NULL;
	for (;;) {
		trace = thread->GetStackTrace();
		if (trace != NULL)
			break;

		locker.Unlock();
		status_t result = acquire_sem(fTeamDataSem);
		if (result != B_OK)
			return result;

		locker.Lock();
	}

	_output << "\t\tFrame\t\tIP\t\t\tFunction Name\n";
	_output << "\t\t-----------------------------------------------\n";
	BString data;
	for (int32 i = 0; StackFrame* frame = trace->FrameAt(i); i++) {
		char functionName[512];
		data.SetToFormat("\t\t%#08" B_PRIx64 "\t%#08" B_PRIx64 "\t%s\n",
			frame->FrameAddress(), frame->InstructionPointer(),
			UiUtils::FunctionNameForFrame(frame, functionName,
				sizeof(functionName)));

		_output << data;
		if (frame->CountParameters() == 0
			&& frame->CountLocalVariables() == 0) {
			continue;
		}

		_output << "\t\t\tVariables:\n";
		status_t result = fNodeManager->SetStackFrame(thread, frame);
		if (result != B_OK)
			continue;

		ValueNodeContainer* container = fNodeManager->GetContainer();
		AutoLocker<ValueNodeContainer> containerLocker(container);
		for (int32 i = 0; i < container->CountChildren(); i++) {
			ValueNodeChild* child = container->ChildAt(i);
			_DumpValueNode(_output, frame, child);
		}
		_output << "\n";
	}

	_output << "\n\t\tRegisters:\n";

	CpuState* state = thread->GetCpuState();
	BVariant value;
	const Register* reg = NULL;
	for (int32 i = 0; i < fArchitecture->CountRegisters(); i++) {
		reg = fArchitecture->Registers() + i;
		state->GetRegisterValue(reg, value);

		char buffer[64];
		data.SetToFormat("\t\t\t%5s:\t%s\n", reg->Name(),
			UiUtils::VariantToString(value, buffer, sizeof(buffer)));
		_output << data;
	}

	return B_OK;
}


status_t
DebugReportGenerator::_DumpValueNode(BString& _output, StackFrame* frame,
	ValueNodeChild* child, bool recurse)
{
	status_t result = _ResolveLocationIfNeeded(child, frame);
	if (result != B_OK)
		return result;

	_output << "\t\t\t";
	if (!recurse)
		_output << "\t";
	_output << child->Name() << ": ";

	ValueNode* node = child->Node();
	if (node->LocationAndValueResolutionState() == VALUE_NODE_UNRESOLVED) {
		if (_ResolveValueIfNeeded(node, frame) == B_OK) {
			Value* value = node->GetValue();
			if (value != NULL) {
				BString valueData;
				value->ToString(valueData);
				_output << valueData;
			} else
				_output << "Unavailable";
		} else
			_output << "Unknown";
	}
	if (recurse && node->CountChildren() != 0)
		_output << " {";

	_output << "\n";

	if (recurse) {
		if (node->CountChildren() == 0)
			return B_OK;

		if (node->CountChildren() == 1
			&& node->GetType()->Kind() == TYPE_ADDRESS
			&& node->ChildAt(0)->GetType()->Kind() == TYPE_COMPOUND) {
			// for the case of a pointer to a compound type,
			// we want to hide the intervening compound node and print
			// the children directly.
			status_t result = fNodeManager->AddChildNodes(node->ChildAt(0));
			if (result == B_OK) {
				result = _ResolveLocationIfNeeded(node->ChildAt(0), frame);
				if (result == B_OK) {
					node = node->ChildAt(0)->Node();
					// attempt to resolve the value here since the node's
					// representation may requires its value to be resolved
					// before its children can be created
					result = _ResolveValueIfNeeded(node, frame);
				}
			}
		}

		for (int32 i = 0; i < node->CountChildren(); i++) {
			if (fNodeManager->AddChildNodes(node->ChildAt(i)) != B_OK)
				continue;

			// don't dump compound nodes since we won't traverse into
			// them anyways and their top level node has no interesting
			// information.
			if (node->ChildAt(i)->GetType()->Kind() != TYPE_COMPOUND)
				_DumpValueNode(_output, frame, node->ChildAt(i), false);
		}
		_output << "\t\t\t}\n";
	}

	return B_OK;
}


status_t
DebugReportGenerator::_ResolveValueIfNeeded(ValueNode* node, StackFrame* frame)
{
	ValueLocation* location = NULL;
	Value* value = NULL;
	ValueLoader loader(fTeam->GetArchitecture(), fTeam->GetTeamMemory(),
		fTeam->GetTeamTypeInformation(), frame->GetCpuState());
	status_t result = node->ResolvedLocationAndValue(&loader, location,
		value);
	node->SetLocationAndValue(location, value, result);
	if (location != NULL)
		location->ReleaseReference();
	if (value != NULL)
		value->ReleaseReference();

	return result;
}


status_t
DebugReportGenerator::_ResolveLocationIfNeeded(ValueNodeChild* child,
	StackFrame* frame)
{
	ValueLocation* location = NULL;
	ValueLoader loader(fTeam->GetArchitecture(), fTeam->GetTeamMemory(),
		fTeam->GetTeamTypeInformation(), frame->GetCpuState());
	status_t result = child->ResolveLocation(&loader, location);
	child->SetLocation(location, result);
	if (location != NULL)
		location->ReleaseReference();
	return result;
}
