/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <errno.h>
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
#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "debug_utils.h"


extern const char* __progname;
static const char* kCommandName = __progname;


class Image;
class Team;


enum {
	SAMPLE_AREA_SIZE	= 128 * 1024,
};


static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and periodically samples\n"
	"all started threads' program counters. When a thread terminates, a list\n"
	"of the functions where the thread was encountered is printed.\n"
	"\n"
	"Options:\n"
	"  -c             - Don't profile child threads. Default is to\n"
	"                   recursively profile all threads created by a profiled\n"
	"                   thread.\n"
	"  -C             - Don't profile child teams. Default is to recursively\n"
	"                   profile all teams created by a profiled team.\n"
	"  -h, --help     - Print this usage info.\n"
	"  -i <interval>  - Use a tick interval of <interval> microseconds.\n"
	"                   Default is 1000 (1 ms). On a fast machine, a shorter\n"
	"                   interval might lead to better results, while it might\n"
	"                   make them worse on slow machines.\n"
	"  -k             - Don't check kernel images for hits.\n"
	"  -l             - Also profile loading the executable.\n"
	"  -o <output>    - Print the results to file <output>.\n"
	"  -s <depth>     - Number of return address samples to take from the\n"
	"                   caller stack per tick. If the topmost address doesn't\n"
	"                   hit a known image, the next address will be matched\n"
	"                   (and so on).\n"
;


struct Options {
	Options()
		:
		interval(1000),
		stack_depth(5),
		output(NULL),
		profile_kernel(true),
		profile_loading(false),
		profile_teams(true),
		profile_threads(true)
	{
	}

	bigtime_t	interval;
	int32		stack_depth;
	FILE*		output;
	bool		profile_kernel;
	bool		profile_loading;
	bool		profile_teams;
	bool		profile_threads;
};

static Options sOptions;


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


class Image : public Referenceable {
public:
	Image(const image_info& info, team_id owner, int32 creationEvent)
		:
		fInfo(info),
		fOwner(owner),
		fSymbols(NULL),
		fSymbolCount(0),
		fCreationEvent(creationEvent),
		fDeletionEvent(-1)
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

	const image_id ID() const
	{
		return fInfo.id;
	}

	const image_info& Info() const
	{
		return fInfo;
	}

	team_id Owner() const
	{
		return fOwner;
	}

	int32 CreationEvent() const
	{
		return fCreationEvent;
	}

	int32 DeletionEvent() const
	{
		return fDeletionEvent;
	}

	void SetDeletionEvent(int32 event)
	{
		fDeletionEvent = event;
	}

