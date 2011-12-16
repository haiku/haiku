/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>
#include <String.h>


enum {
	FLAG_MANDATORY_FIELD	= 0x01,
	FLAG_LIST_ATTRIBUTE		= 0x02,
	FLAG_DONT_QUOTE			= 0x04,
};


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s [ <options> ] <optional package description> "
		"[ <package info> ]\n"
	"Converts an .OptionalPackageDescription to a .PackageInfo. If "
		"<package info>\n"
	"is not specified, the output is printed to stdout.\n"
	"Note that the generated .PackageInfo will not be complete. For several\n"
	"fields an empty string will be used, unless specified via an option.\n"
	"The \"provides\" and \"requires\" lists will always be empty, though\n"
	"\n"
	"Options:\n"
	"  -a <arch>        - Use the given architecture string. Default is to "
		"guess from the file name.\n"
	"  -d <description> - Use the given descripton string. Default is to use\n"
	"                     the summary.\n"
	"  -h, --help       - Print this usage info.\n"
	"  -p <packager>    - Use the given packager string. Default is an empty "
		"string.\n"
	"  -s <summary>     - Use the given summary string. Default is an empty "
		"string.\n"
	"  -v <version>     - Use the given version string. Overrides the version\n"
	"                     from the input file.\n"
	"  -V <vendor>      - Use the given vendor string. Default is an empty "
		"string.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


static const char*
guess_architecture(const char* name)
{
	if (strstr(name, "x86") != NULL) {
		if (strstr(name, "gcc4") != NULL)
			return "x86";

		return "x86_gcc2";
	}

	return NULL;
}


struct OuputWriter {
	OuputWriter(FILE* output, const BMessage& package)
		:
		fOutput(output),
		fPackage(package)
	{
	}

	void WriteAttribute(const char* attributeName, const char* fieldName,
		const char* defaultValue, uint32 flags)
	{
		if (fieldName != NULL) {
			int32 count;
			type_code type;
			if (fPackage.GetInfo(fieldName, &type, &count) != B_OK) {
				if ((flags & FLAG_MANDATORY_FIELD) != 0) {
					fprintf(stderr, "Error: Missing mandatory field \"%s\" in "
						"input file.\n", fieldName);
					exit(1);
				}
				count = 0;
			}

			if (count > 0) {
				if (count == 1) {
					const char* value;
					fPackage.FindString(fieldName, &value);
					_WriteSingleElementAttribute(attributeName, value, flags);
				} else {
					fprintf(fOutput, "\n%s {\n", attributeName);

					for (int32 i = 0; i < count; i++) {
						fprintf(fOutput, "\t");
						const char* value;
						fPackage.FindString(fieldName, i, &value);
						_WriteValue(value, flags);
						fputc('\n', fOutput);
					}

					fputs("}\n", fOutput);
				}

				return;
			}
		}

		// write the default value
		if (defaultValue != NULL)
			_WriteSingleElementAttribute(attributeName, defaultValue, flags);
	}

private:
	void _WriteSingleElementAttribute(const char* attributeName,
		const char* value, uint32 flags)
	{
		fputs(attributeName, fOutput);

		int32 indentation = 16 - (int32)strlen(attributeName);
		if (indentation > 0)
			indentation = (indentation + 3) / 4;
		else
			indentation = 1;

		for (int32 i = 0; i < indentation; i++)
			fputc('\t', fOutput);

		_WriteValue(value, flags);
		fputc('\n',  fOutput);
	}

	void _WriteValue(const char* value, uint32 flags)
	{
		BString escapedValue(value);

		if ((flags & FLAG_DONT_QUOTE) != 0) {
			escapedValue.CharacterEscape("\\\"' \t", '\\');
			fputs(escapedValue.String(), fOutput);
		} else {
			escapedValue.CharacterEscape("\\\"", '\\');
			fprintf(fOutput, "\"%s\"", escapedValue.String());
		}
	}

private:
	FILE*			fOutput;
	const BMessage&	fPackage;
};


