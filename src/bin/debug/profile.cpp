/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <debugger.h>
#include <OS.h>
#include <String.h>

#include <debug_support.h>
#include <ObjectList.h>

#include <util/DoublyLinkedList.h>

#include "debug_utils.h"


extern const char* __progname;
static const char* kCommandName = __progname;


class Image;
class Team;


enum {
	SAMPLE_AREA_SIZE	= 128 * 1024,
	SAMPLE_STACK_DEPTH	= 5
};


class Symbol {
public:
	Symbol(Image* image, addr_t base, size_t size, const char* name)
		:
		image(image),
		base(base),
		size(size),
		name(name)
	{
	}

	const char* Name() const	{ return name.String(); }

	Image*	image;
	addr_t	base;
	size_t	size;
	BString	name;
};


struct SymbolComparator {
	inline bool operator()(const Symbol* a, const Symbol* b) const
	{
		return a->base < b->base;
	}
};


struct HitSymbol {
	int64	hits;
	Symbol*	symbol;

	inline bool operator<(const HitSymbol& other) const
	{
		return hits > other.hits;
	}
};


class Image {
public:
	Image(const image_info& info)
		:
		fInfo(info),
		fSymbols(NULL),
		fSymbolCount(0)
	{
	}

	~Image()
	{
		if (fSymbols != NULL) {
			for (int32 i = 0; i < fSymbolCount; i++)
				delete fSymbols[i];
			delete[] fSymbols;
		}
	}

	const image_info& Info() const
	{
		return fInfo;
	}

	status_t LoadSymbols(debug_symbol_lookup_context* lookupContext)
	{
		printf("Loading symbols of image \"%s\" (%ld)...\n", fInfo.name,
			fInfo.id);

		// create symbol iterator
		debug_symbol_iterator* iterator;
		status_t error = debug_create_image_symbol_iterator(lookupContext,
			fInfo.id, &iterator);
		if (error != B_OK) {
			printf("Failed to init symbol iterator: %s\n", strerror(error));
			return error;
		}

		// iterate through the symbols
		BObjectList<Symbol>	symbols(512, true);
		char symbolName[1024];
		int32 symbolType;
		void* symbolLocation;
		size_t symbolSize;
		while (debug_next_image_symbol(iterator, symbolName, sizeof(symbolName),
				&symbolType, &symbolLocation, &symbolSize) == B_OK) {
//			printf("  %s %p (%6lu) %s\n",
//				symbolType == B_SYMBOL_TYPE_TEXT ? "text" : "data",
//				symbolLocation, symbolSize, symbolName);
			if (symbolSize > 0 && symbolType == B_SYMBOL_TYPE_TEXT) {
				Symbol* symbol = new(std::nothrow) Symbol(this,
					(addr_t)symbolLocation, symbolSize, symbolName);
				if (symbol == NULL || !symbols.AddItem(symbol)) {
					delete symbol;
					fprintf(stderr, "%s: Out of memory\n", kCommandName);
					debug_delete_image_symbol_iterator(iterator);
					return B_NO_MEMORY;
				}
			}
		}

		debug_delete_image_symbol_iterator(iterator);

		// sort the symbols
		fSymbolCount = symbols.CountItems();
		fSymbols = new(std::nothrow) Symbol*[fSymbolCount];
		if (fSymbols == NULL)
			return B_NO_MEMORY;

		for (int32 i = fSymbolCount - 1; i >= 0 ; i--)
			fSymbols[i] = symbols.RemoveItemAt(i);

		std::sort(fSymbols, fSymbols + fSymbolCount, SymbolComparator());

		return B_OK;
	}

	Symbol** Symbols() const
	{
		return fSymbols;
	}

	int32 SymbolCount() const
	{
		return fSymbolCount;
	}

	bool ContainsAddress(addr_t address) const
	{
		return address >= (addr_t)fInfo.text
			&& address < (addr_t)fInfo.data + fInfo.data_size;
	}

	int32 FindSymbol(addr_t address) const
	{
		// binary search the function
		int32 lower = 0;
		int32 upper = fSymbolCount;

		while (lower < upper) {
			int32 mid = (lower + upper) / 2;
			if (address >= fSymbols[mid]->base + fSymbols[mid]->size)
				lower = mid + 1;
			else
				upper = mid;
		}

		if (lower == fSymbolCount)
			return -1;

		const Symbol* symbol = fSymbols[lower];
		if (address >= symbol->base && address < symbol->base + symbol->size)
			return lower;
		return -1;
	}

private:
	image_info			fInfo;
	Symbol**			fSymbols;
	int32				fSymbolCount;
};


