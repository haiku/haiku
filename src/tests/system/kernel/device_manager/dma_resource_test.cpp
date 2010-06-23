/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <device_manager.h>

#include <vm/vm.h>

#include "dma_resources.h"
#include "io_requests.h"
#include "IOSchedulerSimple.h"


#define DMA_TEST_BLOCK_SIZE				512
#define DMA_TEST_BUFFER_COUNT			10
#define DMA_TEST_BOUNCE_BUFFER_COUNT	128


class TestSuite;

class TestSuiteContext {
public:
							TestSuiteContext();
							~TestSuiteContext();

			status_t		Init(size_t size, bool contiguous = true);

			addr_t			DataBase() const { return fDataBase; }
			addr_t			PhysicalDataBase() const
								{ return fPhysicalDataBase; }
			addr_t			SecondPhysicalDataBase() const
								{ return fSecondPhysicalDataBase; }

			bool			IsContiguous() const
								{ return fSecondPhysicalDataBase == 0; }

			addr_t			CompareBase() const { return fCompareBase; }

			size_t			Size() const { return fSize; }

private:
			void			_Uninit();

			area_id			fDataArea;
			addr_t			fDataBase;
			addr_t			fPhysicalDataBase;
			addr_t			fSecondPhysicalDataBase;
			area_id			fCompareArea;
			addr_t			fCompareBase;
			size_t			fSize;
};

class Test : public DoublyLinkedListLinkImpl<Test> {
public:
							Test(TestSuite& suite, off_t offset, size_t length,
								bool isWrite, uint32 flags);

			Test&			AddSource(addr_t base, size_t length);
			Test&			NextResult(off_t offset, bool partialBegin,
								bool partialEnd);
			Test&			AddTarget(addr_t base, size_t length,
								bool usesBounceBuffer);

			void			Run(DMAResource& resource);

private:
			addr_t			_SourceToVirtual(addr_t source);
			addr_t			_SourceToCompare(addr_t source);
			void			_Prepare();
			void			_CheckCompare();
			void			_CheckWrite();
			void			_CheckResults();
			status_t		_DoIO(IOOperation& operation);
			void			_Panic(const char* message,...);

			TestSuite&		fSuite;
			off_t			fOffset;
			size_t			fLength;
			bool			fIsWrite;
			uint32			fFlags;
			generic_io_vec	fSourceVecs[32];
			uint32			fSourceCount;

			struct target_t {
				addr_t		address;
				size_t		length;
				bool		uses_bounce_buffer;
			};
			struct result_t {
				off_t		offset;
				target_t	targets[32];
				uint32		count;
				bool		partial_begin;
				bool		partial_end;
			};
			result_t		fResults[32];
			uint32			fResultCount;
};

typedef DoublyLinkedList<Test> TestList;


class TestSuite {
public:
	TestSuite(TestSuiteContext& context, const char* name,
			const dma_restrictions& restrictions, size_t blockSize)
		:
		fContext(context)
	{
		dprintf("----- Run \"%s\" tests ---------------------------\n", name);
		dprintf("  DMA restrictions: address %#" B_PRIxGENADDR " - %#"
			B_PRIxGENADDR ", align %" B_PRIuGENADDR ", boundary %" B_PRIuGENADDR
			",\n    max transfer %" B_PRIuGENADDR ", max segs %" B_PRIx32
			", max seg size %" B_PRIxGENADDR ", flags %" B_PRIx32 "\n\n",
			restrictions.low_address, restrictions.high_address,
			restrictions.alignment, restrictions.boundary,
			restrictions.max_transfer_size, restrictions.max_segment_count,
			restrictions.max_segment_size, restrictions.flags);

		status_t status = fDMAResource.Init(restrictions, blockSize, 10, 10);
		if (status != B_OK)
			panic("initializing DMA resource failed: %s\n", strerror(status));
	}

	~TestSuite()
	{
		while (Test* test = fTests.RemoveHead()) {
			delete test;
		}
	}

	Test& AddTest(off_t offset, size_t length, bool isWrite, uint32 flags)
	{
		Test* test = new(std::nothrow) Test(*this, offset, length, isWrite,
			flags);
		fTests.Add(test);

		return *test;
	}

	void Run()
	{
		TestList::Iterator iterator = fTests.GetIterator();
		uint32 count = 1;
		while (Test* test = iterator.Next()) {
			dprintf("test %lu...\n", count++);
			test->Run(fDMAResource);
		}
	}

