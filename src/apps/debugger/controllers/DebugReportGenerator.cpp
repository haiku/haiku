/*
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DebugReportGenerator.h"

#include <cpu_type.h>

#include <AutoLocker.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <StringForSize.h>

#include "Architecture.h"
#include "AreaInfo.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Image.h"
#include "MessageCodes.h"
#include "Register.h"
#include "SemaphoreInfo.h"
#include "StackFrame.h"
#include "StackTrace.h"
#include "StringUtils.h"
#include "SystemInfo.h"
#include "Team.h"
#include "Thread.h"
#include "Type.h"
#include "UiUtils.h"
#include "UserInterface.h"
#include "Value.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"


DebugReportGenerator::DebugReportGenerator(::Team* team,
	UserInterfaceListener* listener, DebuggerInterface* interface)
	:
	BLooper("DebugReportGenerator"),
	fTeam(team),
	fArchitecture(team->GetArchitecture()),
	fDebuggerInterface(interface),
	fTeamDataSem(-1),
	fNodeManager(NULL),
	fListener(listener),
	fWaitingNode(NULL),
	fCurrentBlock(NULL),
	fBlockRetrievalStatus(B_OK),
	fTraceWaitingThread(NULL)
{
	fTeam->AddListener(this);
	fArchitecture->AcquireReference();
}


DebugReportGenerator::~DebugReportGenerator()
{
	fTeam->RemoveListener(this);
	fArchitecture->ReleaseReference();
	if (fNodeManager != NULL) {
		fNodeManager->RemoveListener(this);
		fNodeManager->ReleaseReference();
	}

	if (fCurrentBlock != NULL)
		fCurrentBlock->ReleaseReference();
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

	fNodeManager->AddListener(this);

	Run();

	return B_OK;
}


DebugReportGenerator*
DebugReportGenerator::Create(::Team* team, UserInterfaceListener* listener,
	DebuggerInterface* interface)
{
	DebugReportGenerator* self = new DebugReportGenerator(team, listener,
		interface);

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

	result = _DumpRunningThreads(output);
	if (result != B_OK)
		return result;

	result = _DumpLoadedImages(output);
	if (result != B_OK)
		return result;

	result = _DumpAreas(output);
	if (result != B_OK)
		return result;

	result = _DumpSemaphores(output);
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
	if (fTraceWaitingThread == event.GetThread()) {
		fTraceWaitingThread = NULL;
		release_sem(fTeamDataSem);
	}
}


void
DebugReportGenerator::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	_HandleMemoryBlockRetrieved(block, B_OK);
}


void
DebugReportGenerator::MemoryBlockRetrievalFailed(TeamMemoryBlock* block,
	status_t result)
{
	_HandleMemoryBlockRetrieved(block, result);
}


void
DebugReportGenerator::ValueNodeValueChanged(ValueNode* node)
{
	if (node == fWaitingNode) {
		fWaitingNode = NULL;
		release_sem(fTeamDataSem);
	}
}


status_t
DebugReportGenerator::_GenerateReportHeader(BString& _output)
{
	AutoLocker< ::Team> locker(fTeam);

	BString data;
	data.SetToFormat("Debug information for team %s (%" B_PRId32 "):\n",
		fTeam->Name(), fTeam->ID());
	_output << data;

	SystemInfo sysInfo;

	if (fDebuggerInterface->GetSystemInfo(sysInfo) == B_OK) {
		const system_info &info = sysInfo.GetSystemInfo();
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

		const utsname& name = sysInfo.GetSystemName();
		data.SetToFormat("Haiku revision: %s (%s)\n", name.version,
			name.machine);
		_output << data;
	}

	return B_OK;
}


status_t
DebugReportGenerator::_DumpLoadedImages(BString& _output)
{
	AutoLocker< ::Team> locker(fTeam);

	_output << "\nLoaded Images:\n";
	BObjectList<Image> images;
	BString data;
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		 Image* image = it.Next();) {
		 images.AddItem(image);
	}

	images.SortItems(&_CompareImages);

	Image* image = NULL;
	data.SetToFormat("\tID\t\tText Base\tText End\tData Base\tData"
		" End\tType\tName\n\t");
	_output << data;
	_output.Append('-', 80);
	_output.Append("\n");
	for (int32 i = 0; (image = images.ItemAt(i)) != NULL; i++) {
		const ImageInfo& info = image->Info();
		char buffer[32];
		try {
			target_addr_t textBase = info.TextBase();
			target_addr_t dataBase = info.DataBase();

			data.SetToFormat("\t%" B_PRId32 "\t0x%08" B_PRIx64 "\t"
				"0x%08" B_PRIx64 "\t0x%08" B_PRIx64 "\t0x%08" B_PRIx64 "\t"
				"%-7s\t%s\n", info.ImageID(), textBase, textBase + info.TextSize(),
				dataBase, dataBase + info.DataSize(),
				UiUtils::ImageTypeToString(info.Type(), buffer,
					sizeof(buffer)), info.Name().String());

			_output << data;
		} catch (...) {
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DebugReportGenerator::_DumpAreas(BString& _output)
{
	BObjectList<AreaInfo> areas(20, true);
	status_t result = fDebuggerInterface->GetAreaInfos(areas);
	if (result != B_OK)
		return result;

	areas.SortItems(&_CompareAreas);

	_output << "\nAreas:\n";
	BString data;
	data.SetToFormat("\tID\t\tBase\t\tEnd\t\t\tSize (KiB)\tProtection\tLocking\t\t\tName\n\t");
	_output << data;
	_output.Append('-', 80);
	_output.Append("\n");
	AreaInfo* info;
	BString protectionBuffer;
	char lockingBuffer[32];
	for (int32 i = 0; (info = areas.ItemAt(i)) != NULL; i++) {
		try {
			data.SetToFormat("\t%" B_PRId32 "\t0x%08" B_PRIx64 "\t"
				"0x%08" B_PRIx64 "\t%10" B_PRId64 "\t%-11s\t%-14s\t%s\n",
				info->AreaID(), info->BaseAddress(), info->BaseAddress()
					+ info->Size(), info->Size() / 1024,
				UiUtils::AreaProtectionFlagsToString(info->Protection(),
					protectionBuffer).String(),
				UiUtils::AreaLockingFlagsToString(info->Lock(), lockingBuffer,
					sizeof(lockingBuffer)), info->Name().String());

			_output << data;
		} catch (...) {
			return B_NO_MEMORY;
		}
	}

	_output << "\nProtection Flags: r - read, w - write, x - execute, "
		"s - stack, o - overcommit, c - cloneable, S - shared, k - kernel\n";

	return B_OK;
}


status_t
DebugReportGenerator::_DumpSemaphores(BString& _output)
{
	BObjectList<SemaphoreInfo> semaphores(20, true);
	status_t result = fDebuggerInterface->GetSemaphoreInfos(semaphores);
	if (result != B_OK)
		return result;

	semaphores.SortItems(&_CompareSemaphores);

	_output << "\nSemaphores:\n";
	BString data;
	data.SetToFormat("\tID\t\tCount\tLast Holder\tName\n\t");
	_output << data;
	_output.Append('-', 60);
	_output.Append("\n");
	SemaphoreInfo* info;
	for (int32 i = 0; (info = semaphores.ItemAt(i)) != NULL; i++) {
		try {
			data.SetToFormat("\t%" B_PRId32 "\t%5" B_PRId32 "\t%11" B_PRId32
				"\t%s\n", info->SemID(), info->Count(),
				info->LatestHolder(), info->Name().String());

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
	BObjectList< ::Thread> threads;
	::Thread* thread;
	for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
		  (thread = it.Next());) {
		 threads.AddItem(thread);
	}

	threads.SortItems(&_CompareThreads);
	for (int32 i = 0; (thread = threads.ItemAt(i)) != NULL; i++) {
		try {
			data.SetToFormat("\tthread %" B_PRId32 ": %s %s", thread->ID(),
					thread->Name(), thread->IsMainThread()
						? "(main)" : "");
			_output << data << "\n";

			if (thread->State() == THREAD_STATE_STOPPED) {
				data.SetToFormat("\t\tstate: %s",
					UiUtils::ThreadStateToString(thread->State(),
							thread->StoppedReason()));
				const BString& stoppedInfo = thread->StoppedReasonInfo();
				if (stoppedInfo.Length() != 0)
					data << " (" << stoppedInfo << ")";
				_output << data << "\n\n";

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
		fTraceWaitingThread = thread;
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
			// only dump the topmost frame
			if (i == 0) {
				_DumpStackFrameMemory(_output, thread->GetCpuState(),
					frame->FrameAddress(), thread->GetTeam()->GetArchitecture()
						->StackGrowthDirection());
			}
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
			containerLocker.Unlock();
			_ResolveValueIfNeeded(child->Node(), frame, 1);
			containerLocker.Lock();
			UiUtils::PrintValueNodeGraph(_output, child, 3, 1);
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


void
DebugReportGenerator::_DumpStackFrameMemory(BString& _output,
	CpuState* state, target_addr_t framePointer, uint8 stackDirection)
{
	target_addr_t startAddress;
	target_addr_t endAddress;
	if (stackDirection == STACK_GROWTH_DIRECTION_POSITIVE) {
		startAddress = framePointer;
		endAddress = state->StackPointer();
	} else {
		startAddress = state->StackPointer();
		endAddress = framePointer;
	}

	if (fCurrentBlock == NULL || !fCurrentBlock->Contains(startAddress)) {
		fListener->InspectRequested(startAddress, this);
		status_t result = B_OK;
		do {
			result = acquire_sem(fTeamDataSem);
		} while (result == B_INTERRUPTED);
	}

	_output << "\t\t\tFrame memory:\n";
	if (fBlockRetrievalStatus == B_OK) {
		UiUtils::DumpMemory(_output, 3, fCurrentBlock, startAddress, 1, 16,
			endAddress - startAddress);
	} else {
		BString data;
		data.SetToFormat("\t\t\tUnavailable (%s)\n", strerror(
				fBlockRetrievalStatus));
		_output += data;
	}

}


status_t
DebugReportGenerator::_ResolveValueIfNeeded(ValueNode* node, StackFrame* frame,
	int32 maxDepth)
{
	status_t result = B_OK;
	if (node->LocationAndValueResolutionState() == VALUE_NODE_UNRESOLVED) {
		fWaitingNode = node;
		fListener->ValueNodeValueRequested(frame->GetCpuState(),
			fNodeManager->GetContainer(), node);
		do {
			result = acquire_sem(fTeamDataSem);
		} while (result == B_INTERRUPTED);
	}

	if (node->LocationAndValueResolutionState() == B_OK && maxDepth > 0) {
		AutoLocker<ValueNodeContainer> containerLocker(
			fNodeManager->GetContainer());
		for (int32 i = 0; i < node->CountChildren(); i++) {
			ValueNodeChild* child = node->ChildAt(i);
			containerLocker.Unlock();
			result = fNodeManager->AddChildNodes(child);
			if (result != B_OK)
				continue;

			// since in the case of a pointer to a compound we hide
			// the intervening compound, don't consider the hidden node
			// a level for the purposes of depth traversal
			if (node->GetType()->Kind() == TYPE_ADDRESS
				&& child->GetType()->Kind() == TYPE_COMPOUND) {
				_ResolveValueIfNeeded(child->Node(), frame, maxDepth);
			} else
				_ResolveValueIfNeeded(child->Node(), frame, maxDepth - 1);
			containerLocker.Lock();
		}
	}

	return result;
}


void
DebugReportGenerator::_HandleMemoryBlockRetrieved(TeamMemoryBlock* block,
	status_t result)
{
	if (fCurrentBlock != NULL) {
		fCurrentBlock->ReleaseReference();
		fCurrentBlock = NULL;
	}

	fBlockRetrievalStatus = result;

	fCurrentBlock = block;
	release_sem(fTeamDataSem);
}


/*static*/ int
DebugReportGenerator::_CompareAreas(const AreaInfo* a, const AreaInfo* b)
{
	if (a->BaseAddress() < b->BaseAddress())
		return -1;

	return 1;
}


/*static*/ int
DebugReportGenerator::_CompareImages(const Image* a, const Image* b)
{
	if (a->Info().TextBase() < b->Info().TextBase())
		return -1;

	return 1;
}


/*static*/ int
DebugReportGenerator::_CompareSemaphores(const SemaphoreInfo* a,
	const SemaphoreInfo* b)
{
	if (a->SemID() < b->SemID())
		return -1;

	return 1;
}


/*static*/ int
DebugReportGenerator::_CompareThreads(const ::Thread* a,
	const ::Thread* b)
{
	// sort stopped threads last, otherwise sort by thread ID
	if (a->State() == b->State())
		return a->ID() < b->ID() ? -1 : 1;

	if (a->State() == THREAD_STATE_STOPPED && b->State()
			!= THREAD_STATE_STOPPED) {
		return 1;
	}

	return -1;
}