class ThreadImage : public DoublyLinkedListLinkImpl<ThreadImage> {
public:
	ThreadImage(Image* image)
		:
		fImage(image),
		fSymbolHits(NULL),
		fTotalHits(0),
		fMissedTicks(0)
	{
	}

	status_t Init()
	{
		int32 symbolCount = fImage->SymbolCount();
		fSymbolHits = new(std::nothrow) int64[symbolCount];
		if (fSymbolHits == NULL)
			return B_NO_MEMORY;

		memset(fSymbolHits, 0, 8 * symbolCount);

		return B_OK;
	}

	bool ContainsAddress(addr_t address) const
	{
		return fImage->ContainsAddress(address);
	}

	void AddHit(addr_t address)
	{
		int32 symbolIndex = fImage->FindSymbol(address);
		if (symbolIndex >= 0)
			fSymbolHits[symbolIndex]++;
		else
			fMissedTicks++;

		fTotalHits++;
	}

	Image* GetImage() const
	{
		return fImage;
	}

	const int64* SymbolHits() const
	{
		return fSymbolHits;
	}

	int64 TotalHits() const
	{
		return fTotalHits;
	}

	int64 MissedHits() const
	{
		return fMissedTicks;
	}

private:
	Image*	fImage;
	int64*	fSymbolHits;
	int64	fTotalHits;
	int64	fMissedTicks;
};


class Thread {
public:
	Thread(const thread_info& info, Team* team)
		:
		fInfo(info),
		fTeam(team),
		fSampleArea(-1),
		fSamples(NULL),
		fImages(),
		fOldImages(),
		fTotalHits(0),
		fMissedTicks(0),
		fInterval(1)
	{
	}

	~Thread()
	{
		while (ThreadImage* image = fImages.RemoveHead())
			delete image;
		while (ThreadImage* image = fOldImages.RemoveHead())
			delete image;

		if (fSampleArea >= 0)
			delete_area(fSampleArea);
	}

	thread_id ID() const		{ return fInfo.thread; }
	const char* Name() const	{ return fInfo.name; }
	addr_t* Samples() const		{ return fSamples; }
	Team* GetTeam() const		{ return fTeam; }

	void SetSampleArea(area_id area, addr_t* samples)
	{
		fSampleArea = area;
		fSamples = samples;
	}

	void SetInterval(bigtime_t interval)
	{
		fInterval = interval;
	}

	status_t AddImage(Image* image)
	{
		ThreadImage* threadImage = new(std::nothrow) ThreadImage(image);
		if (threadImage == NULL)
			return B_NO_MEMORY;

		status_t error = threadImage->Init();
		if (error != B_OK) {
			delete threadImage;
			return error;
		}

		fImages.Add(threadImage);

		return B_OK;
	}

	ThreadImage* FindImage(addr_t address) const
	{
		ImageList::ConstIterator it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->ContainsAddress(address))
				return image;
		}
		return NULL;
	}

	void AddSamples(int32 count, int32 stackDepth)
	{
		count = count / stackDepth * stackDepth;

		for (int32 i = 0; i < count; i += stackDepth) {
			ThreadImage* image = NULL;
			for (int32 k = 0; k < stackDepth; k++) {
				addr_t address = fSamples[i + k];
				image = FindImage(address);
				if (image != NULL) {
					image->AddHit(address);
					break;
				}
			}

			if (image == NULL)
				fMissedTicks++;
		}

		fTotalHits += count / stackDepth;
	}

	void PrintResults() const
	{
		printf("total hits: %lld, missed: %lld\n", fTotalHits, fMissedTicks);

		int32 symbolCount = 0;

		ImageList::ConstIterator it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			const image_info& imageInfo = image->GetImage()->Info();
			printf("  image: %s (%ld): %lld hits, %lld misses\n",
				imageInfo.name, imageInfo.id, image->TotalHits(),
				image->MissedHits());
			symbolCount += image->GetImage()->SymbolCount();
		}

		// find and sort the hit symbols
		HitSymbol hitSymbols[symbolCount];
		int32 hitSymbolCount = 0;

		it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			Symbol** symbols = image->GetImage()->Symbols();
			const int64* symbolHits = image->SymbolHits();
			int32 imageSymbolCount = image->GetImage()->SymbolCount();
			for (int32 i = 0; i < imageSymbolCount; i++) {
				if (symbolHits[i] > 0) {
					HitSymbol& hitSymbol = hitSymbols[hitSymbolCount++];
					hitSymbol.hits = symbolHits[i];
					hitSymbol.symbol = symbols[i];
				}
			}
		}

		if (hitSymbolCount > 1)
			std::sort(hitSymbols, hitSymbols + hitSymbolCount);

		int64 totalTicks = fTotalHits;
		printf("\nprofiling results for thread \"%s\" (%ld):\n", Name(), ID());
		printf("  tick interval: %lld us\n", fInterval);
		printf("  total ticks:   %lld (%lld us)\n", totalTicks,
			totalTicks * fInterval);
		if (totalTicks == 0)
			totalTicks = 1;
		printf("  missed ticks:  %lld (%lld us, %6.2f%%)\n",
			fMissedTicks, fMissedTicks * fInterval,
			100.0 * fMissedTicks / totalTicks);


		if (hitSymbolCount > 0) {
			printf("\n");
			printf("        hits       in us    in %%  function\n");
			printf("  -------------------------------------------------"
				"-----------------------------\n");
			for (int32 i = 0; i < hitSymbolCount; i++) {
				const HitSymbol& hitSymbol = hitSymbols[i];
				const Symbol* symbol = hitSymbol.symbol;
				printf("  %10lld  %10lld  %6.2f  %s\n", hitSymbol.hits,
					hitSymbol.hits * fInterval,
					100.0 * hitSymbol.hits / totalTicks,
					symbol->Name());
			}
		} else
			printf("  no functions were hit\n");
	}

private:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

private:
	thread_info	fInfo;
	::Team*		fTeam;
	area_id		fSampleArea;
	addr_t*		fSamples;
	ImageList	fImages;
	ImageList	fOldImages;
	int64		fTotalHits;
	int64		fMissedTicks;
	bigtime_t	fInterval;
};


class Team {
public:
	Team()
		:
		fNubPort(-1),
		fImages(20, false)
	{
		fInfo.team = -1;
		fDebugContext.nub_port = -1;
	}

	~Team()
	{
		if (fDebugContext.nub_port >= 0)
			destroy_debug_context(&fDebugContext);

		if (fNubPort >= 0)
			remove_team_debugger(fInfo.team);

// TODO: Just decrement ref-count!
		for (int32 i = 0; Image* image = fImages.ItemAt(i); i++)
			delete image;
	}

	status_t Init(team_id teamID, port_id debuggerPort)
	{
		// get team info
		status_t error = get_team_info(teamID, &fInfo);
		if (error != B_OK)
			return error;

		// install ourselves as the team debugger
		fNubPort = install_team_debugger(teamID, debuggerPort);
		if (fNubPort < 0) {
			fprintf(stderr, "%s: Failed to install as debugger for team %ld: "
				"%s\n", kCommandName, teamID, strerror(fNubPort));
			return fNubPort;
		}

		// init debug context
		error = init_debug_context(&fDebugContext, teamID, fNubPort);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to init debug context for team %ld: "
				"%s\n", kCommandName, teamID, strerror(error));
			return error;
		}

		// create symbol lookup context
		debug_symbol_lookup_context* lookupContext;
		error = debug_create_symbol_lookup_context(&fDebugContext,
			&lookupContext);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to create symbol lookup context for "
				"team %ld: %s\n", kCommandName, teamID, strerror(error));
			return error;
		}

		error = _LoadSymbols(lookupContext);
		debug_delete_symbol_lookup_context(lookupContext);
		if (error != B_OK)
			return error;

		// set team debugging flags
		int32 teamDebugFlags = B_TEAM_DEBUG_THREADS
			| B_TEAM_DEBUG_TEAM_CREATION;
		set_team_debugging_flags(fNubPort, teamDebugFlags);

		return B_OK;
	}

	status_t InitThread(Thread* thread)
	{
		// create the sample area
		char areaName[B_OS_NAME_LENGTH];
		snprintf(areaName, sizeof(areaName), "profiling samples %ld",
			thread->ID());
		void* samples;
		area_id sampleArea = create_area(areaName, &samples, B_ANY_ADDRESS,
			SAMPLE_AREA_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (sampleArea < 0) {
			fprintf(stderr, "%s: Failed to create sample area for thread %ld: "
				"%s\n", kCommandName, thread->ID(), strerror(sampleArea));
			return sampleArea;
		}

		thread->SetSampleArea(sampleArea, (addr_t*)samples);

		// add the current images to the thread
		int32 imageCount = fImages.CountItems();
		for (int32 i = 0; i < imageCount; i++) {
			status_t error = thread->AddImage(fImages.ItemAt(i));
			if (error != B_OK)
				return error;
		}

		// set thread debugging flags and start profiling
		int32 threadDebugFlags = 0;
//		if (!traceTeam) {
//			threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
//				| (traceChildThreads
//					? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
//		}
		set_thread_debugging_flags(fNubPort, thread->ID(), threadDebugFlags);

		// start profiling
		debug_nub_start_profiler message;
		message.reply_port = fDebugContext.reply_port;
		message.thread = thread->ID();
		message.interval = 1000;
		message.sample_area = sampleArea;
		message.stack_depth = SAMPLE_STACK_DEPTH;

		debug_nub_start_profiler_reply reply;
		status_t error = send_debug_message(&fDebugContext,
			B_DEBUG_START_PROFILER, &message, sizeof(message), &reply,
			sizeof(reply));
		if (error != B_OK || (error = reply.error) != B_OK) {
			fprintf(stderr, "%s: Failed to start profiler for thread %ld: %s\n",
				kCommandName, thread->ID(), strerror(error));
			return error;
		}

		thread->SetInterval(reply.interval);
//reply.profile_event

		// resume the target thread to be sure, it's running
		resume_thread(thread->ID());

		return B_OK;
	}

	team_id ID() const
	{
		return fInfo.team;
	}

private:
	status_t _LoadSymbols(debug_symbol_lookup_context* lookupContext)
	{
		// iterate through the team's images and collect the symbols
		image_info imageInfo;
		int32 cookie = 0;
		while (get_next_image_info(fInfo.team, &cookie, &imageInfo) == B_OK) {
			printf("Loading symbols of image \"%s\" (%ld)...\n",
				imageInfo.name, imageInfo.id);

			Image* image = new(std::nothrow) Image(imageInfo);
			if (image == NULL)
				return B_NO_MEMORY;

			status_t error = image->LoadSymbols(lookupContext);
			if (error != B_OK) {
				delete image;
				if (error == B_NO_MEMORY)
					return error;
				continue;
			}

			if (!fImages.AddItem(image)) {
				delete image;
				return B_NO_MEMORY;
			}
		}

		return B_OK;
	}

private:
	team_info					fInfo;
	port_id						fNubPort;
	debug_context				fDebugContext;
	BObjectList<Image>			fImages;
};