	addr_t DataBase() const { return fContext.DataBase(); }
	addr_t PhysicalDataBase() const { return fContext.PhysicalDataBase(); }
	addr_t SecondPhysicalDataBase() const
		{ return fContext.SecondPhysicalDataBase(); }
	bool IsContiguous() const { return fContext.IsContiguous(); }
	addr_t CompareBase() const { return fContext.CompareBase(); }
	size_t Size() const { return fContext.Size(); }

private:
	TestSuiteContext& fContext;
	DMAResource		fDMAResource;
	uint8*			fBase;
	uint8*			fPhysicalBase;
	size_t			fSize;
	TestList		fTests;
};


struct device_manager_info* sDeviceManager;

static area_id sArea;
static size_t sAreaSize;
static void* sAreaAddress;
static DMAResource* sDMAResource;
static IOScheduler* sIOScheduler;


status_t
do_io(void* data, IOOperation* operation)
{
	uint8* disk = (uint8*)sAreaAddress;
	off_t offset = operation->Offset();

	for (uint32 i = 0; i < operation->VecCount(); i++) {
		const generic_io_vec& vec = operation->Vecs()[i];
		generic_addr_t base = vec.base;
		generic_size_t length = vec.length;

		if (operation->IsWrite())
			vm_memcpy_from_physical(disk + offset, base, length, false);
		else
			vm_memcpy_to_physical(base, disk + offset, length, false);
	}

	if (sIOScheduler != NULL)
		sIOScheduler->OperationCompleted(operation, B_OK, operation->Length());
	return B_OK;
}


//	#pragma mark -


TestSuiteContext::TestSuiteContext()
	:
	fDataArea(-1),
	fCompareArea(-1),
	fSize(0)
{
}


TestSuiteContext::~TestSuiteContext()
{
	_Uninit();
}


void
TestSuiteContext::_Uninit()
{
	delete_area(fDataArea);
	delete_area(fCompareArea);
}


