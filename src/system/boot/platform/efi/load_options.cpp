/*
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "load_options.h"

#include <StackOrHeapArray.h>
#include <SupportDefs.h>
#include <boot/platform.h>
#include <efi/protocol/loaded-image.h>
#include <string.h>
#include <util/convertutf.h>

#include "efi_platform.h"


static const char sBootMenu[] = "boot_menu";
static const char sBootDebugOutput[] = "boot_debug_output";
static efi_guid sLoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;


typedef struct {
	const char* start;
	const char* end;
	bool is_valid;
} parsed_option;


static bool
match_option_name(parsed_option option, const char* name)
{
	size_t len = option.end - option.start;
	return len == strlen(name) && memcmp(option.start, name, len) == 0;
}


static parsed_option
next_option(const char* options, size_t start, size_t length)
{
	while (start < length && options[start] == ' ')
		start++;

	size_t end = start;
	while (end < length && options[end] != ' ' && options[end] != '\0')
		end++;

	bool is_valid = start < length && options[start] != '\0';

	return parsed_option{
		options + start,
		options + end,
		is_valid,
	};
}


uint32
get_loader_boot_options(void)
{
	efi_loaded_image_protocol* loadedImageProtocol;
	efi_status status = kSystemTable->BootServices->HandleProtocol(
		kImage, &sLoadedImageProtocolGUID, (void**)&loadedImageProtocol);

	if (status != EFI_SUCCESS
		|| loadedImageProtocol->LoadOptions == NULL
		|| loadedImageProtocol->LoadOptionsSize == 0)
		return 0;

	uint32 boot_options = 0;

	const uint16* options_ucs2 = (const uint16*)loadedImageProtocol->LoadOptions;
	size_t options_size = loadedImageProtocol->LoadOptionsSize;
	size_t length_ucs2 = options_size / 2;
	BStackOrHeapArray<char, 64> options(options_size);

	ssize_t length = utf16le_to_utf8(options_ucs2, length_ucs2, options, options_size);
	if (length < 0)
		return 0;

	parsed_option option = next_option(options, 0, length);

	while (option.is_valid) {
		if (match_option_name(option, sBootMenu))
			boot_options |= BOOT_OPTION_MENU;

		if (match_option_name(option, sBootDebugOutput))
			boot_options |= BOOT_OPTION_DEBUG_OUTPUT;

		option = next_option(options, option.end - options, length);
	}

	return boot_options;
}