class ThreadManager {
public:
	ThreadManager(port_id debuggerPort)
		:
		fTeams(20, true),
		fThreads(20, true),
		fDebuggerPort(debuggerPort)
	{
	}

	status_t AddTeam(team_id teamID, Team** _team = NULL)
	{
		if (FindTeam(teamID) != NULL)
			return B_BAD_VALUE;

		Team* team = new(std::nothrow) Team;
		if (team == NULL)
			return B_NO_MEMORY;

		status_t error = team->Init(teamID, fDebuggerPort);
		if (error != B_OK) {
			delete team;
			return error;
		}

		fTeams.AddItem(team);

		if (_team != NULL)
			*_team = team;

		return B_OK;
	}

	status_t AddThread(thread_id threadID)
	{
		if (FindThread(threadID) != NULL)
			return B_BAD_VALUE;

		thread_info threadInfo;
		status_t error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK)
			return error;

		Team* team = FindTeam(threadInfo.team);
		if (team == NULL)
			return B_BAD_TEAM_ID;

		Thread* thread = new(std::nothrow) Thread(threadInfo, team);
		if (thread == NULL)
			return B_NO_MEMORY;

		error = team->InitThread(thread);
		if (error != B_OK) {
			delete thread;
			return error;
		}

		fThreads.AddItem(thread);
		return B_OK;
	}

	void RemoveTeam(team_id teamID)
	{
		if (Team* team = FindTeam(teamID))
			fTeams.RemoveItem(team, true);
	}

	void RemoveThread(thread_id threadID)
	{
		if (Thread* thread = FindThread(threadID))
			fThreads.RemoveItem(thread, true);
	}

	Team* FindTeam(team_id teamID) const
	{
		for (int32 i = 0; Team* team = fTeams.ItemAt(i); i++) {
			if (team->ID() == teamID)
				return team;
		}
		return NULL;
	}

	Thread* FindThread(thread_id threadID) const
	{
		for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++) {
			if (thread->ID() == threadID)
				return thread;
		}
		return NULL;
	}

private:
	BObjectList<Team>				fTeams;
	BObjectList<Thread>				fThreads;
	port_id							fDebuggerPort;
};