status_t
TestSuiteContext::Init(size_t size, bool contiguous)
{
	if (!contiguous) {
		// we can't force this, so we have to try and see

		if (size != B_PAGE_SIZE * 2)
			return B_NOT_SUPPORTED;

		while (true) {
			fDataArea = create_area("data buffer", (void**)&fDataBase,
				B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
			if (fDataArea < B_OK)
				return fDataArea;

			// get memory map to see if we succeeded

			physical_entry entry[2];
			get_memory_map((void*)fDataBase, 2 * B_PAGE_SIZE, entry, 2);

			if (entry[0].size == B_PAGE_SIZE) {
				fPhysicalDataBase = (addr_t)entry[0].address;
				fSecondPhysicalDataBase = (addr_t)entry[1].address;

				dprintf("DMA Test area %p, physical %#" B_PRIxPHYSADDR
					", second physical %#" B_PRIxPHYSADDR "\n",
					(void*)fDataBase, entry[0].address, entry[1].address);
				break;
			}

			// try again
			delete_area(fDataArea);
		}
	} else {
		fDataArea = create_area("data buffer", (void**)&fDataBase,
			B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (fDataArea < B_OK)
			return fDataArea;

		physical_entry entry;
		get_memory_map((void*)fDataBase, B_PAGE_SIZE, &entry, 1);

		fPhysicalDataBase = (addr_t)entry.address;
		fSecondPhysicalDataBase = 0;

		dprintf("DMA Test area %p, physical %#" B_PRIxPHYSADDR "\n",
			(void*)fDataBase, entry.address);
	}

	fCompareArea = create_area("compare buffer", (void**)&fCompareBase,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (fCompareArea < B_OK)
		return fCompareArea;

	fSize = size;
	return B_OK;
}


//	#pragma mark -


Test::Test(TestSuite& suite, off_t offset, size_t length, bool isWrite,
		uint32 flags)
	:
	fSuite(suite),
	fOffset(offset),
	fLength(length),
	fIsWrite(isWrite),
	fFlags(flags),
	fSourceCount(0),
	fResultCount(0)
{
}


Test&
Test::AddSource(addr_t address, size_t length)
{
	if (fSuite.IsContiguous() || (fFlags & B_PHYSICAL_IO_REQUEST) != 0
		|| address < B_PAGE_SIZE) {
		fSourceVecs[fSourceCount].base
			= ((fFlags & B_PHYSICAL_IO_REQUEST) == 0
				? fSuite.DataBase() : fSuite.PhysicalDataBase()) + address;
	} else {
		fSourceVecs[fSourceCount].base
			= fSuite.SecondPhysicalDataBase() + address;
	}
	fSourceVecs[fSourceCount].length = length;
	fSourceCount++;

	return *this;
}


Test&
Test::NextResult(off_t offset, bool partialBegin, bool partialEnd)
{
	fResults[fResultCount].offset = offset;
	fResults[fResultCount].count = 0;
	fResults[fResultCount].partial_begin = partialBegin;
	fResults[fResultCount].partial_end = partialEnd;
	fResultCount++;

	return *this;
}


Test&
Test::AddTarget(addr_t base, size_t length, bool usesBounceBuffer)
{
	struct result_t& result = fResults[fResultCount - 1];
	struct target_t& target = result.targets[result.count++];

	target.address = base;
	target.length = length;
	target.uses_bounce_buffer = usesBounceBuffer;

	return *this;
}


addr_t
Test::_SourceToVirtual(addr_t source)
{
	if ((fFlags & B_PHYSICAL_IO_REQUEST) != 0) {
		if (!fSuite.IsContiguous() && source >= B_PAGE_SIZE)
			return source - fSuite.SecondPhysicalDataBase() + fSuite.DataBase();

		return source - fSuite.PhysicalDataBase() + fSuite.DataBase();
	}

	return source;
}


addr_t
Test::_SourceToCompare(addr_t source)
{
	if ((fFlags & B_PHYSICAL_IO_REQUEST) != 0) {
		if (!fSuite.IsContiguous() && source >= B_PAGE_SIZE) {
			return source - fSuite.SecondPhysicalDataBase()
				+ fSuite.CompareBase();
		}

		return source - fSuite.PhysicalDataBase() + fSuite.CompareBase();
	}

	return source - fSuite.DataBase() + fSuite.CompareBase();
}


void
Test::_Prepare()
{
	// prepare disk

	uint8* disk = (uint8*)sAreaAddress;
	for (size_t i = 0; i < sAreaSize; i++) {
		disk[i] = i % 26 + 'a';
	}

	// prepare data

	memset((void*)fSuite.DataBase(), 0xcc, fSuite.Size());

	if (fIsWrite) {
		off_t offset = fOffset;
		size_t length = fLength;

		for (uint32 i = 0; i < fSourceCount; i++) {
			uint8* data = (uint8*)_SourceToVirtual(fSourceVecs[i].base);
			size_t vecLength = min_c(fSourceVecs[i].length, length);

			for (uint32 j = 0; j < vecLength; j++) {
				data[j] = (offset + j) % 10 + '0';
			}
			offset += vecLength;
			length -= vecLength;
		}
	}

	// prepare compare data

	memset((void*)fSuite.CompareBase(), 0xcc, fSuite.Size());

	if (fIsWrite) {
		// copy data from source
		off_t offset = fOffset;
		size_t length = fLength;

		for (uint32 i = 0; i < fSourceCount; i++) {
			uint8* compare = (uint8*)_SourceToCompare(fSourceVecs[i].base);
			size_t vecLength = min_c(fSourceVecs[i].length, length);

			memcpy(compare, (void*)_SourceToVirtual(fSourceVecs[i].base),
				vecLength);
			offset += vecLength;
			length -= vecLength;
		}
	} else {
		// copy data from drive
		off_t offset = fOffset;
		size_t length = fLength;

		for (uint32 i = 0; i < fSourceCount; i++) {
			uint8* compare = (uint8*)_SourceToCompare(fSourceVecs[i].base);
			size_t vecLength = min_c(fSourceVecs[i].length, length);

			memcpy(compare, disk + offset, vecLength);
			offset += vecLength;
			length -= vecLength;
		}
	}

	if (fIsWrite)
		_CheckCompare();
}


void
Test::_CheckCompare()
{
	uint8* data = (uint8*)fSuite.DataBase();
	uint8* compare = (uint8*)fSuite.CompareBase();

	for (size_t i = 0; i < fSuite.Size(); i++) {
		if (data[i] != compare[i]) {
			dprintf("offset %lu differs, %s:\n", i,
				fIsWrite ? "write" : "read");
			i &= ~63;
			dump_block((char*)&data[i], min_c(64, fSuite.Size() - i), "  ");
			dprintf("should be:\n");
			dump_block((char*)&compare[i], min_c(64, fSuite.Size() - i), "  ");

			_Panic("Data %s differs", fIsWrite ? "write" : "read");
		}
	}
}


void
Test::_CheckWrite()
{
	_CheckCompare();

	// check if we overwrote parts we shouldn't have

	uint8* disk = (uint8*)sAreaAddress;
	for (size_t i = 0; i < sAreaSize; i++) {
		if (i >= fOffset && i < fOffset + fLength)
			continue;

		if (disk[i] != i % 26 + 'a') {
			dprintf("disk[i] %c, expected %c, i %lu, fLength + fOffset %Ld\n",
				disk[i], (int)(i % 26 + 'a'), i, fLength + fOffset);
			dprintf("offset %lu differs, touched innocent data:\n", i);
			i &= ~63;
			dump_block((char*)&disk[i], min_c(64, fSuite.Size() - i), "  ");

			_Panic("Data %s differs", fIsWrite ? "write" : "read");
		}
	}

	// check if the data we wanted to have on disk ended up there

	off_t offset = fOffset;
	size_t length = fLength;

	for (uint32 i = 0; i < fSourceCount; i++) {
		uint8* data = (uint8*)_SourceToVirtual(fSourceVecs[i].base);
		size_t vecLength = min_c(fSourceVecs[i].length, length);

		for (uint32 j = 0; j < vecLength; j++) {
			if (disk[offset + j] != data[j]) {
				dprintf("offset %lu differs, found on disk:\n", j);
				j &= ~63;
				dump_block((char*)&disk[offset + j],
					min_c(64, fSuite.Size() - i), "  ");
				dprintf("should be:\n");
				dump_block((char*)&data[j], min_c(64, fSuite.Size() - j), "  ");

				_Panic("Data write differs");
			}
		}

		offset += vecLength;
		length -= vecLength;
	}
}


void
Test::_CheckResults()
{
	if (fIsWrite)
		_CheckWrite();
	else
		_CheckCompare();
}


status_t
Test::_DoIO(IOOperation& operation)
{
	return do_io(NULL, &operation);
}


void
Test::Run(DMAResource& resource)
{
	_Prepare();

	IORequest request;
	status_t status = request.Init(fOffset, fSourceVecs, fSourceCount,
		fLength, fIsWrite, fFlags);
	if (status != B_OK)
		_Panic("request init failed: %s\n", strerror(status));

	uint32 resultIndex = 0;

	IOOperation operation;
	while (request.RemainingBytes() > 0) {
		if (resultIndex >= fResultCount)
			_Panic("no results left");

		status_t status = resource.TranslateNext(&request, &operation, 0);
		if (status != B_OK) {
			_Panic("DMAResource::TranslateNext() failed: %s\n",
				strerror(status));
			break;
		}

		DMABuffer* buffer = operation.Buffer();

		dprintf("IOOperation: offset %" B_PRIdOFF ", length %" B_PRIuGENADDR
			" (%" B_PRIdOFF "/%" B_PRIuGENADDR ")\n", operation.Offset(),
			operation.Length(), operation.OriginalOffset(),
			operation.OriginalLength());
		dprintf("  DMABuffer %p, %lu vecs, bounce buffer: %p (%p) %s\n", buffer,
			buffer->VecCount(), buffer->BounceBufferAddress(),
			(void*)buffer->PhysicalBounceBufferAddress(),
			operation.UsesBounceBuffer() ? "used" : "unused");
		for (uint32 i = 0; i < buffer->VecCount(); i++) {
			dprintf("    [%" B_PRIu32 "] base %#" B_PRIxGENADDR ", length %"
				B_PRIuGENADDR "%s\n", i, buffer->VecAt(i).base,
				buffer->VecAt(i).length,
				buffer->UsesBounceBufferAt(i) ? ", bounce" : "");
		}

		dprintf("  remaining bytes: %" B_PRIuGENADDR "\n",
			request.RemainingBytes());

		// check results

		const result_t& result = fResults[resultIndex];
		if (result.count != buffer->VecCount())
			panic("result count differs (expected %lu)\n", result.count);

		for (uint32 i = 0; i < result.count; i++) {
			const target_t& target = result.targets[i];
			const generic_io_vec& vec = buffer->VecAt(i);

			if (target.length != vec.length)
				_Panic("[%lu] length differs", i);

			generic_addr_t address;
			if (target.uses_bounce_buffer) {
				address = target.address
					+ (addr_t)buffer->PhysicalBounceBufferAddress();
			} else if (fSuite.IsContiguous() || target.address < B_PAGE_SIZE) {
				address = target.address + fSuite.PhysicalDataBase();
			} else {
				address = target.address - B_PAGE_SIZE
					+ fSuite.SecondPhysicalDataBase();
			}

			if (address != vec.base) {
				_Panic("[%" B_PRIu32 "] address differs: %#" B_PRIxGENADDR
					", should be %#" B_PRIuGENADDR "", i, vec.base, address);
			}
		}

		_DoIO(operation);
		operation.SetStatus(B_OK);
		bool finished = operation.Finish();
		bool isPartial = result.partial_begin || result.partial_end;
		if (finished == (isPartial && fIsWrite))
			_Panic("partial finished %s", finished ? "early" : "late");

		if (!finished) {
			dprintf("  operation not done yet!\n");
			_DoIO(operation);
			operation.SetStatus(B_OK);

			isPartial = result.partial_begin && result.partial_end;
			finished = operation.Finish();
			if (finished == result.partial_begin && result.partial_end)
				_Panic("partial finished %s", finished ? "early" : "late");

			if (!finished) {
				dprintf("  operation not done yet!\n");
				_DoIO(operation);
				operation.SetStatus(B_OK);

				if (!operation.Finish())
					_Panic("operation doesn't finish");
			}
		}

		request.OperationFinished(&operation, operation.Status(),
			false,
			operation.OriginalOffset() - operation.Parent()->Offset()
				+ operation.OriginalLength());

		resultIndex++;
	}

	_CheckResults();
}


void
Test::_Panic(const char* message,...)
{
	char buffer[1024];

	va_list args;
	va_start(args, message);
	vsnprintf(buffer, sizeof(buffer), message, args);
	va_end(args);

	dprintf("test failed\n");
	dprintf("  offset:  %lld\n", fOffset);
	dprintf("  base:    %p (physical: %p)\n", (void*)fSuite.DataBase(),
		(void*)fSuite.PhysicalDataBase());
	dprintf("  length:  %lu\n", fLength);
	dprintf("  write:   %d\n", fIsWrite);
	dprintf("  flags:   %#lx\n", fFlags);
	dprintf("  sources:\n");
	for (uint32 i = 0; i < fSourceCount; i++) {
		dprintf("    [%#" B_PRIxGENADDR ", %" B_PRIuGENADDR "]\n",
			fSourceVecs[i].base, fSourceVecs[i].length);
	}
	for (uint32 i = 0; i < fResultCount; i++) {
		const result_t& result = fResults[i];
		dprintf("  result %lu:\n", i);
		dprintf("    offset:  %lld\n", result.offset);
		dprintf("    partial: %d/%d\n", result.partial_begin,
			result.partial_end);

		for (uint32 k = 0; k < result.count; k++) {
			const target_t& target = result.targets[k];
			dprintf("    [%p, %lu, %d]\n", (void*)target.address, target.length,
				target.uses_bounce_buffer);
		}
	}

	panic("%s", buffer);
}


static void
run_tests_no_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		0,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "no restrictions", restrictions, 512);

	suite.AddTest(0, 1024, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false);

	// read partial begin/end
	suite.AddTest(23, 1024, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 23, true)
			.AddTarget(0, 1024, false)
			.AddTarget(23, 512 - 23, true);

	// read less than a block
	suite.AddTest(23, 30, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 23, true)
			.AddTarget(0, 30, false)
			.AddTarget(23, 512 - 53, true);

	// write begin/end
	suite.AddTest(23, 1024, true, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 512, true)
			.AddTarget(489, 512, false)
			.AddTarget(512, 512, true);

	// read partial end, length < iovec length
	suite.AddTest(0, 1028, false, 0)
		.AddSource(0, 512)
		.AddSource(1024, 1024)
		.NextResult(0, false, true)
			.AddTarget(0, 512, false)
			.AddTarget(1024, 516, false)
			.AddTarget(0, 508, true);

	// write partial end, length < iovec length
	suite.AddTest(0, 1028, true, 0)
		.AddSource(0, 512)
		.AddSource(1024, 1024)
		.NextResult(0, false, true)
			.AddTarget(0, 512, false)
			.AddTarget(1024, 512, false)
			.AddTarget(0, 512, true);

	suite.Run();
}


static void
run_tests_address_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		context.PhysicalDataBase() + 512,	// low
		0,		// high
		0,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "address", restrictions, 512);

	suite.AddTest(0, 1024, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 512, true)
			.AddTarget(512, 512, false);

	suite.Run();
}


