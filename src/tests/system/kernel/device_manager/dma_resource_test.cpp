/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <device_manager.h>

#include "dma_resources.h"
#include "io_requests.h"


#define DMA_TEST_BLOCK_SIZE	512


struct device_manager_info* sDeviceManager;

static area_id sArea;
static size_t sAreaSize;
static void* sAreaAddress;
static DMAResource* sDMAResource;


class Test : public DoublyLinkedListLinkImpl<Test> {
public:
							Test(off_t offset, uint8* base, uint8* physicalBase,
								size_t length, bool isWrite, uint32 flags);

			Test&			AddSource(addr_t base, size_t length);
			Test&			NextResult(off_t offset, bool partialBegin,
								bool partialEnd);
			Test&			AddTarget(addr_t base, size_t length,
								bool usesBounceBuffer);

			void			Run(DMAResource& resource);

private:
			void			_Panic(const char* message,...);

			off_t			fOffset;
			uint8*			fBase;
			uint8*			fPhysicalBase;
			size_t			fLength;
			bool			fIsWrite;
			uint32			fFlags;
			iovec			fSourceVecs[32];
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
	TestSuite(const char* name, const dma_restrictions& restrictions,
			size_t blockSize, uint8* base, uint8* physicalBase)
		:
		fBase(base),
		fPhysicalBase(physicalBase)
	{
		dprintf("----- Run \"%s\" tests ---------------------------\n", name);
		dprintf("  DMA restrictions: address %#lx - %#lx, align %lu, boundary "
			"%lu,\n    max transfer %lu, max segs %lu, max seg size %lu, "
			"flags %lx\n\n", restrictions.low_address,
			restrictions.high_address, restrictions.alignment,
			restrictions.boundary, restrictions.max_transfer_size,
			restrictions.max_segment_count, restrictions.max_segment_size,
			restrictions.flags);

		status_t status = fDMAResource.Init(restrictions, blockSize, 10);
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
		Test* test = new(std::nothrow) Test(offset, fBase, fPhysicalBase,
			length, isWrite, flags);
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

private:
	DMAResource		fDMAResource;
	uint8*			fBase;
	uint8*			fPhysicalBase;
	TestList		fTests;
};


Test::Test(off_t offset, uint8* base, uint8* physicalBase, size_t length,
		bool isWrite, uint32 flags)
	:
	fOffset(offset),
	fBase(base),
	fPhysicalBase(physicalBase),
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
	fSourceVecs[fSourceCount].iov_base
		= (void*)(((fFlags & B_PHYSICAL_IO_REQUEST) == 0
			? fBase : fPhysicalBase) + address);
	fSourceVecs[fSourceCount].iov_len = length;
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


void
Test::Run(DMAResource& resource)
{
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

		status_t status = resource.TranslateNext(&request, &operation);
		if (status != B_OK) {
			_Panic("DMAResource::TranslateNext() failed: %s\n",
				strerror(status));
			break;
		}

		DMABuffer* buffer = operation.Buffer();

		dprintf("IOOperation: offset %Ld, length %lu (%Ld/%lu)\n",
			operation.Offset(), operation.Length(), operation.OriginalOffset(),
			operation.OriginalLength());
		dprintf("  DMABuffer %p, %lu vecs, bounce buffer: %p (%p) %s\n", buffer,
			buffer->VecCount(), buffer->BounceBuffer(),
			(void*)buffer->PhysicalBounceBuffer(),
			buffer->UsesBounceBuffer() ? "used" : "unused");
		for (uint32 i = 0; i < buffer->VecCount(); i++) {
			dprintf("    [%lu] base %p, length %lu\n", i,
				buffer->VecAt(i).iov_base, buffer->VecAt(i).iov_len);
		}

		dprintf("  remaining bytes: %lu\n", request.RemainingBytes());

		// check results

		const result_t& result = fResults[resultIndex];
		if (result.count != buffer->VecCount())
			panic("result count differs (expected %lu)\n", result.count);

		for (uint32 i = 0; i < result.count; i++) {
			const target_t& target = result.targets[i];
			const iovec& vec = buffer->VecAt(i);

			if (target.length != vec.iov_len)
				_Panic("[%lu] length differs", i);

			void* address;
			if (target.uses_bounce_buffer) {
				address = (void*)(target.address
					+ (addr_t)buffer->PhysicalBounceBuffer());
			} else
				address = (void*)(target.address + fPhysicalBase);

			if (address != vec.iov_base) {
				_Panic("[%lu] address differs: %p, should be %p", i,
					vec.iov_base, address);
			}
		}

		operation.SetStatus(B_OK);
		bool finished = operation.Finish();
		bool isPartial = result.partial_begin || result.partial_end;
		if (finished == (isPartial && fIsWrite))
			_Panic("partial finished %s", finished ? "early" : "late");

		if (!finished) {
			dprintf("  operation not done yet!\n");
			operation.SetStatus(B_OK);

			isPartial = result.partial_begin && result.partial_end;
			finished = operation.Finish();
			if (finished == result.partial_begin && result.partial_end)
				_Panic("partial finished %s", finished ? "early" : "late");

			if (!finished) {
				dprintf("  operation not done yet!\n");
				operation.SetStatus(B_OK);

				if (!operation.Finish())
					_Panic("operation doesn't finish");
			}
		}

		resultIndex++;
	}
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
	dprintf("  base:    %p (physical: %p)\n", fBase, fPhysicalBase);
	dprintf("  length:  %lu\n", fLength);
	dprintf("  write:   %d\n", fIsWrite);
	dprintf("  flags:   %#lx\n", fFlags);
	dprintf("  sources:\n");
	for (uint32 i = 0; i < fSourceCount; i++) {
		dprintf("    [%p, %lu]\n", fSourceVecs[i].iov_base,
			fSourceVecs[i].iov_len);
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
run_tests_no_restrictions(uint8* address, uint8* physicalAddress, size_t size)
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

	TestSuite suite("no restrictions", restrictions, 512, address,
		physicalAddress);

	suite.AddTest(0, 1024, false, B_USER_IO_REQUEST)
		.AddSource(0, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false);
	suite.AddTest(23, 1024, true, B_USER_IO_REQUEST)
		.AddSource(0, 1024)
		.NextResult(0, true, true)
			.AddTarget(0, 23, true)
			.AddTarget(0, 1024, false)
			.AddTarget(23, 512 - 23, true)
			;
	suite.AddTest(0, 1028, true, B_USER_IO_REQUEST)
		.AddSource(0, 512)
		.AddSource(1024, 516)
		.NextResult(0, false, true)
			.AddTarget(0, 512, false)
			.AddTarget(1024, 516, false)
			.AddTarget(0, 508, true);

	suite.Run();
}


static void
run_tests_address_restrictions(uint8* address, uint8* physicalAddress,
	size_t size)
{
	const dma_restrictions restrictions = {
		(addr_t)physicalAddress + 512,	// low
		0,		// high
		0,		// alignment
		0,		// boundary
		0,		// max transfer
		0,		// max segment count
		0,		// max segment size
		0		// flags
	};

	TestSuite suite("address", restrictions, 512, address, physicalAddress);

	suite.AddTest(0, 1024, false, B_USER_IO_REQUEST)
		.AddSource(0, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 512, true)
			.AddTarget(512, 512, false);

	suite.Run();
}


static void
run_tests_alignment_restrictions(uint8* address, uint8* physicalAddress,
	size_t size)
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