// TODO: Adjust!
static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and print an analysis of\n"
	"the user and kernel times of all threads that ran during that time.\n"
	"\n"
	"Options:\n"
	"  -h, --help   - Print this usage info.\n"
	"  -o <output>  - Print the results to file <output>.\n"
	"  -s           - Also perform a scheduling analysis over the time the\n"
	"                 child process ran. This requires that scheduler kernel\n"
	"                 tracing had been enabled at compile time.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, __progname);
    exit(error ? 1 : 0);
}


/*
// get_id
static bool
get_id(const char *str, int32 &id)
{
	int32 len = strlen(str);
	for (int32 i = 0; i < len; i++) {
		if (!isdigit(str[i]))
			return false;
	}

	id = atol(str);
	return true;
}
*/


int
main(int argc, const char* const* argv)
{
//	const char* outputFile = NULL;
//	bool schedulingAnalysis = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
//			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;
/*			case 'o':
				outputFile = optarg;
				break;
			case 's':
				schedulingAnalysis = true;
				break;
*/

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	// get thread/team to be debugged
	thread_id threadID = -1;
	team_id teamID = -1;
//	if (programArgCount > 1
//		|| !get_id(*programArgs, (traceTeam ? teamID : thread))) {
		// we've been given an executable and need to load it
		threadID = load_program(programArgs, programArgCount, false);
		if (threadID < 0) {
			fprintf(stderr, "%s: Failed to start `%s': %s\n", kCommandName,
				programArgs[0], strerror(threadID));
			exit(1);
		}
//	}

	// get the team ID, if we have none yet
	if (teamID < 0) {
		thread_info threadInfo;
		status_t error = get_thread_info(threadID, &threadInfo);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to get info for thread %ld: %s\n",
				kCommandName, threadID, strerror(error));
			exit(1);
		}
		teamID = threadInfo.team;
	}

	// create a debugger port
	port_id debuggerPort = create_port(10, "debugger port");
	if (debuggerPort < 0) {
		fprintf(stderr, "%s: Failed to create debugger port: %s\n",
			kCommandName, strerror(debuggerPort));
		exit(1);
	}

	// add team and thread to the thread manager
	ThreadManager threadManager(debuggerPort);
	if (threadManager.AddTeam(teamID) != B_OK
		|| threadManager.AddThread(threadID) != B_OK) {
		exit(1);
	}

	// debug loop
	while (true) {
		debug_debugger_message_data message;
		bool quitLoop = false;
		int32 code;
		ssize_t messageSize = read_port(debuggerPort, &code, &message,
			sizeof(message));

		if (messageSize < 0) {
			if (messageSize == B_INTERRUPTED)
				continue;

			fprintf(stderr, "%s: Reading from debugger port failed: %s\n",
				kCommandName, strerror(messageSize));
			exit(1);
		}

		switch (code) {
			case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
			{
debug_printf("B_DEBUGGER_MESSAGE_PROFILER_UPDATE: thread %ld, %ld samples\n",
message.profiler_update.origin.thread, message.profiler_update.sample_count);
				Thread* thread = threadManager.FindThread(
					message.profiler_update.origin.thread);
				if (thread == NULL)
					break;

				thread->AddSamples(message.profiler_update.sample_count,
					message.profiler_update.stack_depth);

				if (message.profiler_update.stopped) {
					thread->PrintResults();
					threadManager.RemoveThread(thread->ID());
				}
				break;
			}

			case B_DEBUGGER_MESSAGE_TEAM_CREATED:
				if (threadManager.AddTeam(message.team_created.new_team)
						== B_OK) {
					threadManager.AddThread(message.team_created.new_team);
				}
				break;
			case B_DEBUGGER_MESSAGE_TEAM_DELETED:
				// a debugged team is gone -- quit, if it is our team
				quitLoop = message.origin.team == teamID;
				break;
			case B_DEBUGGER_MESSAGE_THREAD_CREATED:
				threadManager.AddThread(message.thread_created.new_thread);
				break;
			case B_DEBUGGER_MESSAGE_THREAD_DELETED:
				threadManager.RemoveThread(message.origin.thread);
				break;

			case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				break;
		}

		if (quitLoop)
			break;

		// tell the thread to continue (only when there is a thread and the
		// message was synchronous)
		if (message.origin.thread >= 0 && message.origin.nub_port >= 0)
			continue_thread(message.origin.nub_port, message.origin.thread);
	}

	return 0;
}