static void
run_tests_alignment_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		32,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "alignment", restrictions, 512);

	suite.AddTest(0, 1024, false, B_PHYSICAL_IO_REQUEST)
		.AddSource(16, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, true);

	suite.AddTest(0, 2559, true, B_PHYSICAL_IO_REQUEST)
		.AddSource(0, 2559)
		.NextResult(0, false, true)
			.AddTarget(0, 2048, false)
			.AddTarget(0, 512, true);

	suite.AddTest(0, 51, true, B_PHYSICAL_IO_REQUEST)
		.AddSource(0, 4096)
		.NextResult(0, false, true)
			.AddTarget(0, 512, true);

	suite.AddTest(32, 51, true, B_PHYSICAL_IO_REQUEST)
		.AddSource(0, 4096)
		.NextResult(0, true, false)
			.AddTarget(0, 512, true);
			// Note: The operation has actually both partial begin and end, but
			// our Test is not clever enough to realize that it is only one
			// block and thus only one read phase is needed.

	suite.Run();
}


static void
run_tests_boundary_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		0,		// alignment
		1024,	// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "boundary", restrictions, 512);

	suite.AddTest(0, 2000, false, 0)
		.AddSource(0, 2048)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false)
			.AddTarget(1024, 976, false)
			.AddTarget(0, 48, true);

	suite.Run();
}


