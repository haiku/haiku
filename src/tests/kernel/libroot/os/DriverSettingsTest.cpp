#include <driver_settings.h>
#include <Directory.h>
#include <Entry.h>

#include <stdio.h>
#include <string.h>


bool gVerbose = false;


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
check_settings_string(int32 num)
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
		void *handle = load_driver_settings(ref.name);
		if (handle == NULL) {
			fprintf(stderr, "Could not load settings \"%s\"\n", ref.name);
			continue;
		}

		const driver_settings *settings = get_driver_settings(handle);
		if (settings == NULL) {
			fprintf(stderr, "Could not get settings from loaded file \"%s\"\n", ref.name);
			goto end;
		}

		printf("settings \"%s\" loaded successfully\n", ref.name);
		if (gVerbose)
			dump_settings(*settings);

	end:
		if (unload_driver_settings(handle) < B_OK)
			fprintf(stderr, "Could not unload driver settings \"%s\"\n", ref.name);
	}

	// check fixed settings strings

	for (int32 i = 0; i < 3; i++)
		check_settings_string(i);
	
	return 0;
}

