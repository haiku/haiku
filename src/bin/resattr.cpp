// resattr.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Resources.h>

// usage
static const char *kUsage =
"Usage: %s [ <options> ] -o <outFile> [ <inFile> ... ]\n"
"\n"
"Reads resources from zero or more input files and adds them as attributes\n"
"to the specified output file, or (in reverse mode) reads attributes from\n"
"zero or more input files and adds them as resources to the specified output\n"
"file. If not existent the output file is created as an empty file.\n"
"\n"
"Options:\n"
"  -h, --help       - Print this text.\n"
"  -o <outfile>     - Specifies the output file.\n"
"  -O, --overwrite  - Overwrite existing attributes. regardless of whether\n"
"                     an attribute does already exist, it is always written\n"
"                     when a respective resource is encountered in an input\n"
"                     file. The last input file specifying the attribute\n"
"                     wins. If the options is not given, already existing\n"
"                     attributes remain untouched. Otherwise the first input\n"
"                     file specifying an attribute wins.\n"
"  -r, --reverse    - Reverse mode: Reads attributes from input files and\n"
"                     writes resources. A unique resource ID is assigned to\n"
"                     each attribute, so there will be no conflicts if two\n"
"                     input files feature the same attribute.\n"
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
	fprintf((error ? stderr : stdout), kUsage, programName);
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

// write_attributes
static
void
write_attributes(BNode &out, const char *inFileName, BResources &resources,
	bool overwrite)
{
	// iterate through the resources
	type_code type;
	int32 id;
	const char *name;
	size_t size;
	for (int resIndex = 0;
		 resources.GetResourceInfo(resIndex, &type, &id, &name, &size);
		 resIndex++) {
		// if we shall not overwrite attributes, we skip the attribute, if it
		// already exists
		attr_info attrInfo;
		if (!overwrite && out.GetAttrInfo(name, &attrInfo) == B_OK)
			continue;

		// get the resource
		const void *data = resources.LoadResource(type, id, &size);
		if (!data && size > 0) {
			// should not happen
			fprintf(stderr, "Failed to get resource `%s', type: %" B_PRIx32
				", id: %" B_PRId32 " from input file `%s'\n", name, type, id,
				inFileName);
			exit(1);
		}

		// construct a name, if the resource doesn't have one
		char nameBuffer[32];
		if (!name) {
			sprintf(nameBuffer, "unnamed_%d\n", resIndex);
			name = nameBuffer;
		}

		// write the attribute
		ssize_t bytesWritten = out.WriteAttr(name, type, 0LL, data, size);
		if (bytesWritten < 0) {
			fprintf(stderr, "Failed to write attribute `%s' to output file: "
				"%s\n", name, strerror(bytesWritten));
		}
	}
}

// write_resources
static
void
write_resources(BResources &resources, const char *inFileName, BNode &in,
	int32 &resID)
{
	// iterate through the attributes
	char name[B_ATTR_NAME_LENGTH];
	while (in.GetNextAttrName(name) == B_OK) {
		// get attribute info
		attr_info attrInfo;
		status_t error = in.GetAttrInfo(name, &attrInfo);
		if (error != B_OK) {
			fprintf(stderr, "Failed to get info for attribute `%s' of input "
				"file `%s': %s\n", name, inFileName, strerror(error));
			exit(1);
		}

		// read attribute
		char *data = new char[attrInfo.size];
		ssize_t bytesRead = in.ReadAttr(name, attrInfo.type, 0LL, data,
			attrInfo.size);
		if (bytesRead < 0) {
			fprintf(stderr, "Failed to read attribute `%s' of input "
				"file `%s': %s\n", name, inFileName, strerror(bytesRead));
			delete[] data;
			exit(1);
		}

		// find unique ID
		const char *existingName;
		size_t existingSize;
		while (resources.GetResourceInfo(attrInfo.type, resID, &existingName,
			&existingSize)) {
			resID++;
		}

		// write resource
		error = resources.AddResource(attrInfo.type, resID++, data,
			attrInfo.size, name);
		if (error != B_OK) {
			fprintf(stderr, "Failed to write resource `%s' to output "
				"file: %s\n", name, strerror(error));
		}
		delete[] data;
	}
}