static void
run_tests_segment_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		0,		// alignment
		0,		// boundary
		0,		// max transfer
		4,		// max segment count
		1024,	// max segment size
		0		// flags
	};

	TestSuite suite(context, "segment", restrictions, 512);

	suite.AddTest(0, 4096, false, 0)
		.AddSource(0, 4096)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false)
			.AddTarget(1024, 1024, false)
			.AddTarget(2048, 1024, false)
			.AddTarget(3072, 1024, false);

	suite.AddTest(0, 2560, false, B_PHYSICAL_IO_REQUEST)
		.AddSource(0, 512)
		.AddSource(1024, 512)
		.AddSource(2048, 512)
		.AddSource(3072, 512)
		.AddSource(4096, 512)
		.NextResult(0, false, false)
			.AddTarget(0, 512, false)
			.AddTarget(1024, 512, false)
			.AddTarget(2048, 512, false)
			.AddTarget(3072, 512, false)
		.NextResult(2048, false, false)
			.AddTarget(4096, 512, false);

	suite.Run();
}


static void
run_tests_transfer_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		0,		// alignment
		0,		// boundary
		1024,	// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "transfer", restrictions, 512);

	suite.AddTest(0, 4000, false, 0)
		.AddSource(0, 4096)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false)
		.NextResult(0, false, false)
			.AddTarget(1024, 1024, false)
		.NextResult(0, false, false)
			.AddTarget(2048, 1024, false)
		.NextResult(0, false, false)
			.AddTarget(3072, 1024 - 96, false)
			.AddTarget(0, 96, true);

	suite.Run();
}


