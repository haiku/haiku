/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>

#include <debug.h>
#include <stdio.h>

#include "Zycore/Format.h"
#include "Zydis/Zydis.h"

#include "disasm_arch.h"
#include "elf.h"


static ZydisDecoder sDecoder;
static ZydisFormatter sFormatter;
static ZydisFormatterFunc sDefaultPrintAddressAbsolute;


static ZyanStatus
ZydisFormatterPrintAddressAbsolute(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer,
	ZydisFormatterContext* context)
{
	ZyanU64 address;
	ZYAN_CHECK(ZydisCalcAbsoluteAddress(context->instruction, context->operand,
		context->runtime_address, &address));

	const char* symbolName;
	addr_t baseAddress;
	status_t error;

	if (IS_KERNEL_ADDRESS(address)) {
		error = elf_debug_lookup_symbol_address(address, &baseAddress,
			&symbolName, NULL, NULL);
	} else {
		error = elf_debug_lookup_user_symbol_address(
			debug_get_debugged_thread()->team, address, &baseAddress,
			&symbolName, NULL, NULL);
	}

	if (error == B_OK) {
		ZYAN_CHECK(ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL));
		ZyanString* string;
		ZYAN_CHECK(ZydisFormatterBufferGetString(buffer, &string));
		int64_t offset = address - baseAddress;
		if (offset == 0)
			return ZyanStringAppendFormat(string, "<%s>", symbolName);
		return ZyanStringAppendFormat(string, "<%s+0x%" B_PRIx64 ">", symbolName, offset);
	}

	return sDefaultPrintAddressAbsolute(formatter, buffer, context);
}


extern "C" void
diasm_arch_assert_fail(const char* assertion, const char* file, unsigned int line,
	const char* function)
{
	kprintf("assert_fail: %s\n", assertion);
	while (true)
		;
}


extern "C" void
disasm_arch_assert(const char *condition)
{
	kprintf("assert: %s\n", condition);
}


status_t
disasm_arch_dump_insns(addr_t where, int count, addr_t baseAddress,
	int backCount)
{
	ZyanU8 buffer[ZYDIS_MAX_INSTRUCTION_LENGTH];
	ZydisDecodedInstruction instruction;
	int skipCount = 0;

	if (backCount > 0) {
		// count the instructions from base address to start address
		addr_t address = baseAddress;

		int baseCount = 0;

		while (address < where
			&& debug_memcpy(B_CURRENT_TEAM, &buffer, (const void*)address, sizeof(buffer)) == B_OK
			&& ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&sDecoder,
				(ZydisDecoderContext*)ZYAN_NULL, buffer, sizeof(buffer), &instruction))) {
			address += instruction.length;
			baseCount++;
		}

		if (address == where) {
			if (baseCount > backCount)
				skipCount = baseCount - backCount;
			count += baseCount;
		} else
			baseAddress = where;
	} else
		baseAddress = where;

	ZyanUSize offset = 0;

	for (int i = 0; i < count; i++, offset += instruction.length) {
		if (debug_memcpy(B_CURRENT_TEAM, &buffer, (const void*)(baseAddress + offset),
				sizeof(buffer))
			!= B_OK) {
			kprintf("<read fault>\n");
			break;
		}
		ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
		if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&sDecoder, buffer, sizeof(buffer), &instruction,
				operands))) {
			break;
		}
		if (skipCount > 0) {
			skipCount--;
			continue;
		}

		addr_t address = baseAddress + offset;
		if (address == where)
			kprintf("\x1b[34m");

		char hexString[32];
		char* srcHex = hexString;
		for (ZyanUSize i = 0; i < instruction.length; i++) {
			sprintf(srcHex, "%02" PRIx8, buffer[i]);
			srcHex += 2;
		}

		char formatted[1024];
		if (ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&sFormatter, &instruction, operands,
				instruction.operand_count_visible, formatted, sizeof(formatted),
				baseAddress + offset, NULL))) {
			kprintf("%#16llx: %16.16s\t%s\n", static_cast<unsigned long long>(address), hexString,
				formatted);
		} else {
			kprintf("%#16llx: failed-to-format\n", static_cast<unsigned long long>(address));
		}
		if (address == where)
			kprintf("\x1b[m");
	}
	return B_OK;
}


status_t
disasm_arch_init()
{
#ifdef __x86_64__
	ZydisDecoderInit(&sDecoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
	ZydisDecoderInit(&sDecoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

	ZydisFormatterInit(&sFormatter, ZYDIS_FORMATTER_STYLE_ATT);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_HEX_UPPERCASE, ZYAN_FALSE);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_ABSOLUTE,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_RELATIVE,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_DISP_PADDING,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&sFormatter, ZYDIS_FORMATTER_PROP_IMM_PADDING,
		ZYDIS_PADDING_DISABLED);

	sDefaultPrintAddressAbsolute = (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
	ZydisFormatterSetHook(&sFormatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS,
		(const void**)&sDefaultPrintAddressAbsolute);

	// XXX: check for AMD and set sVendor;
	return B_OK;
}


status_t
disasm_arch_fini()
{
	return B_OK;
}