// resources_to_attributes
static
void
resources_to_attributes(const char *outputFile, const char **inputFiles,
	int inputFileCount, bool overwrite)
{
	// create output file, if it doesn't exist
	if (!BEntry(outputFile).Exists()) {
		BFile createdOut;
		status_t error = createdOut.SetTo(outputFile,
			B_READ_WRITE | B_CREATE_FILE);
		if (error != B_OK) {
			fprintf(stderr, "Failed to create output file `%s': %s\n",
				outputFile, strerror(error));
			exit(1);
		}
	}

	// open output file
	BNode out;
	status_t error = out.SetTo(outputFile);
	if (error != B_OK) {
		fprintf(stderr, "Failed to open output file `%s': %s\n", outputFile,
			strerror(error));
		exit(1);
	}

	// iterate through the input files
	for (int i = 0; i < inputFileCount; i++) {
		// open input file
		BFile in;
		error = in.SetTo(inputFiles[i], B_READ_ONLY);
		if (error != B_OK) {
			fprintf(stderr, "Failed to open input file `%s': %s\n",
				inputFiles[i], strerror(error));
			exit(1);
		}

		// open resources
		BResources resources;
		error = resources.SetTo(&in, false);
		if (error != B_OK) {
			fprintf(stderr, "Failed to read resources of input file `%s': %s\n",
				inputFiles[i], strerror(error));
			exit(1);
		}

		// add the attributes
		write_attributes(out, inputFiles[i], resources, overwrite);
	}
}

// resources_to_attributes
static
void
attributes_to_resources(const char *outputFile, const char **inputFiles,
	int inputFileCount)
{
	// open output file
	BFile out;
	status_t error = out.SetTo(outputFile, B_READ_WRITE | B_CREATE_FILE);
	if (error != B_OK) {
		fprintf(stderr, "Failed to open output file `%s': %s\n", outputFile,
			strerror(error));
		exit(1);
	}

	// init output resources
	BResources resources;
	error = resources.SetTo(&out, false);
	if (error != B_OK) {
		fprintf(stderr, "Failed to init resources of output file `%s': %s\n",
			outputFile, strerror(error));
		exit(1);
	}

	int32 resID = 0;

	// iterate through the input files
	for (int i = 0; i < inputFileCount; i++) {
		// open input file
		BNode in;
		error = in.SetTo(inputFiles[i]);
		if (error != B_OK) {
			fprintf(stderr, "Failed to open input file `%s': %s\n",
				inputFiles[i], strerror(error));
			exit(1);
		}

		// add the resources

		write_resources(resources, inputFiles[i], in, resID);
	}
}

// main
int
main(int argc, const char *const *argv)
{
	sArgc = argc;
	sArgv = argv;

	// parameters
	const char *outputFile = NULL;
	bool overwrite = false;
	bool reverse = false;
	const char **inputFiles = new const char*[argc];
	int inputFileCount = 0;

	// parse arguments
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "-o") == 0) {
				outputFile = next_arg(argc, argv, argi);
			} else if (strcmp(arg, "-O") == 0) {
				overwrite = true;
			} else if (strcmp(arg, "-r") == 0
				|| strcmp(arg, "--reverse") == 0) {
				reverse = true;
			} else {
				print_usage_and_exit(true);
			}
		} else {
			inputFiles[inputFileCount++] = arg;
		}
	}

	// check parameters
	if (!outputFile)
		print_usage_and_exit(true);

	if (reverse) {
		attributes_to_resources(outputFile, inputFiles, inputFileCount);
	} else {
		resources_to_attributes(outputFile, inputFiles, inputFileCount,
			overwrite);
	}

	delete[] inputFiles;

	return 0;
}