static void
run_tests_interesting_restrictions(TestSuiteContext& context)
{
	dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		32,		// alignment
		512,	// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "interesting", restrictions, 512);

	// read with partial begin/end
	suite.AddTest(32, 1000, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 32, true)
			.AddTarget(0, 512, false)
			.AddTarget(512, 480, false)
			.AddTarget(32, 480, true)
			.AddTarget(512, 32, true);

	// write with partial begin/end
	suite.AddTest(32, 1000, true, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 512, true)
			.AddTarget(480, 32, false)
			.AddTarget(512, 480, false)
			.AddTarget(512, 512, true);

	suite.Run();

	restrictions = (dma_restrictions){
		0x0,	// low
		0x0,	// high
		32,		// alignment
		512,	// boundary
		0,		// max transfer
		4,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite2(context, "interesting2", restrictions, 512);

	suite2.AddTest(32, 1000, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, true, false)
			.AddTarget(0, 32, true)
			.AddTarget(0, 512, false)
			.AddTarget(512, 480, false)
		.NextResult(0, false, true)
			.AddTarget(0, 512, true);

	suite2.Run();
}


static void
run_tests_mean_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		context.PhysicalDataBase() + 1024,	// low
		0x0,	// high
		32,		// alignment
		1024,	// boundary
		0,		// max transfer
		2,		// max segment count
		512,	// max segment size
		0		// flags
	};

	TestSuite suite(context, "mean", restrictions, 512);

	suite.AddTest(0, 1024, false, 0)
		.AddSource(0, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 512, true)
			.AddTarget(512, 512, true);

	suite.AddTest(0, 1024, false, 0)
		.AddSource(1024 + 32, 1024)
		.NextResult(0, false, false)
			.AddTarget(1024 + 32, 512, false)
		.NextResult(0, false, false)
			.AddTarget(1568, 480, false)
			.AddTarget(1568 + 480, 32, false);

	suite.Run();
}


