/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <arch/debug.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <elf.h>
#include <kimage.h>
#include <arch/generic/user_memory.h>
#include <AutoDeleterDrivers.h>


kernel_args *sKernelArgs;
bool sInitCalled = false;


static void
WriteImage(preloaded_image* _image)
{
	preloaded_elf64_image* image = (preloaded_elf64_image*)_image;
	dprintf("image %p\n", image);
	dprintf("image \"%s\"\n", (char*)image->name);
	dprintf("  text: 0x%" B_PRIxADDR " - 0x%" B_PRIxADDR ",%"
		B_PRIdSSIZE "\n", image->text_region.start,
		image->text_region.start + image->text_region.size,
		image->text_region.delta);
	dprintf("  data: 0x%" B_PRIxADDR " - 0x%" B_PRIxADDR ", %"
		B_PRIdSSIZE "\n", image->data_region.start,
		image->data_region.start + image->data_region.size,
		image->data_region.delta);
}


static void
WriteImages()
{
	WriteImage(sKernelArgs->kernel_image);

	preloaded_image* image = sKernelArgs->preloaded_images;
	while (image != NULL) {
		WriteImage(image);
		image = image->next;
	}
}


static bool
AddressInImage(preloaded_image* _image, addr_t adr)
{
	preloaded_elf64_image* image = (preloaded_elf64_image*)_image;
	if (adr >= image->text_region.start
			&& adr < image->text_region.start + image->text_region.size) {
		return true;
	}

	if (adr >= image->data_region.start
			&& adr < image->data_region.start + image->data_region.size) {
		return true;
	}

	return false;
}


static bool
SymbolAt(preloaded_image* _image, addr_t adr, const char **name, ssize_t *ofs)
{
	preloaded_elf64_image* image = (preloaded_elf64_image*)_image;
	adr -= image->text_region.delta;
	for (uint32 i = 0; i < image->num_debug_symbols; i++) {
		Elf64_Sym& sym = image->debug_symbols[i];
		if (sym.st_shndx != STN_UNDEF && adr >= sym.st_value
			&& adr < sym.st_value + sym.st_size) {
			if (name != NULL)
				*name = &image->debug_string_table[sym.st_name];
			if (ofs != NULL)
				*ofs = adr - sym.st_value;
			return true;
		}
	}
	return false;
}


static preloaded_image*
FindImage(addr_t adr)
{
	if (AddressInImage(sKernelArgs->kernel_image, adr))
		return sKernelArgs->kernel_image;

	preloaded_image* image = sKernelArgs->preloaded_images;
	while (image != NULL) {
		if (AddressInImage(image, adr))
			return image;
		image = image->next;
	}

	return NULL;
}


static VMArea*
FindArea(addr_t adr)
{
	if (IS_KERNEL_ADDRESS(adr)) {
		VMAddressSpacePutter addrSpace(VMAddressSpace::GetKernel());
		return addrSpace->LookupArea(adr);
	}
	if (IS_USER_ADDRESS(adr)) {
		VMAddressSpacePutter addrSpace(VMAddressSpace::GetCurrent());
		return addrSpace->LookupArea(adr);
	}
	return NULL;
}


static status_t
lookup_symbol(Thread* thread, addr_t address, addr_t* _baseAddress,
	const char** _symbolName, const char** _imageName, bool* _exactMatch)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (IS_KERNEL_ADDRESS(address)) {
		// a kernel symbol
		status = elf_debug_lookup_symbol_address(address, _baseAddress,
			_symbolName, _imageName, _exactMatch);
	} else if (true && thread != NULL && thread->team != NULL) {
		// try a lookup using the userland runtime loader structures
		status = elf_debug_lookup_user_symbol_address(thread->team, address,
			_baseAddress, _symbolName, _imageName, _exactMatch);

		if (status != B_OK) {
			// try to locate the image in the images loaded into user space
			status = image_debug_lookup_user_symbol_address(thread->team,
				address, _baseAddress, _symbolName, _imageName, _exactMatch);
		}
	}

	return status;
}


void
WritePCBoot(addr_t pc)
{
	preloaded_image* image = FindImage(pc);
	if (image != NULL) {
		const char *name;
		ssize_t ofs;
		if (SymbolAt(image, pc, &name, &ofs)) {
			dprintf("<%s> %s + %" B_PRIdSSIZE, (char*)image->name, name, ofs);
			return;
		}
		dprintf("<%s> 0x%" B_PRIxADDR, (char*)image->name,
			pc - ((preloaded_elf64_image*)image)->text_region.delta);
		return;
	}
/*
	VMArea* area = FindArea(pc);
	if (area != NULL) {
		dprintf("<%s> 0x%" B_PRIxADDR, area->name, pc - area->Base());
		return;
	}
*/
	dprintf("0x%" B_PRIxADDR, pc);
}


