// restest.cpp

#include <algobase.h>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <String.h>

#include "Exception.h"
#include "OffsetFile.h"
#include "ResourceFile.h"
#include "Warnings.h"

const char kUsage[] = {
"Usage: %s <options> <filenames>\n"
"options:\n"
"  -h, --help         print this help\n"
"  -l, --list         list each file's resources (short version)\n"
"  -L, --list-long    list each file's resources (long version)\n"
"  -s, --summary      print a summary\n"
"  -w, --write-test   write the file resources (in memory only) and\n"
"                     compare the data with the file's\n"
};

const status_t USAGE_ERROR	= B_ERRORS_END + 1;
const status_t USAGE_HELP	= B_ERRORS_END + 2;

enum listing_level {
	NO_LISTING,
	SHORT_LISTING,
	LONG_LISTING,
};

struct TestOptions {
	listing_level	listing;
	bool			write_test;
	bool			summary;
};

struct TestResult {
	TestResult(const char* filename)
		: filename(filename), warnings(), exception(NULL)
	{
	}

	~TestResult()
	{
		delete exception;
	}

	BString			filename;
	Warnings		warnings;
	Exception*		exception;
};

// print_indented
void
print_indented(const char* str, uint32 chars, bool indentFirst = true)
{
	const uint32 MAX_CHARS_PER_LINE = 75;
	int32 charsLeft = strlen(str);
	if (chars < MAX_CHARS_PER_LINE) {
		for (int32 line = 0; charsLeft > 0; line++) {
			if (line != 0 || indentFirst) {
				for (int32 i = 0; (uint32)i < chars; i++)
					printf(" ");
			}
			int32 bytesLeftOnLine = MAX_CHARS_PER_LINE - chars;
			int32 printChars = min(bytesLeftOnLine, charsLeft);
			printf("%.*s\n", (int)printChars, str);
			str += printChars;
			charsLeft -= printChars;
			// skip spaces
			while (*str == ' ') {
				str++;
				charsLeft--;
			}
		}
	}
}

// print_indented
void
print_indented(const char* indentStr, const char* str)
{
	uint32 chars = strlen(indentStr);
	printf(indentStr);
	print_indented(str, chars, false);
}

// parse_arguments
void
parse_arguments(int argc, const char* const* argv, BList& files,
				TestOptions& options)
{
	// default options
	options.listing = NO_LISTING;
	options.write_test = false;
	options.summary = false;
	// parse arguments
	for (int32 i = 1; i < argc; i++) {
		const char* arg = argv[i];
		int32 len = strlen(arg);
		if (len == 0)
			throw Exception(USAGE_ERROR, "Illegal argument: `'.");
		if (arg[0] == '-') {
			if (len < 2)
				throw Exception(USAGE_ERROR, "Illegal argument: `-'.");
			if (arg[1] == '-') {
				const char* option = arg + 2;
				// help
				if (!strcmp(option, "help")) {
					throw Exception(USAGE_HELP);
				// list
				} else if (!strcmp(option, "list")) {
					if (options.listing == NO_LISTING)
						options.listing = SHORT_LISTING;
				// list-long
				} else if (!strcmp(option, "list-long")) {
					options.listing = LONG_LISTING;
				// summary
				} else if (!strcmp(option, "summary")) {
					options.summary = true;
				// write-test
				} else if (!strcmp(option, "write-test")) {
					options.write_test = true;
				// error
				} else {
					throw Exception(USAGE_ERROR, BString("Illegal option: `")
												 << arg << "'.");
				}
			} else {
				for (int32 i = 1; i < len; i++) {
					char option = arg[i];
					switch (option) {
						// help
						case 'h':
							throw Exception(USAGE_HELP);
							break;
						// list
						case 'l':
							if (options.listing == NO_LISTING)
								options.listing = SHORT_LISTING;
							break;
						// list long
						case 'L':
							options.listing = LONG_LISTING;
							break;
						// summary
						case 's':
							options.summary = true;
							break;
						// write test
						case 'w':
							options.write_test = true;
							break;
						// error
						default:
							throw Exception(USAGE_ERROR,
											BString("Illegal option: `")
											<< arg << "'.");
							break;
					}
				}
			}
		} else
			files.AddItem(const_cast<char*>(arg));
	}
}