	TestSuite suite("alignment", restrictions, 512, address, physicalAddress);

	suite.AddTest(0, 1024, false, B_PHYSICAL_IO_REQUEST)
		.AddSource(16, 1024)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, true);

	suite.Run();
}


static void
run_tests_boundary_restrictions(uint8* address, uint8* physicalAddress,
	size_t size)
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

	TestSuite suite("boundary", restrictions, 512, address, physicalAddress);

	suite.AddTest(0, 2000, false, B_USER_IO_REQUEST)
		.AddSource(0, 2048)
		.NextResult(0, false, false)
			.AddTarget(0, 1024, false)
			.AddTarget(1024, 976, false)
			.AddTarget(0, 48, true);

	suite.Run();
}


static void
run_tests_segment_restrictions(uint8* address, uint8* physicalAddress,
	size_t size)
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

	TestSuite suite("segment", restrictions, 512, address, physicalAddress);

#if 0
	suite.AddTest(0, 1024, false, B_USER_IO_REQUEST)
		.AddSource(0, 1024)
		.NextResult(0, false)
			.AddTarget(0, 1024, false);
#endif

	suite.Run();
}


static void
run_tests_mean_restrictions(uint8* address, uint8* physicalAddress, size_t size)
{
	const dma_restrictions restrictions = {
		(addr_t)physicalAddress + 1024,	// low
		0x0,	// high
		32,		// alignment
		512,	// boundary
		2048,	// max transfer
		2,		// max segment count
		1024,	// max segment size
		0		// flags
	};

	TestSuite suite("mean", restrictions, 512, address, physicalAddress);

#if 0
	suite.AddTest(0, 1024, false, B_USER_IO_REQUEST)
		.AddSource(0, 1024)
		.NextResult(0, false)
			.AddTarget(0, 1024, false);
#endif

	suite.Run();
}


