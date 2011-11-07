/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <File.h>

#include <syscalls.h>
#include <system_profiler_defs.h>

#include <DebugEventStream.h>

#include "debug_utils.h"


#define SCHEDULING_RECORDING_AREA_SIZE	(4 * 1024 * 1024)

#define DEBUG_EVENT_MASK \
	(B_SYSTEM_PROFILER_TEAM_EVENTS | B_SYSTEM_PROFILER_THREAD_EVENTS	\
		| B_SYSTEM_PROFILER_SCHEDULING_EVENTS							\
		| B_SYSTEM_PROFILER_IO_SCHEDULING_EVENTS)


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s [ <options> ] <output file> [ <command line> ]\n"
	"Records thread scheduling information to a file for later analysis.\n"
	"If a command line <command line> is given, recording starts right before\n"
	"executing the command and steps when the respective team quits.\n"
	"\n"
	"Options:\n"
	"  -l           - When a command line is given: Start recording before\n"
	"                 executable has been loaded.\n"
	"  -h, --help   - Print this usage info.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


class Recorder {
public:
	Recorder()
		:
		fMainTeam(-1),
		fSkipLoading(true),
		fCaughtDeadlySignal(false)
	{
	}

	~Recorder()
	{
		fOutput.Flush();
	}


	status_t Init(const char* outputFile)
	{
		// open file
		status_t error = fOutputFile.SetTo(outputFile,
			B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n", outputFile,
				strerror(error));
			return error;
		}

		// create output stream
		error = fOutput.SetTo(&fOutputFile, 0, DEBUG_EVENT_MASK);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to initialize the output "
				"stream: %s\n", strerror(error));
			return error;
		}

		return B_OK;
	}

	void SetSkipLoading(bool skipLoading)
	{
		fSkipLoading = skipLoading;
	}

	void Run(const char* const* programArgs, int programArgCount)
	{
		// Load the executable, if we have to.
		thread_id threadID = -1;
		if (programArgCount >= 1) {
			threadID = load_program(programArgs, programArgCount,
				!fSkipLoading);
			if (threadID < 0) {
				fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
					programArgs[0], strerror(threadID));
				exit(1);
			}
			fMainTeam = threadID;
		}

		// install signal handlers so we can exit gracefully
		struct sigaction action;
		action.sa_handler = (__sighandler_t)_SignalHandler;
		action.sa_flags = 0;
		sigemptyset(&action.sa_mask);
		action.sa_userdata = this;
		if (sigaction(SIGHUP, &action, NULL) < 0
			|| sigaction(SIGINT, &action, NULL) < 0
			|| sigaction(SIGQUIT, &action, NULL) < 0) {
			fprintf(stderr, "%s: Failed to install signal handlers: %s\n",
				kCommandName, strerror(errno));
			exit(1);
		}

		// create an area for the sample buffer
		system_profiler_buffer_header* bufferHeader;
		area_id area = create_area("profiling buffer", (void**)&bufferHeader,
			B_ANY_ADDRESS, SCHEDULING_RECORDING_AREA_SIZE, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (area < 0) {
			fprintf(stderr, "%s: Failed to create sample area: %s\n",
				kCommandName, strerror(area));
			exit(1);
		}

		uint8* bufferBase = (uint8*)(bufferHeader + 1);
		size_t totalBufferSize = SCHEDULING_RECORDING_AREA_SIZE
			- (bufferBase - (uint8*)bufferHeader);

		// start profiling
		system_profiler_parameters profilerParameters;
		profilerParameters.buffer_area = area;
		profilerParameters.flags = DEBUG_EVENT_MASK;
		profilerParameters.locking_lookup_size = 64 * 1024;

		status_t error = _kern_system_profiler_start(&profilerParameters);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to start profiling: %s\n", kCommandName,
				strerror(error));
			exit(1);
		}

		// resume the loaded team, if we have one
		if (threadID >= 0)
			resume_thread(threadID);

		// main event loop
		while (true) {
			// get the current buffer
			size_t bufferStart = bufferHeader->start;
			size_t bufferSize = bufferHeader->size;
			uint8* buffer = bufferBase + bufferStart;
//printf("processing buffer of size %lu bytes\n", bufferSize);

			bool quit;
			if (bufferStart + bufferSize <= totalBufferSize) {
				quit = _ProcessEventBuffer(buffer, bufferSize);
			} else {
				size_t remainingSize = bufferStart + bufferSize
					- totalBufferSize;
				quit = _ProcessEventBuffer(buffer, bufferSize - remainingSize)
					|| _ProcessEventBuffer(bufferBase, remainingSize);
			}

			if (quit || fCaughtDeadlySignal)
				break;

			// get next buffer
			uint64 droppedEvents = 0;
			error = _kern_system_profiler_next_buffer(bufferSize,
				&droppedEvents);

			if (error != B_OK) {
				if (error == B_INTERRUPTED)
					continue;

				fprintf(stderr, "%s: Failed to get next sample buffer: %s\n",
					kCommandName, strerror(error));
				break;
			}

			if (droppedEvents > 0)
				fprintf(stderr, "%llu events dropped\n", droppedEvents);
		}

		// stop profiling
		_kern_system_profiler_stop();
	}

private:
	bool _ProcessEventBuffer(uint8* buffer, size_t bufferSize)
	{
//printf("_ProcessEventBuffer(%p, %lu)\n", buffer, bufferSize);
		const uint8* bufferStart = buffer;
		const uint8* bufferEnd = buffer + bufferSize;
		size_t usableBufferSize = bufferSize;
		bool quit = false;

		while (buffer < bufferEnd) {
			system_profiler_event_header* header
				= (system_profiler_event_header*)buffer;

			buffer += sizeof(system_profiler_event_header);

			if (header->event == B_SYSTEM_PROFILER_BUFFER_END) {
				// Marks the end of the ring buffer -- we need to ignore the
				// remaining bytes.
				usableBufferSize = (uint8*)header - bufferStart;
				break;
			}

			if (header->event == B_SYSTEM_PROFILER_TEAM_REMOVED) {
				system_profiler_team_removed* event
					= (system_profiler_team_removed*)buffer;

				// quit, if the main team we're interested in is gone
				if (fMainTeam >= 0 && event->team == fMainTeam) {
					usableBufferSize = buffer + header->size - bufferStart;
					quit = true;
					break;
				}
			}

			buffer += header->size;
		}

		// write buffer to file
		if (usableBufferSize > 0) {
			status_t error = fOutput.Write(bufferStart, usableBufferSize);
			if (error != B_OK) {
				fprintf(stderr, "%s: Failed to write buffer: %s\n",
					kCommandName, strerror(error));
				quit = true;
			}
		}

		return quit;
	}


	static void _SignalHandler(int signal, void* data)
	{
		Recorder* self = (Recorder*)data;
		self->fCaughtDeadlySignal = true;
	}

private:
	BFile					fOutputFile;
	BDebugEventOutputStream	fOutput;
	team_id					fMainTeam;
	bool					fSkipLoading;
	bool					fCaughtDeadlySignal;
};


int
main(int argc, const char* const* argv)
{
	Recorder recorder;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+hl", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;
			case 'l':
				recorder.SetSkipLoading(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// Remaining arguments should be the output file and the optional command
	// line.
	if (optind >= argc)
		print_usage_and_exit(true);

	const char* outputFile = argv[optind++];
	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	// prepare for battle
	if (recorder.Init(outputFile) != B_OK)
		exit(1);

	// start the action
	recorder.Run(programArgs, programArgCount);

	return 0;
}