static void
run_tests_non_contiguous_no_restrictions(TestSuiteContext& context)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		0,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite(context, "non contiguous, no restrictions", restrictions,
		512);

	suite.AddTest(0, 8192, false, 0)
		.AddSource(0, 8192)
		.NextResult(0, false, false)
			.AddTarget(0, 4096, false)
			.AddTarget(4096, 4096, false);

	suite.Run();
}


static void
run_test()
{
	TestSuiteContext context;
	status_t status = context.Init(4 * B_PAGE_SIZE);
	if (status != B_OK)
		return;

	run_tests_no_restrictions(context);
	run_tests_address_restrictions(context);
	run_tests_alignment_restrictions(context);
	run_tests_boundary_restrictions(context);
	run_tests_segment_restrictions(context);
	run_tests_transfer_restrictions(context);
	run_tests_interesting_restrictions(context);
	run_tests_mean_restrictions(context);

	// physical non-contiguous

	status = context.Init(2 * B_PAGE_SIZE, false);
	if (status != B_OK)
		return;

	run_tests_non_contiguous_no_restrictions(context);

	dprintf("All tests passed!\n");
}


//	#pragma mark - driver


static float
dma_test_supports_device(device_node *parent)
{
	const char* bus = NULL;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			== B_OK && !strcmp(bus, "generic"))
		return 0.8;

	return -1;
}


static status_t
dma_test_register_device(device_node *parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "DMA Test"}},
		{NULL}
	};

	return sDeviceManager->register_node(parent,
		"drivers/disk/dma_resource_test/driver_v1", attrs, NULL, NULL);
}


static status_t
dma_test_init_driver(device_node *node, void **_driverCookie)
{
	sAreaSize = 10 * 1024 * 1024;
	sArea = create_area("dma test", &sAreaAddress, B_ANY_KERNEL_ADDRESS,
		sAreaSize, B_LAZY_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sArea < B_OK)
		return sArea;

	*_driverCookie = node;

	run_test();
	return B_OK;
}


static void
dma_test_uninit_driver(void *driverCookie)
{
	delete_area(sArea);
}


static status_t
dma_test_register_child_devices(void *driverCookie)
{
	return sDeviceManager->publish_device((device_node*)driverCookie,
		"disk/virtual/dma_test/raw",
		"drivers/disk/dma_resource_test/device_v1");
}


//	#pragma mark - device


static status_t
dma_test_init_device(void *driverCookie, void **_deviceCookie)
{
	const dma_restrictions restrictions = {
		0x0,	// low
		0x0,	// high
		4,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		B_PAGE_SIZE, // max segment size
		0		// flags
	};

	*_deviceCookie = driverCookie;
	sDMAResource = new(std::nothrow) DMAResource;
	if (sDMAResource == NULL)
		return B_NO_MEMORY;

	status_t status = sDMAResource->Init(restrictions, DMA_TEST_BLOCK_SIZE,
		DMA_TEST_BUFFER_COUNT, DMA_TEST_BOUNCE_BUFFER_COUNT);
	if (status != B_OK) {
		delete sDMAResource;
		return status;
	}

	sIOScheduler = new(std::nothrow) IOSchedulerSimple(sDMAResource);
	if (sIOScheduler == NULL) {
		delete sDMAResource;
		return B_NO_MEMORY;
	}

	status = sIOScheduler->Init("dma test scheduler");
	if (status != B_OK) {
		delete sIOScheduler;
		delete sDMAResource;
		return status;
	}

	sIOScheduler->SetCallback(&do_io, NULL);
	return B_OK;
}