int
main(int argc, const char* const* argv)
{
	const char* architecture = NULL;
	const char* version = NULL;
	const char* summary = "";
	const char* description = "";
	const char* packager = "";
	const char* vendor = "";

	while (true) {
		static const struct option kLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+ha:d:p:s:v:V:", kLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				architecture = optarg;
				break;

			case 'd':
				description = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'p':
				packager = optarg;
				break;

			case 's':
				summary = optarg;
				break;

			case 'v':
				version = optarg;
				break;

			case 'V':
				vendor = optarg;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One or two argument should remain -- the input file and optionally the
	// output file.
	if (optind + 1 != argc && optind + 2 != argc)
		print_usage_and_exit(true);

	const char* opdName = argv[optind++];
	const char* packageInfoName = optind < argc ? argv[optind++] : NULL;

	// guess architecture from the input file name, if not given
	if (architecture == NULL) {
		const char* fileName = strrchr(opdName, '/');
		if (fileName == NULL)
			fileName = opdName;
		else
			fileName++;

		// Try to guess from the file name.
		architecture = guess_architecture(fileName);

		// If we've got nothing yet, try to guess from the file name.
		if (architecture == NULL && fileName != opdName)
			architecture = guess_architecture(opdName);

		// fallback is "any"
		if (architecture == NULL)
			architecture = "any";
	}

	// open the input
	FILE* input = fopen(opdName, "r");
	if (input == NULL) {
		fprintf(stderr, "Failed to open input file \"%s\": %s\n", opdName,
			strerror(errno));
		exit(1);
	}

	// open the output
	FILE* output = packageInfoName != NULL
		? fopen(packageInfoName, "w+") : stdout;
	if (output == NULL) {
		fprintf(stderr, "Failed to open output file \"%s\": %s\n",
			packageInfoName, strerror(errno));
		exit(1);
	}

	// read and parse the input file
	BMessage package;
	BString fieldName;
	BString fieldValue;
	char lineBuffer[LINE_MAX];
	bool seenPackageAttribute = false;

	while (char* line = fgets(lineBuffer, sizeof(lineBuffer), input)) {
		// chop off line break
		size_t lineLen = strlen(line);
		if (lineLen > 0 && line[lineLen - 1] == '\n')
			line[--lineLen] = '\0';

		// flush previous field, if a new field begins, otherwise append
		if (lineLen == 0 || !isspace(line[0])) {
			// new field -- flush the previous one
			if (fieldName.Length() > 0) {
				fieldValue.Trim();
				package.AddString(fieldName.String(), fieldValue);
				fieldName = "";
			}
		} else if (fieldName.Length() > 0) {
			// append to current field
			fieldValue += line;
			continue;
		} else {
			// bogus line -- ignore
			continue;
		}

		if (lineLen == 0)
			continue;

		// parse new field
		char* colon = strchr(line, ':');
		if (colon == NULL) {
			// bogus line -- ignore
			continue;
		}

		fieldName.SetTo(line, colon - line);
		fieldName.Trim();
		if (fieldName.Length() == 0) {
			// invalid field name
			continue;
		}

		fieldValue = colon + 1;

		if (fieldName == "Package") {
			if (seenPackageAttribute) {
				fprintf(stderr, "Duplicate \"Package\" attribute!\n");
				exit(1);
			}

			seenPackageAttribute = true;
		}
	}

	// write the output
	OuputWriter writer(output, package);

	// name
	writer.WriteAttribute("name", "Package", NULL,
		FLAG_MANDATORY_FIELD | FLAG_DONT_QUOTE);

	// version
	writer.WriteAttribute("version", "Version", version, FLAG_DONT_QUOTE);

	// architecture
	fprintf(output, "architecture\t%s\n", architecture);

	// summary
	fprintf(output, "summary\t\t\t\"%s\"\n", summary);

	// description
	if (description != NULL)
		fprintf(output, "description\t\t\"%s\"\n", description);
	else
		fprintf(output, "description\t\t\"%s\"\n", summary);

	// packager
	fprintf(output, "packager\t\t\"%s\"\n", packager);

	// vendor
	fprintf(output, "vendor\t\t\t\"%s\"\n", vendor);

	// copyrights
	writer.WriteAttribute("copyrights", "Copyright", NULL,
		FLAG_MANDATORY_FIELD | FLAG_LIST_ATTRIBUTE);

	// licenses
	writer.WriteAttribute("licenses", "License", NULL, FLAG_LIST_ATTRIBUTE);

	// empty provides
	fprintf(output, "\nprovides {\n}\n");

	// empty requires
	fprintf(output, "\nrequires {\n}\n");

	// URLs
	writer.WriteAttribute("urls", "URL", NULL, FLAG_LIST_ATTRIBUTE);

	// source URLs
	writer.WriteAttribute("source-urls", "SourceURL", NULL,
		FLAG_LIST_ATTRIBUTE);

	return 0;
}
