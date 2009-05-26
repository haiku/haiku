/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <driver_settings.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool gVerbose = false;

// Just add your test settings here, they will be picked
// up automatically

const char *kSettings[] = {
	"keyA b c d {\n"
	"	keyB {\n"
	"		keyC d e f {\n"
	"			keyD e\n"
	"			keyE f\n"
	"		}\n"
	"	}}\n",

	"keyA {\ndisabled\n}\n",

	"keyA = b { keyB=d =e { keyC \"f g\"; keyD h } }"
};


// needed by the driver_settings implementation, but not part of
// the original R5 libroot.so

size_t
strnlen(char const *string, size_t count)
{
	const char *pos = string;

	while (count-- && pos[0] != '\0')
		pos++;

	return pos - string;
}


/** Concatenates the source string to the destination, writes
 *	as much as "maxLength" bytes to the dest string.
 *	Always null terminates the string as long as maxLength is
 *	larger than the dest string.
 *	Returns the length of the string that it tried to create
 *	to be able to easily detect string truncation.
 */

size_t
strlcat(char *dest, const char *source, size_t maxLength)
{
	size_t destLength = strnlen(dest, maxLength);

	// This returns the wrong size, but it's all we can do
	if (maxLength == destLength)
		return destLength + strlen(source);

	dest += destLength;
	maxLength -= destLength;

	size_t i = 0;
	for (; i < maxLength - 1 && source[i]; i++) {
		dest[i] = source[i];
	}

	dest[i] = '\0';

	return destLength + i + strlen(source + i);
}


//	#pragma mark -


void
put_level_space(int32 level)
{
	while (level-- > 0)
		putchar('\t');
}


void
dump_parameter(const driver_parameter &parameter, int32 level)
{
	put_level_space(level);
	printf("\"%s\" =", parameter.name);

	for (int32 i = 0; i < parameter.value_count; i++)
		printf(" \"%s\"", parameter.values[i]);
	putchar('\n');

	for (int32 i = 0; i < parameter.parameter_count; i++)
		dump_parameter(parameter.parameters[i], level + 1);
}


void
dump_settings(const driver_settings &settings)
{
	for (int32 i = 0; i < settings.parameter_count; i++)
		dump_parameter(settings.parameters[i], 0);
}


void
print_settings(void *handle)
{
	char buffer[2048];
	size_t bufferSize = sizeof(buffer);

	if (get_driver_settings_string(handle, buffer, &bufferSize, false) < B_OK) {
		fprintf(stderr, "Could not get settings string (standard)\n");
		exit(-1);
	}
	puts("  ----- standard");
	puts(buffer);
	puts("  ----- standard reparsed");

	void *reparsedHandle = parse_driver_settings_string(buffer);
	if (reparsedHandle == NULL) {
		fprintf(stderr, "Could not reparse settings\n");
		exit(-1);
	}
	const driver_settings *settings = get_driver_settings(reparsedHandle);
	dump_settings(*settings);

	delete_driver_settings(reparsedHandle);

	bufferSize = sizeof(buffer);
	if (get_driver_settings_string(handle, buffer, &bufferSize, true) < B_OK) {
		fprintf(stderr, "Could not get settings string (flat)\n");
		exit(-1);
	}
	puts("  ----- flat");
	puts(buffer);
	puts("  ----- flat reparsed");

	reparsedHandle = parse_driver_settings_string(buffer);
	if (reparsedHandle == NULL) {
		fprintf(stderr, "Could not reparse settings\n");
		exit(-1);
	}
	settings = get_driver_settings(reparsedHandle);
	dump_settings(*settings);

	delete_driver_settings(reparsedHandle);
}


void
check_settings_string(uint32 num)
{
	const char *string = kSettings[num];

	printf("\n--------- settings %ld -----------\n", num);

	void *handle = parse_driver_settings_string(string);
	if (handle == NULL) {
		fprintf(stderr, "Could not parse settings 1\n");
		exit(-1);
	}
	const driver_settings *settings = get_driver_settings(handle);

	if (gVerbose) {
		puts("  ----- original");
		puts(string);
		puts("  ----- parsed");
		dump_settings(*settings);
	}

	print_settings(handle);
	delete_driver_settings(handle);
}


void
load_settings(const char *name)
{
	void *handle = load_driver_settings(name);
	if (handle == NULL) {
		fprintf(stderr, "Could not load settings \"%s\"\n", name);
		return;
	}

	const driver_settings *settings = get_driver_settings(handle);
	if (settings == NULL) {
		fprintf(stderr, "Could not get settings from loaded file \"%s\"\n", name);
		goto end;
	}

	printf("settings \"%s\" loaded successfully\n", name);
	if (gVerbose)
		dump_settings(*settings);

end:
	if (unload_driver_settings(handle) < B_OK)
		fprintf(stderr, "Could not unload driver settings \"%s\"\n", name);
}


int
main(int argc, char **argv)
{
	BDirectory directory("/boot/home/config/settings/kernel/drivers");
	if (directory.InitCheck() != B_OK) {
		fprintf(stderr, "Could not open directory: %s\n", strerror(directory.InitCheck()));
		return 0;
	}

	// yes, I know I am lazy...
	if (argc > 1 && !strcmp(argv[1], "-v"))
		gVerbose = true;

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		load_settings(ref.name);
	}

	// load additional files specified on the command line

	for (int32 i = 1; i < argc; i++) {
		const char *name = argv[i];
		if (name[0] == '-')
			continue;

		// make path absolute, so that load_driver_settings()
		// doesn't search in the standard directory
		BPath path(name);
		load_settings(path.Path());
	}

	// check fixed settings strings

	for (uint32 i = 0; i < sizeof(kSettings) / sizeof(char *); i++)
		check_settings_string(i);

	return 0;
}