static void
dma_test_uninit_device(void *deviceCookie)
{
}


static status_t
dma_test_open(void *deviceCookie, const char *path, int openMode,
	void **_cookie)
{
	return B_OK;
}


static status_t
dma_test_close(void *cookie)
{
	return B_OK;
}


static status_t
dma_test_free(void *cookie)
{
	return B_OK;
}


static status_t
dma_test_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	size_t length = *_length;

	if (pos >= sAreaSize)
		return B_BAD_VALUE;
	if (pos + length > sAreaSize)
		length = sAreaSize - pos;

#if 1
	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, false, 0);
	if (status != B_OK)
		return status;

	status = sIOScheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	dprintf("dma_test_read(): request.Wait() returned: %s\n", strerror(status));
#else
	status_t status = user_memcpy(buffer, (uint8*)sAreaAddress + pos, length);
#endif

	if (status == B_OK)
		*_length = length;
	return status;
}


static status_t
dma_test_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	size_t length = *_length;

	if (pos >= sAreaSize)
		return B_BAD_VALUE;
	if (pos + length > sAreaSize)
		length = sAreaSize - pos;

#if 1
	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, true, 0);
	if (status != B_OK)
		return status;

	status = sIOScheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	dprintf("dma_test_write(): request.Wait() returned: %s\n",
		strerror(status));
#else
	status_t status = user_memcpy((uint8*)sAreaAddress + pos, buffer, length);
#endif

	if (status == B_OK)
		*_length = length;

	return status;
}


static status_t
dma_test_io(void *cookie, io_request *request)
{
	dprintf("dma_test_io(%p)\n", request);

	return sIOScheduler->ScheduleRequest(request);
}


static status_t
dma_test_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case B_GET_DEVICE_SIZE:
			return user_memcpy(buffer, &sAreaSize, sizeof(size_t));

		case B_SET_NONBLOCKING_IO:
		case B_SET_BLOCKING_IO:
			return B_OK;

		case B_GET_READ_STATUS:
		case B_GET_WRITE_STATUS:
		{
			bool value = true;
			return user_memcpy(buffer, &value, sizeof(bool));
		}

		case B_GET_GEOMETRY:
		case B_GET_BIOS_GEOMETRY:
		{
			device_geometry geometry;
			geometry.bytes_per_sector = DMA_TEST_BLOCK_SIZE;
			geometry.sectors_per_track = 1;
			geometry.cylinder_count = sAreaSize / DMA_TEST_BLOCK_SIZE;
			geometry.head_count = 1;
			geometry.device_type = B_DISK;
			geometry.removable = true;
			geometry.read_only = false;
			geometry.write_once = false;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_MEDIA_STATUS:
		{
			status_t status = B_OK;
			return user_memcpy(buffer, &status, sizeof(status_t));
		}

		case B_SET_UNINTERRUPTABLE_IO:
		case B_SET_INTERRUPTABLE_IO:
		case B_FLUSH_DRIVE_CACHE:
			return B_OK;
	}
	return B_BAD_VALUE;
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager},
	{}
};


static const struct driver_module_info sDMATestDriverModule = {
	{
		"drivers/disk/dma_resource_test/driver_v1",
		0,
		NULL
	},

	dma_test_supports_device,
	dma_test_register_device,
	dma_test_init_driver,
	dma_test_uninit_driver,
	dma_test_register_child_devices
};

static const struct device_module_info sDMATestDeviceModule = {
	{
		"drivers/disk/dma_resource_test/device_v1",
		0,
		NULL
	},

	dma_test_init_device,
	dma_test_uninit_device,
	NULL,

	dma_test_open,
	dma_test_close,
	dma_test_free,

	dma_test_read,
	dma_test_write,
	dma_test_io,

	dma_test_control,

	NULL,	// select
	NULL	// deselect
};

const module_info* modules[] = {
	(module_info*)&sDMATestDriverModule,
	(module_info*)&sDMATestDeviceModule,
	NULL
};