// test_file
void
test_file(const char* filename, const TestOptions& options,
		  TestResult& testResult)
{
	Warnings::SetCurrentWarnings(&testResult.warnings);
	ResourceFile resFile;
	try {
		// check if the file exists
		BEntry entry(filename, true);
		status_t error = entry.InitCheck();
		if (error != B_OK)
			throw Exception(error);
		if (!entry.Exists() || !entry.IsFile())
			throw Exception("Entry doesn't exist or is no regular file.");
		entry.Unset();
		// open the file
		BFile file(filename, B_READ_ONLY);
		error = file.InitCheck();
		if (error != B_OK)
			throw Exception(error, "Failed to open file.");
		// do the actual test
		resFile.Init(file);
		if (options.write_test)
			resFile.WriteTest();
	} catch (Exception exception) {
		testResult.exception = new Exception(exception);
	}
	Warnings::SetCurrentWarnings(NULL);
	// print warnings and error
	if (options.listing != NO_LISTING
		|| testResult.warnings.CountWarnings() > 0 || testResult.exception) {
		printf("\nFile `%s':\n", filename);
	}
	// warnings
	if (testResult.warnings.CountWarnings() > 0) {
		for (int32 i = 0;
			 const char* warning = testResult.warnings.WarningAt(i);
			 i++) {
			print_indented("  Warning: ", warning);
		}
	}
	// error
	if (testResult.exception) {
		status_t error = testResult.exception->GetError();
		const char* description = testResult.exception->GetDescription();
		if (strlen(description) > 0) {
			print_indented("  Error:   ", description);
			if (error != B_OK)
				print_indented("           ", strerror(error));
		} else if (error != B_OK)
			print_indented("  Error:   ", strerror(error));
	}
	// list resources
	if (resFile.InitCheck() == B_OK) {
		switch (options.listing) {
			case NO_LISTING:
				break;
			case SHORT_LISTING:
				resFile.PrintToStream(false);
				break;
			case LONG_LISTING:
				resFile.PrintToStream(true);
				break;
		}
	}
}

// test_files
void
test_files(BList& files, TestOptions& options)
{
	BList testResults;
	int32 successTestCount = 0;
	int32 warningTestCount = 0;
	int32 failedTestCount = 0;
	for (int32 i = 0;
		 const char* filename = (const char*)files.ItemAt(i);
		 i++) {
		TestResult* testResult = new TestResult(filename);
		testResults.AddItem(testResult);
		test_file(filename, options, *testResult);
		if (testResult->exception)
			failedTestCount++;
		else if (testResult->warnings.CountWarnings() > 0)
			warningTestCount++;
		else
			successTestCount++;
	}
	// print summary
	if (options.summary) {
		printf("\nSummary:\n");
		printf(  "=======\n");
		// successful tests
		if (successTestCount > 0) {
			if (successTestCount == 1)
				printf("one successful test\n");
			else
				printf("%ld successful tests\n", successTestCount);
		}
		// tests with warnings
		if (warningTestCount > 0) {
			if (warningTestCount == 1)
				printf("one test with warnings:\n");
			else
				printf("%ld tests with warnings:\n", warningTestCount);
			for (int32 i = 0;
				 TestResult* testResult = (TestResult*)testResults.ItemAt(i);
				 i++) {
				if (!testResult->exception
					&& testResult->warnings.CountWarnings() > 0) {
					printf("  `%s'\n", testResult->filename.String());
				}
			}
		}
		// failed tests
		if (failedTestCount > 0) {
			if (failedTestCount == 1)
				printf("one test failed:\n");
			else
				printf("%ld tests failed:\n", failedTestCount);
			for (int32 i = 0;
				 TestResult* testResult = (TestResult*)testResults.ItemAt(i);
				 i++) {
				if (testResult->exception)
					printf("  `%s'\n", testResult->filename.String());
			}
		}
	}
	// cleanup
	for (int32 i = 0;
		 TestResult* testResult = (TestResult*)testResults.ItemAt(i);
		 i++) {
		delete testResult;
	}
}

// main
int
main(int argc, const char* const* argv)
{
	int returnValue = 0;
	const char* cmdName = argv[0];
	TestOptions options;
	BList files;
	try {
		// parse arguments
		parse_arguments(argc, argv, files, options);
		if (files.CountItems() == 0)
			throw Exception(USAGE_ERROR, "No files given.");
		// test the files
		test_files(files, options);
	} catch (Exception exception) {
		status_t error = exception.GetError();
		const char* description = exception.GetDescription();
		switch (error) {
			case B_OK:
				if (strlen(description) > 0)
					fprintf(stderr, "%s\n", description);
				returnValue = 1;
				break;
			case USAGE_ERROR:
				if (strlen(description) > 0)
					fprintf(stderr, "%s\n", description);
				fprintf(stderr, kUsage, cmdName);
				returnValue = 1;
				break;
			case USAGE_HELP:
				printf(kUsage, cmdName);
				break;
			default:
				fprintf(stderr, "  error: %s\n", strerror(error));
				returnValue = 1;
				break;
		}
	}
	return returnValue;
}

