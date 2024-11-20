/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IOSchedulerRoster.h"

#include <util/AutoLock.h>


/*static*/ IOSchedulerRoster IOSchedulerRoster::sDefaultInstance;


IOSchedulerRoster::IOSchedulerRoster()
	:
	fNextID(1),
	fNotificationService("I/O")
{
	mutex_init(&fLock, "IOSchedulerRoster");
	fNotificationService.Register();
}


IOSchedulerRoster::~IOSchedulerRoster()
{
	mutex_destroy(&fLock);
	fNotificationService.Unregister();
}


void
IOSchedulerRoster::AddScheduler(IOScheduler* scheduler)
{
	AutoLocker<IOSchedulerRoster> locker(this);
	fSchedulers.Add(scheduler);
	locker.Unlock();

	Notify(IO_SCHEDULER_ADDED, scheduler);
}


void
IOSchedulerRoster::RemoveScheduler(IOScheduler* scheduler)
{
	AutoLocker<IOSchedulerRoster> locker(this);
	fSchedulers.Remove(scheduler);
	locker.Unlock();

	Notify(IO_SCHEDULER_REMOVED, scheduler);
}


void
IOSchedulerRoster::Notify(uint32 eventCode, const IOScheduler* scheduler,
	IORequest* request, IOOperation* operation)
{
	AutoLocker<DefaultNotificationService> locker(fNotificationService);

	if (!fNotificationService.HasListeners())
		return;

	KMessage event;
	event.SetTo(fEventBuffer, sizeof(fEventBuffer), IO_SCHEDULER_MONITOR);
	event.AddInt32("event", eventCode);
	event.AddPointer("scheduler", scheduler);
	if (request != NULL) {
		event.AddPointer("request", request);
		if (operation != NULL)
			event.AddPointer("operation", operation);
	}

	fNotificationService.NotifyLocked(event, eventCode);
}


int32
IOSchedulerRoster::NextID()
{
	AutoLocker<IOSchedulerRoster> locker(this);
	return fNextID++;
}


//	#pragma mark - debug methods and initialization


void
IOSchedulerRoster::Dump() const
{
	kprintf("IOSchedulerRoster at %p\n", this);
	kprintf("  mutex:   %p\n", &fLock);
	kprintf("  next ID: %" B_PRId32 "\n", fNextID);

	kprintf("  schedulers:");
	for (IOSchedulerList::ConstIterator it
				= fSchedulers.GetIterator();
			IOScheduler* scheduler = it.Next();) {
		kprintf(" %p", scheduler);
	}
	kprintf("\n");
}


static int
dump_io_scheduler_roster(int argc, char** argv)
{
	IOSchedulerRoster* roster;
	if (argc == 1) {
		roster = IOSchedulerRoster::Default();
	} else if (argc == 2) {
		roster = (IOSchedulerRoster*)parse_expression(argv[1]);
		if (roster == NULL)
			return -1;
	} else {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	roster->Dump();
	return 0;
}


static int
dump_io_scheduler(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	IOScheduler* scheduler = (IOScheduler*)parse_expression(argv[1]);
	scheduler->Dump();
	return 0;
}


static int
dump_io_request_owner(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	IORequestOwner* owner = (IORequestOwner*)parse_expression(argv[1]);
	owner->Dump();
	return 0;
}


static int
dump_io_request(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-request>\n", argv[0]);
		return 0;
	}

	IORequest* request = (IORequest*)parse_expression(argv[1]);
	request->Dump();
	return 0;
}


static int
dump_io_operation(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-operation>\n", argv[0]);
		return 0;
	}

	IOOperation* operation = (IOOperation*)parse_expression(argv[1]);
	operation->Dump();
	return 0;
}


static int
dump_io_buffer(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-buffer>\n", argv[0]);
		return 0;
	}

	IOBuffer* buffer = (IOBuffer*)parse_expression(argv[1]);
	buffer->Dump();
	return 0;
}


static int
dump_dma_buffer(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-dma-buffer>\n", argv[0]);
		return 0;
	}

	DMABuffer* buffer = (DMABuffer*)parse_expression(argv[1]);
	buffer->Dump();
	return 0;
}


/*static*/ void
IOSchedulerRoster::Init()
{
	new(&sDefaultInstance) IOSchedulerRoster;

	add_debugger_command_etc("io_scheduler_roster", &dump_io_scheduler_roster,
		"Dump an I/O scheduler roster",
		"<scheduler-roster>\n"
		"Dumps I/O scheduler roster at address <scheduler-roster>.\n"
		"If unspecified, dump the default roster.\n", 0);
	add_debugger_command_etc("io_scheduler", &dump_io_scheduler,
		"Dump an I/O scheduler",
		"<scheduler>\n"
		"Dumps I/O scheduler at address <scheduler>.\n", 0);
	add_debugger_command_etc("io_request_owner", &dump_io_request_owner,
		"Dump an I/O request owner",
		"<owner>\n"
		"Dumps I/O request owner at address <owner>.\n", 0);
	add_debugger_command("io_request", &dump_io_request, "dump an I/O request");
	add_debugger_command("io_operation", &dump_io_operation,
		"dump an I/O operation");
	add_debugger_command("io_buffer", &dump_io_buffer, "dump an I/O buffer");
	add_debugger_command("dma_buffer", &dump_dma_buffer, "dump a DMA buffer");
}