	status_t LoadSymbols(debug_symbol_lookup_context* lookupContext)
	{
//		fprintf(sOptions.output, "Loading symbols of image \"%s\" (%ld)...\n",
//			fInfo.name, fInfo.id);

		// create symbol iterator
		debug_symbol_iterator* iterator;
		status_t error = debug_create_image_symbol_iterator(lookupContext,
			fInfo.id, &iterator);
		if (error != B_OK) {
			fprintf(stderr, "Failed to init symbol iterator: %s\n",
				strerror(error));
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
	team_id				fOwner;
	Symbol**			fSymbols;
	int32				fSymbolCount;
	int32				fCreationEvent;
	int32				fDeletionEvent;
};


class ThreadImage : public DoublyLinkedListLinkImpl<ThreadImage> {
public:
	ThreadImage(Image* image)
		:
		fImage(image),
		fSymbolHits(NULL),
		fTotalHits(0),
		fUnknownHits(0)
	{
		fImage->AddReference();
	}

	~ThreadImage()
	{
		fImage->RemoveReference();
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

	image_id ID() const
	{
		return fImage->ID();
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
			fUnknownHits++;

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

	int64 UnknownHits() const
	{
		return fUnknownHits;
	}

private:
	Image*	fImage;
	int64*	fSymbolHits;
	int64	fTotalHits;
	int64	fUnknownHits;
};


class Thread : public DoublyLinkedListLinkImpl<Thread> {
public:
	Thread(const thread_info& info, Team* team)
		:
		fInfo(info),
		fTeam(team),
		fSampleArea(-1),
		fSamples(NULL),
		fImages(),
		fOldImages(),
		fTotalTicks(0),
		fUnkownTicks(0),
		fDroppedTicks(0),
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

	void UpdateInfo()
	{
		thread_info info;
		if (get_thread_info(ID(), &info) == B_OK)
			fInfo = info;
	}

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

	void ImageRemoved(Image* image)
	{
		ImageList::Iterator it = fImages.GetIterator();
		while (ThreadImage* threadImage = it.Next()) {
			if (threadImage->GetImage() == image) {
				fImages.Remove(threadImage);
				if (threadImage->TotalHits() > 0)
					fOldImages.Add(threadImage);
				else
					delete threadImage;
				return;
			}
		}
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

	void AddSamples(int32 count, int32 dropped, int32 stackDepth, int32 event)
	{
		_RemoveObsoleteImages(event);

		// Temporarily remove images that have been created after the given
		// event.
		ImageList newImages;
		ImageList::Iterator it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->GetImage()->CreationEvent() > event) {
				it.Remove();
				newImages.Add(image);
			}
		}

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
				fUnkownTicks++;
		}

		fTotalTicks += count / stackDepth;
		fDroppedTicks += dropped;

		// re-add the new images
		fImages.MoveFrom(&newImages);

		_SynchronizeImages();
	}

	void PrintResults() const
	{
		// count images and symbols
		int32 imageCount = 0;
		int32 symbolCount = 0;

		ImageList::ConstIterator it = fOldImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->TotalHits() > 0) {
				imageCount++;
				if (image->TotalHits() > image->UnknownHits())
					symbolCount += image->GetImage()->SymbolCount();
			}
		}

		it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->TotalHits() > 0) {
				imageCount++;
				if (image->TotalHits() > image->UnknownHits())
					symbolCount += image->GetImage()->SymbolCount();
			}
		}

		ThreadImage* images[imageCount];
		imageCount = 0;

		it = fOldImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->TotalHits() > 0)
				images[imageCount++] = image;
		}

		it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->TotalHits() > 0)
				images[imageCount++] = image;
		}

		// find and sort the hit symbols
		HitSymbol hitSymbols[symbolCount];
		int32 hitSymbolCount = 0;

		for (int32 k = 0; k < imageCount; k++) {
			ThreadImage* image = images[k];
			if (image->TotalHits() > image->UnknownHits()) {
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
		}

		if (hitSymbolCount > 1)
			std::sort(hitSymbols, hitSymbols + hitSymbolCount);

		int64 totalTicks = fTotalTicks;
		fprintf(sOptions.output, "\nprofiling results for thread \"%s\" "
			"(%ld):\n", Name(), ID());
		fprintf(sOptions.output, "  tick interval:  %lld us\n", fInterval);
		fprintf(sOptions.output, "  total ticks:    %lld (%lld us)\n",
			totalTicks, totalTicks * fInterval);
		if (totalTicks == 0)
			totalTicks = 1;
		fprintf(sOptions.output, "  unknown ticks:  %lld (%lld us, %6.2f%%)\n",
			fUnkownTicks, fUnkownTicks * fInterval,
			100.0 * fUnkownTicks / totalTicks);
		fprintf(sOptions.output, "  dropped ticks:  %lld (%lld us, %6.2f%%)\n",
			fDroppedTicks, fDroppedTicks * fInterval,
			100.0 * fDroppedTicks / totalTicks);

		if (imageCount > 0) {
			fprintf(sOptions.output, "\n");
			fprintf(sOptions.output, "        hits     unknown    image\n");
			fprintf(sOptions.output, "  ---------------------------------------"
				"---------------------------------------\n");
			for (int32 k = 0; k < imageCount; k++) {
				ThreadImage* image = images[k];
				const image_info& imageInfo = image->GetImage()->Info();
				fprintf(sOptions.output, "  %10lld  %10lld  %7ld %s\n",
					image->TotalHits(), image->UnknownHits(), imageInfo.id,
					imageInfo.name);
			}
		}

		if (hitSymbolCount > 0) {
			fprintf(sOptions.output, "\n");
			fprintf(sOptions.output, "        hits       in us    in %%   "
				"image  function\n");
			fprintf(sOptions.output, "  ---------------------------------------"
				"---------------------------------------\n");
			for (int32 i = 0; i < hitSymbolCount; i++) {
				const HitSymbol& hitSymbol = hitSymbols[i];
				const Symbol* symbol = hitSymbol.symbol;
				fprintf(sOptions.output, "  %10lld  %10lld  %6.2f  %6ld  %s\n",
					hitSymbol.hits, hitSymbol.hits * fInterval,
					100.0 * hitSymbol.hits / totalTicks, symbol->image->ID(),
					symbol->Name());
			}
		} else
			fprintf(sOptions.output, "  no functions were hit\n");
	}

private:
	void _SynchronizeImages();

	void _RemoveObsoleteImages(int32 event)
	{
		// remove obsolete images
		ImageList::Iterator it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			int32 deleted = image->GetImage()->DeletionEvent();
			if (deleted >= 0 && event >= deleted) {
				it.Remove();
				if (image->TotalHits() > 0)
					fOldImages.Add(image);
				else
					delete image;
			}
		}
	}

	ThreadImage* _FindImageByID(image_id id) const
	{
		ImageList::ConstIterator it = fImages.GetIterator();
		while (ThreadImage* image = it.Next()) {
			if (image->ID() == id)
				return image;
		}

		return NULL;
	}

private:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

	thread_info	fInfo;
	::Team*		fTeam;
	area_id		fSampleArea;
	addr_t*		fSamples;
	ImageList	fImages;
	ImageList	fOldImages;
	int64		fTotalTicks;
	int64		fUnkownTicks;
	int64		fDroppedTicks;
	bigtime_t	fInterval;
};


class Team {
public:
	Team()
		:
		fNubPort(-1),
		fThreads(),
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

		for (int32 i = 0; Image* image = fImages.ItemAt(i); i++)
			image->RemoveReference();
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

		// load the team's images and their symbols
		error = _LoadSymbols(lookupContext, ID());
		debug_delete_symbol_lookup_context(lookupContext);
		if (error != B_OK)
			return error;

		// also try to load the kernel images and symbols
		if (sOptions.profile_kernel) {
			// fake a debug context -- it's not really needed anyway
			debug_context debugContext;
			debugContext.team = B_SYSTEM_TEAM;
			debugContext.nub_port = -1;
			debugContext.reply_port = -1;

			// create symbol lookup context
			error = debug_create_symbol_lookup_context(&debugContext,
				&lookupContext);
			if (error != B_OK) {
				fprintf(stderr, "%s: Failed to create symbol lookup context "
					"for the kernel team: %s\n", kCommandName, strerror(error));
				return error;
			}

			// load the kernel's images and their symbols
			_LoadSymbols(lookupContext, B_SYSTEM_TEAM);
			debug_delete_symbol_lookup_context(lookupContext);
		}

		// set team debugging flags
		int32 teamDebugFlags = B_TEAM_DEBUG_THREADS
			| B_TEAM_DEBUG_TEAM_CREATION | B_TEAM_DEBUG_IMAGES;
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
		message.interval = sOptions.interval;
		message.sample_area = sampleArea;
		message.stack_depth = sOptions.stack_depth;

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

		fThreads.Add(thread);

		// resume the target thread to be sure, it's running
		resume_thread(thread->ID());

		return B_OK;
	}

	void RemoveThread(Thread* thread)
	{
		fThreads.Remove(thread);
	}

	void Exec(int32 event)
	{
		// remove all non-kernel images
		int32 imageCount = fImages.CountItems();
		for (int32 i = imageCount - 1; i >= 0; i--) {
			Image* image = fImages.ItemAt(i);
			if (image->Owner() == ID())
				_RemoveImage(i, event);
		}

		// update the main thread
		ThreadList::Iterator it = fThreads.GetIterator();
		while (Thread* thread = it.Next()) {
			if (thread->ID() == ID()) {
				thread->UpdateInfo();
				break;
			}
		}
	}

	status_t AddImage(const image_info& imageInfo, int32 event)
	{
		// create symbol lookup context
		debug_symbol_lookup_context* lookupContext;
		status_t error = debug_create_symbol_lookup_context(&fDebugContext,
			&lookupContext);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to create symbol lookup context for "
				"team %ld: %s\n", kCommandName, ID(), strerror(error));
			return error;
		}

		Image* image;
		error = _LoadImageSymbols(lookupContext, imageInfo, ID(), event,
			&image);
		debug_delete_symbol_lookup_context(lookupContext);

		// Although we generally synchronize the threads' images lazily, we have
		// to add new images at least, since otherwise images could be added
		// and removed again, and the hits inbetween could never be matched.
		if (error == B_OK) {
			ThreadList::Iterator it = fThreads.GetIterator();
			while (Thread* thread = it.Next())
				thread->AddImage(image);
		}

		return error;
	}

	status_t RemoveImage(const image_info& imageInfo, int32 event)
	{
		for (int32 i = 0; Image* image = fImages.ItemAt(i); i++) {
			if (image->ID() == imageInfo.id) {
				_RemoveImage(i, event);
				return B_OK;
			}
		}

		return B_ENTRY_NOT_FOUND;
	}

	const BObjectList<Image>& Images() const
	{
		return fImages;
	}

	Image* FindImage(image_id id) const
	{
		for (int32 i = 0; Image* image = fImages.ItemAt(i); i++) {
			if (image->ID() == id)
				return image;
		}

		return NULL;
	}

	team_id ID() const
	{
		return fInfo.team;
	}