void
WritePC(addr_t pc)
{
	dprintf("0x%" B_PRIxADDR " ", pc);
	if (!sInitCalled) {
		WritePCBoot(pc); return;
	}

	addr_t baseAddress;
	const char* symbolName;
	const char* imageName;
	bool exactMatch;
	if (lookup_symbol(thread_get_current_thread(), pc, &baseAddress,
		&symbolName, &imageName, &exactMatch) >= B_OK) {
		if (symbolName != NULL) {
			dprintf("<%s> %s + %" B_PRIdSSIZE, imageName, symbolName,
				pc - baseAddress);
			return;
		}
		dprintf("<%s> 0x%" B_PRIxADDR, imageName, pc - baseAddress);
		return;
	}

	VMArea* area = FindArea(pc);
	if (area != NULL) {
		dprintf("<%s> 0x%" B_PRIxADDR, area->name, pc - area->Base());
		return;
	}

	dprintf("0x%" B_PRIxADDR, pc);
}


static void
DumpMemory(uint64* adr, size_t len)
{
	while (len > 0) {
		if ((addr_t)adr % 0x10 == 0)
			dprintf("%08" B_PRIxADDR " ", (addr_t)adr);
		uint64 val;
		if (user_memcpy(&val, adr++, sizeof(val)) < B_OK) {
			dprintf(" ????????????????");
		} else {
			dprintf(" %016" B_PRIx64, val);
		}
		if ((addr_t)adr % 0x10 == 0)
			dprintf("\n");
		len -= 8;
	}
	if ((addr_t)adr % 0x10 != 0)
		dprintf("\n");
}


void
DoStackTrace(addr_t fp, addr_t pc)
{
	dprintf("Stack:\n");
	dprintf("FP: 0x%" B_PRIxADDR, fp);
	if (pc != 0) {
		dprintf(", PC: "); WritePC(pc);
	}
	dprintf("\n");
	addr_t oldFp = fp;
	while (fp != 0) {
		if ((pc >= (addr_t)&strcpy && pc < (addr_t)&strcpy + 32)
				|| (pc >= (addr_t)&memset && pc < (addr_t)&memset + 34)) {
			if (user_memcpy(&fp, (uint64*)fp - 1, sizeof(pc)) < B_OK)
				break;
			pc = 0;
		} else {
			if (user_memcpy(&pc, (uint64*)fp - 1, sizeof(pc)) < B_OK)
				break;
			if (user_memcpy(&fp, (uint64*)fp - 2, sizeof(pc)) < B_OK)
				break;
		}
		dprintf("FP: 0x%" B_PRIxADDR, fp);
		dprintf(", PC: "); WritePC((pc == 0) ? 0 : pc - 1);
		dprintf("\n");
/*
		if (IS_KERNEL_ADDRESS(oldFp) && IS_KERNEL_ADDRESS(fp))
			DumpMemory((uint64*)oldFp, (addr_t)fp - (addr_t)oldFp);
*/
		oldFp = fp;
	}
}


static int
stack_trace(int argc, char **argv)
{
	if (argc >= 2) {
		thread_id id = strtoul(argv[1], NULL, 0);
		Thread* thread = Thread::GetDebug(id);
		if (thread == NULL) {
			kprintf("could not find thread %" B_PRId32 "\n", id);
			return 0;
		}
		uint64 oldSatp = Satp();
		SetSatp(thread->arch_info.context.satp);
		DoStackTrace(thread->arch_info.context.s[0], thread->arch_info.context.ra);
		SetSatp(oldSatp);
		return 0;
	}
	DoStackTrace(Fp(), 0);
	return 0;
}


void
arch_debug_stack_trace(void)
{
	DoStackTrace(Fp(), 0);
}


bool
arch_debug_contains_call(Thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void *
arch_debug_get_caller(void)
{
	return NULL;
}


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{
}


static void __attribute__((naked))
HandleFault()
{
	asm volatile("ld a0, 0(sp)");
	asm volatile("li a1, 1");
	asm volatile("call longjmp");
}


void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
	void (*function)(void*), void* parameter)
{
	cpu->fault_handler = (addr_t)&HandleFault;
	cpu->fault_handler_stack_pointer = (addr_t)&jumpBuffer;
	function(parameter);
}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	// TODO: Implement!
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	return 0;
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	// TODO: Implement!
	return NULL;
}


void
arch_debug_unset_current_thread(void)
{
	// TODO: Implement!
}


status_t
arch_debug_init_early(kernel_args *args)
{
	dprintf("arch_debug_init_early()\n");
	sKernelArgs = args;
	WriteImages();
	return B_OK;
}


status_t
arch_debug_init(kernel_args *args)
{
	sInitCalled = true;

	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace, "Stack crawl for current thread");

	return B_OK;
}
