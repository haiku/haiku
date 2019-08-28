// clipboard.cpp

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include <Application.h>
#include <Clipboard.h>
#include <Message.h>

// usage
static const char *kUsage =
"Usage: %s [ <options> ] clear\n"
"       %s [ <options> ] dump\n"
"       %s [ <options> ] set <value>\n"
"\n"
"Manipulates or dump the contents of the clipboard.\n"
"\n"
"Commands:\n"
"  clear          - Clears the contents of the clipboard.\n"
"  dump           - Prints the contents of the clipboard.\n"
"  set            - Clears the clipboard and adds <value> as \"text/plain\".\n"
"\n"
"Options:\n"
"  -h, --help     - Print this text.\n"
"  -c <clipboard> - The name of the clipboard to be used. Default is the\n"
"                   system clipboard.\n"
;

// command line args
static int sArgc;
static const char *const *sArgv;

// print_usage
void
print_usage(bool error)
{
	// get nice program name
	const char *programName = (sArgc > 0 ? sArgv[0] : "resattr");
	if (const char *lastSlash = strrchr(programName, '/'))
		programName = lastSlash + 1;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, programName, programName,
		programName, programName);
}

// print_usage_and_exit
static
void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}

// next_arg
static
const char*
next_arg(int argc, const char* const* argv, int& argi, bool dontFail = false)
{
	if (argi + 1 >= argc) {
		if (dontFail)
			return NULL;
		print_usage_and_exit(true);
	}

	return argv[++argi];
}

// clipboard_clear
static
void
clipboard_clear(BClipboard &clipboard)
{
	// clear the data
	status_t error = clipboard.Clear();
	if (error != B_OK) {
		fprintf(stderr, "Failed to clear clipboard data: %s\n",
			strerror(error));
		exit(1);
	}
}

// clipboard_dump
static
void
clipboard_dump(BClipboard &clipboard)
{
	const char *data;
	ssize_t size;
	if (clipboard.Data()->FindData("text/plain", B_MIME_TYPE,
		(const void**)&data, &size) == B_OK) {
		printf("%.*s\n", (int)size, data);
	} else
		clipboard.Data()->PrintToStream();
}

// clipboard_set
static
void
clipboard_set(BClipboard &clipboard, const char *value)
{
	// clear clipboard
	clipboard_clear(clipboard);

	// add data
	status_t error = clipboard.Data()->AddData("text/plain", B_MIME_TYPE, value,
		strlen(value) + 1);
	if (error != B_OK) {
		fprintf(stderr, "Failed to add clipboard data: %s\n", strerror(error));
		exit(1);
	}
}

// main
int
main(int argc, const char *const *argv)
{
	sArgc = argc;
	sArgv = argv;

	// parameters
	const char *clipboardName = NULL;
	const char *command = NULL;
	const char *value = NULL;

	// parse arguments
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "-c") == 0) {
				clipboardName = next_arg(argc, argv, argi);
			} else {
				print_usage_and_exit(true);
			}
		} else {
			command = arg;
			value = next_arg(argc, argv, argi, true);
		}
	}

	// check parameters
	if (!command)
		print_usage_and_exit(true);

	// create a BApplication on BeOS
	struct utsname unameInfo;
	if (uname(&unameInfo) < 0 || strcmp(unameInfo.sysname, "Haiku") != 0)
		new BApplication("application/x-vnd.haiku.clipboard");

	// init clipboard
	BClipboard clipboard(clipboardName ? clipboardName : "system");

	// lock clipboard
	if (!clipboard.Lock()) {
		fprintf(stderr, "Failed to lock clipboard `%s'\n", clipboard.Name());
		exit(1);
	}

	// check, if data exist
	if (!clipboard.Data()) {
		fprintf(stderr, "Failed to get data from clipboard `%s'.",
			clipboard.Name());
		exit(1);
	}

	// execute the command
	bool needsCommit = true;
	if (strcmp(command, "clear") == 0) {
		// clear
		if (value)
			print_usage_and_exit(true);
		clipboard_clear(clipboard);
	} else if (strcmp(command, "dump") == 0) {
		// dump
		if (value)
			print_usage_and_exit(true);
		clipboard_dump(clipboard);
		needsCommit = false;
	} else if (strcmp(command, "set") == 0) {
		// set
		if (!value)
			print_usage_and_exit(true);
		clipboard_set(clipboard, value);
	} else
		print_usage_and_exit(true);

	// commit the change
	if (needsCommit) {
		status_t error = clipboard.Commit();
		if (error != B_OK) {
			fprintf(stderr, "Failed to commit clipboard data: %s\n",
				strerror(error));
			exit(1);
		}
	}

	// unlock the clipboard (just for completeness :-)
	clipboard.Unlock();

	return 0;
}