private:
	status_t _LoadSymbols(debug_symbol_lookup_context* lookupContext,
		team_id owner)
	{
		// iterate through the team's images and collect the symbols
		image_info imageInfo;
		int32 cookie = 0;
		while (get_next_image_info(owner, &cookie, &imageInfo) == B_OK) {
			status_t error = _LoadImageSymbols(lookupContext, imageInfo,
				owner, 0);
			if (error == B_NO_MEMORY)
				return error;
		}

		return B_OK;
	}

	status_t _LoadImageSymbols(debug_symbol_lookup_context* lookupContext,
		const image_info& imageInfo, team_id owner, int32 event,
		Image** _image = NULL)
	{
		Image* image = new(std::nothrow) Image(imageInfo, owner, event);
		if (image == NULL)
			return B_NO_MEMORY;

		status_t error = image->LoadSymbols(lookupContext);
		if (error != B_OK) {
			delete image;
			return error;
		}

		if (!fImages.AddItem(image)) {
			delete image;
			return B_NO_MEMORY;
		}

		if (_image != NULL)
			*_image = image;

		return B_OK;
	}

	void _RemoveImage(int32 index, int32 event)
	{
		Image* image = fImages.RemoveItemAt(index);
		if (image == NULL)
			return;

		// Note: We don't tell the threads that the image has been removed. They
		// will be updated lazily when their next profiler update arrives. This
		// is necessary, since the update might contain samples hitting that
		// image.

		image->SetDeletionEvent(event);
		image->RemoveReference();
	}

private:
	typedef DoublyLinkedList<Thread> ThreadList;

	team_info					fInfo;
	port_id						fNubPort;
	debug_context				fDebugContext;
	ThreadList					fThreads;
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
		if (Thread* thread = FindThread(threadID)) {
			thread->GetTeam()->RemoveThread(thread);
			fThreads.RemoveItem(thread, true);
		}
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


void
Thread::_SynchronizeImages()
{
	const BObjectList<Image>& teamImages = fTeam->Images();

	// remove obsolete images
	ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (fTeam->FindImage(image->ID()) == NULL) {
			it.Remove();
			if (image->TotalHits() > 0)
				fOldImages.Add(image);
			else
				delete image;
		}
	}

	// add new images
	for (int32 i = 0; Image* image = teamImages.ItemAt(i); i++) {
		if (_FindImageByID(image->ID()) == NULL)
			AddImage(image);
	}
}


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
	const char* outputFile = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+cChi:klo:s:", sLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'c':
				sOptions.profile_threads = false;
				break;
			case 'C':
				sOptions.profile_teams = false;
				break;
			case 'h':
				print_usage_and_exit(false);
				break;
			case 'i':
				sOptions.interval = atol(optarg);
				break;
			case 'k':
				sOptions.profile_kernel = false;
				break;
			case 'l':
				sOptions.profile_loading = true;
				break;
			case 'o':
				outputFile = optarg;
				break;
			case 's':
				sOptions.stack_depth = atol(optarg);
				break;
			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	const char* const* programArgs = argv + optind;
	int programArgCount = argc - optind;

	if (outputFile != NULL) {
		sOptions.output = fopen(outputFile, "w+");
		if (sOptions.output == NULL) {
			fprintf(stderr, "%s: Failed to open output file \"%s\": %s\n",
				kCommandName, outputFile, strerror(errno));
			exit(1);
		}
	} else
		sOptions.output = stdout;

	// get thread/team to be debugged
	thread_id threadID = -1;
	team_id teamID = -1;
//	if (programArgCount > 1
//		|| !get_id(*programArgs, (traceTeam ? teamID : thread))) {
		// we've been given an executable and need to load it
		threadID = load_program(programArgs, programArgCount,
			sOptions.profile_loading);
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
				Thread* thread = threadManager.FindThread(
					message.profiler_update.origin.thread);
				if (thread == NULL)
					break;

				thread->AddSamples(message.profiler_update.sample_count,
					message.profiler_update.dropped_ticks,
					message.profiler_update.stack_depth,
					message.profiler_update.image_event);

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
			case B_DEBUGGER_MESSAGE_TEAM_EXEC:
				if (Team* team = threadManager.FindTeam(message.origin.team))
					team->Exec(message.team_exec.image_event);
				break;

			case B_DEBUGGER_MESSAGE_THREAD_CREATED:
				if (!sOptions.profile_threads)
					break;

				threadManager.AddThread(message.thread_created.new_thread);
				break;
			case B_DEBUGGER_MESSAGE_THREAD_DELETED:
				threadManager.RemoveThread(message.origin.thread);
				break;

			case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
				if (!sOptions.profile_teams)
					break;

				if (Team* team = threadManager.FindTeam(message.origin.team)) {
					team->AddImage(message.image_created.info,
						message.image_created.image_event);
				}
				break;
			case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
				if (Team* team = threadManager.FindTeam(message.origin.team)) {
					team->RemoveImage(message.image_deleted.info,
						message.image_deleted.image_event);
				}
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