static void
run_test()
{
	size_t size = 1 * 1024 * 1024;
	uint8* address;
	area_id area = create_area("dma source", (void**)&address,
		B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return;

	physical_entry entry;
	get_memory_map(address, size, &entry, 1);

	dprintf("DMA Test area %p, physical %p\n", address, entry.address);

	run_tests_no_restrictions(address, (uint8*)entry.address, size);
	run_tests_address_restrictions(address, (uint8*)entry.address, size);
	run_tests_alignment_restrictions(address, (uint8*)entry.address, size);
	run_tests_boundary_restrictions(address, (uint8*)entry.address, size);
	run_tests_segment_restrictions(address, (uint8*)entry.address, size);
	run_tests_mean_restrictions(address, (uint8*)entry.address, size);

	delete_area(area);
	panic("done.");
}


//	#pragma mark - driver


float
dma_test_supports_device(device_node *parent)
{
	const char* bus = NULL;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			== B_OK && !strcmp(bus, "generic"))
		return 0.8;

	return -1;
}


status_t
dma_test_register_device(device_node *parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "DMA Test"}},
		{NULL}
	};

	return sDeviceManager->register_node(parent,
		"drivers/disk/dma_test/driver_v1", attrs, NULL, NULL);
}


status_t
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


void
dma_test_uninit_driver(void *driverCookie)
{
	delete_area(sArea);
}


status_t
dma_test_register_child_devices(void *driverCookie)
{
	return sDeviceManager->publish_device((device_node*)driverCookie,
		"disk/virtual/dma_test/raw", "drivers/disk/dma_test/device_v1");
}


//	#pragma mark - device


status_t
dma_test_init_device(void *driverCookie, void **_deviceCookie)
{
	*_deviceCookie = driverCookie;
	return B_OK;
}


void
dma_test_uninit_device(void *deviceCookie)
{
}


status_t
dma_test_open(void *deviceCookie, const char *path, int openMode,
	void **_cookie)
{
	return B_OK;
}


status_t
dma_test_close(void *cookie)
{
	return B_OK;
}


status_t
dma_test_free(void *cookie)
{
	return B_OK;
}


status_t
dma_test_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	size_t length = *_length;

	if (pos >= sAreaSize)
		return B_BAD_VALUE;
	if (pos + length > sAreaSize)
		length = sAreaSize - pos;

	status_t status = user_memcpy(buffer, (uint8*)sAreaAddress + pos, length);

	if (status == B_OK)
		*_length = length;

	return status;
}


status_t
dma_test_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	size_t length = *_length;

	if (pos >= sAreaSize)
		return B_BAD_VALUE;
	if (pos + length > sAreaSize)
		length = sAreaSize - pos;

	status_t status = user_memcpy((uint8*)sAreaAddress + pos, buffer, length);

	if (status == B_OK)
		*_length = length;

	return status;
}


status_t
dma_test_io(void *cookie, io_request *request)
{
	return B_BAD_VALUE;
}


status_t
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


const static struct driver_module_info sDMATestDriverModule = {
	{
		"drivers/disk/dma_test/driver_v1",
		0,
		NULL
	},

	dma_test_supports_device,
	dma_test_register_device,
	dma_test_init_driver,
	dma_test_uninit_driver,
	dma_test_register_child_devices
};

const static struct device_module_info sDMATestDeviceModule = {
	{
		"drivers/disk/dma_test/device_v1",
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
	NULL,	// io

	dma_test_control,

	NULL,	// select
	NULL	// deselect
};

const module_info* modules[] = {
	(module_info*)&sDMATestDriverModule,
	(module_info*)&sDMATestDeviceModule,
	NULL
};
