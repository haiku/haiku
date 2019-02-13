/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AppFileInfo.h>
#include <Mime.h>

static const char *kCommandName = "settype";

static int kArgc;
static const char *const *kArgv;

// usage
const char *kUsage =
"Usage: %s <options> <file> ...\n"
"\n"
"Sets the MIME type, signature, and/or preferred application of one or more\n"
"files.\n"
"\n"
"Options:\n"
"  -h, --help\n"
"         - Print this help text and exit.\n"
"  -preferredAppSig <signature>\n"
"         - Set the preferred application of the given files to\n"
"           <signature>.\n"
"  -s <signature>\n"
"         - Set the application signature of the given files to\n"
"           <signature>.\n"
"  -t <type>\n"
"         - Set the MIME type of the given files to <type>.\n"
;

// print_usage
static void
print_usage(bool error)
{
	// get command name
	const char *commandName = NULL;
	if (kArgc > 0) {
		if (const char *lastSlash = strchr(kArgv[0], '/'))
			commandName = lastSlash + 1;
		else
			commandName = kArgv[0];
	}

	if (!commandName || strlen(commandName) == 0)
		commandName = kCommandName;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, commandName, commandName,
		commandName);
}

// print_usage_and_exit
static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}

// next_arg
static const char *
next_arg(int &argi, bool optional = false)
{
	if (argi >= kArgc) {
		if (!optional)
			print_usage_and_exit(true);
		return NULL;
	}

	return kArgv[argi++];
}

// check_mime_type
static void
check_mime_type(const char *type)
{
	// check type
	if (type) {
		if (!BMimeType::IsValid(type)) {
			fprintf(stderr, "\"%s\" is no valid MIME type.\n", type);

			exit(1);
		}
	}
}

// main
int
main(int argc, const char *const *argv)
{
	kArgc = argc;
	kArgv = argv;

	// parameters
	const char **files = new const char*[argc];
	int fileCount = 0;
	const char *type = NULL;
	const char *signature = NULL;
	const char *preferredApp = NULL;

	// parse the arguments
	for (int argi = 1; argi < argc; ) {
		const char *arg = argv[argi++];
		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);

			} else if (strcmp(arg, "-preferredAppSig") == 0) {
				preferredApp = next_arg(argi);

			} else if (strcmp(arg, "-s") == 0) {
				signature = next_arg(argi);

			} else if (strcmp(arg, "-t") == 0) {
				type = next_arg(argi);

			} else {
				fprintf(stderr, "Error: Invalid option: \"%s\"\n", arg);
				print_usage_and_exit(true);
			}
		} else {
			// file
			files[fileCount++] = arg;
		}
	}

	// check parameters
	if (!preferredApp && !signature && !type) {
		fprintf(stderr, "Error: At least one option of \"-preferredAppSig\", "
			"\"-s\", and \"-t\" must be given.\n");
		print_usage_and_exit(true);
	}

	if (fileCount == 0) {
		fprintf(stderr, "Error: No file specified.\n");
		print_usage_and_exit(true);
	}

	// check for valid MIME types
	check_mime_type(preferredApp);
	check_mime_type(type);
	check_mime_type(signature);

	// iterate through the files
	for (int i = 0; i < fileCount; i++) {
		const char *fileName = files[i];

		// check, whether the file exists
		BEntry entry;
		status_t error = entry.SetTo(fileName, false);
		if (error != B_OK) {
			fprintf(stderr, "Error: Can't access file \"%s\": %s\n",
				fileName, strerror(error));

			exit(1);
		}

		if (!entry.Exists()) {
			fprintf(stderr, "Error: \"%s\": No such file or directory.\n",
				fileName);

			exit(1);
		}

		// ... and has the right type
		if (signature && !entry.IsFile()) {
			fprintf(stderr, "Error: \"%s\" is not a file. Signatures can only "
				"be set for executable files.\n", fileName);

			exit(1);
		}

		// open the file
		BFile file;
		BNode _node;
		BNode &node = (signature ? file : _node);
		error = (signature ? file.SetTo(fileName, B_READ_WRITE)
			: node.SetTo(fileName));
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open file \"%s\": %s\n",
				fileName, strerror(error));

			exit(1);
		}

		// prepare an node/app info object
		BAppFileInfo appInfo;
		BNodeInfo _nodeInfo;
		BNodeInfo &nodeInfo = (signature ? appInfo : _nodeInfo);
		error = (signature ? appInfo.SetTo(&file) : nodeInfo.SetTo(&node));
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open file \"%s\": %s\n",
				fileName, strerror(error));

			exit(1);
		}

		// set preferred app
		if (preferredApp) {
			error = nodeInfo.SetPreferredApp(preferredApp);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to set the preferred "
					"application of file \"%s\": %s\n", fileName,
					strerror(error));

				exit(1);
			}
		}

		// set type
		if (type) {
			error = nodeInfo.SetType(type);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to set the MIME type of file "
					"\"%s\": %s\n", fileName, strerror(error));

				exit(1);
			}
		}

		// set signature
		if (signature) {
			error = appInfo.SetSignature(signature);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to set the signature of file "
					"\"%s\": %s\n", fileName, strerror(error));

				exit(1);
			}
		}
	}

	delete[] files;

	return 0;
}
