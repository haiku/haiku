/*
 * Copyright 2003-2007, Haiku. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		jonas.sundstrom@kirilla.com
 */


#include <TranslationKit.h>
#include <Application.h>
#include <String.h>
#include <File.h>
#include <Path.h>
#include <Entry.h>
#include <Mime.h>
#include <NodeInfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


extern const char *__progname;
const char *gProgramName = __progname;
bool gVerbose = false;


class TypeList {
	public:
		void Add(uint32 type);
		bool Remove(uint32 type);
		bool FindType(uint32 type);

		void SetTo(TypeList &types);
		int32 Count();
		uint32 TypeAt(int32 index);

	private:
		BList	fList;
};

class RemovingFile : public BFile {
	public:
		RemovingFile(const char *path);
		~RemovingFile();
		
		void Keep();
	
	private:
		BEntry	fEntry;
};

class Translator {
	public:
		Translator(const char *inPath, const char *outPath, uint32 outFormat);
		~Translator();

		status_t Translate();

	private:
		status_t Directly(BFile &input, translator_info &info, BFile &output);
		status_t Indirectly(BFile &input, BFile &output);
		status_t FindPath(const translation_format *info, BPositionIO &stream,
					TypeList &typesSeen, TypeList &path, double &pathQuality);
		status_t GetMimeTypeFromCode(uint32 type, char *mimeType);

	private:
		BTranslatorRoster *fRoster;
		BPath	fInputPath, fOutputPath;
		uint32	fOutputFormat;
};

class TranslateApp : public BApplication {
	public:
		TranslateApp(void);
		virtual ~TranslateApp();

		virtual void ReadyToRun(void);
		virtual void ArgvReceived(int32 argc, char **argv);

	private:
		void PrintUsage(void);
		void ListTranslators(uint32 type);

		uint32 GetTypeCodeForOutputMime(const char *mime);
		uint32 GetTypeCodeFromString(const char *string);
		status_t Translate(BFile &input, translator_info &translator, BFile &output, uint32 type);
		status_t Translate(BFile &input, BFile &output, uint32 type);
		status_t Translate(const char *inPath, const char *outPath, uint32 type);

		bool				fGotArguments;
		BTranslatorRoster	*fTranslatorRoster;
		int32				fTranslatorCount;
		translator_id		*fTranslatorArray; 
};


static void
print_tupel(const char *format, uint32 value)
{
	char tupel[5];

	for (int32 i = 0; i < 4; i++) {
		tupel[i] = (value >> (24 - i * 8)) & 0xff;
		if (!isprint(tupel[i]))
			tupel[i] = '.';
	}
	tupel[4] = '\0';

	printf(format, tupel);
}


static void
print_translation_format(const translation_format &format)
{
	print_tupel("'%s' ", format.type);
	print_tupel("(%s) ", format.group);

	printf("%.1f %.1f %s ; %s\n", format.quality, format.capability,
		format.MIME, format.name);
}


//	#pragma mark -


void
TypeList::Add(uint32 type)
{
	fList.AddItem((void *)type, 0);
}


bool
TypeList::Remove(uint32 type)
{
	return fList.RemoveItem((void *)type);
}


bool
TypeList::FindType(uint32 type)
{
	return fList.IndexOf((void *)type) >= 0;
}


void
TypeList::SetTo(TypeList &types)
{
	fList.MakeEmpty();

	for (int32 i = 0; i < types.Count(); i++)
		fList.AddItem((void *)types.TypeAt(i));
}


int32
TypeList::Count()
{
	return fList.CountItems();
}


uint32
TypeList::TypeAt(int32 index)
{
	return (uint32)fList.ItemAt(index);
}


//	#pragma mark -


RemovingFile::RemovingFile(const char *path)
	: BFile(path, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE),
	fEntry(path)
{
}


RemovingFile::~RemovingFile()
{
	fEntry.Remove();
}


void
RemovingFile::Keep()
{
	fEntry.Unset();
}


//	#pragma mark -


Translator::Translator(const char *inPath, const char *outPath, uint32 outputFormat)
	:
	fRoster(BTranslatorRoster::Default()),
	fInputPath(inPath),
	fOutputPath(outPath),
	fOutputFormat(outputFormat)
{
}


Translator::~Translator()
{
}


status_t
Translator::Translate()
{
	// input file
	BFile input;
	status_t status = input.SetTo(fInputPath.Path(), B_READ_ONLY);
	if (status != B_OK) {
		fprintf(stderr, "%s: could not open \"%s\": %s\n",
			gProgramName, fInputPath.Path(), strerror(status));
		return status;
	}

	// find a direct translator
	bool direct = true;
	translator_info translator;
	status = fRoster->Identify(&input, NULL, &translator, 0, NULL, fOutputFormat);
	if (status < B_OK) {
		// no direct translator found - let's try with something else
		status = fRoster->Identify(&input, NULL, &translator);
		if (status < B_OK) {
			fprintf(stderr, "%s: identifying \"%s\" failed: %s\n",
				gProgramName, fInputPath.Path(), strerror(status));
			return status;
		}

		direct = false;
	}

	// output file
	RemovingFile output(fOutputPath.Path());
	if ((status = output.InitCheck()) != B_OK) {
		fprintf(stderr, "%s: Could not create \"%s\": %s\n",
			gProgramName, fOutputPath.Path(), strerror(status));
		return status;
	}

	if (direct)
		status = Directly(input, translator, output);
	else
		status = Indirectly(input, output);

	if (status == B_OK) {
		output.Keep();

		// add filetype attribute
		update_mime_info(fOutputPath.Path(), false, true, false);

		char mimeType[B_ATTR_NAME_LENGTH];
		BNode node(fOutputPath.Path());
		BNodeInfo info(&node);
		if (info.GetType(mimeType) != B_OK || !strcasecmp(mimeType, B_FILE_MIME_TYPE)) {
			// the Registrar couldn't find a type for this file
			// so let's use the information we have from the
			// translator
			if (GetMimeTypeFromCode(fOutputFormat, mimeType) == B_OK)
				info.SetType(mimeType);
		}
	} else {
		fprintf(stderr, "%s: translating failed: %s\n",
			gProgramName, strerror(status));
	}

	return status;
}


/** Converts the input file to the output file using the
 *	specified translator.
 */

status_t
Translator::Directly(BFile &input, translator_info &info, BFile &output)
{
	if (gVerbose)
		printf("Direct translation from \"%s\"\n", info.name);

	return fRoster->Translate(&input, &info, NULL, &output, fOutputFormat);
}


/**	Converts the input file to the output file by computing the best
 *	quality path between the input type and the output type, and then
 *	applies as many translators as needed.
 */

status_t
Translator::Indirectly(BFile &input, BFile &output)
{
	TypeList translatorPath;
	TypeList handledTypes;
	double quality;

	if (FindPath(NULL, input, handledTypes, translatorPath, quality) != B_OK)
		return B_NO_TRANSLATOR;

	// The initial input stream is the input file which gets translated into
	// the first buffer. After that, the two buffers fill each other, until
	// the end is reached and the output file is the translation target

	BMallocIO buffer[2];
	BPositionIO *inputStream = &input;
	BPositionIO *outputStream = &buffer[0];

	for (int32 i = 0; i < translatorPath.Count(); i++) {
		uint32 type = translatorPath.TypeAt(i);
		if (type == fOutputFormat)
			outputStream = &output;
		else
			outputStream->SetSize(0);

		inputStream->Seek(0, SEEK_SET);
			// rewind the input stream

		status_t status = fRoster->Translate(inputStream, NULL, NULL, outputStream, type);
		if (status != B_OK)
			return status;

		// switch buffers
		inputStream = &buffer[i % 2];
		outputStream = &buffer[(i + 1) % 2];

		if (gVerbose)
			print_tupel("  '%s'\n", type);
	}

	return B_OK;
}


status_t
Translator::FindPath(const translation_format *format, BPositionIO &stream,
	TypeList &typesSeen, TypeList &path, double &pathQuality)
{
	translator_info *infos = NULL;
	translator_id *ids = NULL;
	int32 count;
	uint32 inFormat = 0;
	uint32 group = 0;
	status_t status;

	// Get a list of capable translators (or all of them)

	if (format == NULL) {
		status = fRoster->GetTranslators(&stream, NULL, &infos, &count);
		if (status == B_OK && count > 0) {
			inFormat = infos[0].type;
			group = infos[0].group;

			if (gVerbose) {
				puts("Indirect translation:");
				print_tupel("  '%s', ", inFormat);
				printf("%s\n", infos[0].name);
			}
		}
	} else {
		status = fRoster->GetAllTranslators(&ids, &count);
		inFormat = format->type;
		group = format->group;
	}
	if (status != B_OK || count == 0) {
		delete[] infos;
		delete[] ids;
		return status;
	}

	// build the best path to get from here to fOutputFormat recursively
	// (via depth search, best quality/capability wins)

	TypeList bestPath;
	double bestQuality = -1;
	status = B_NO_TRANSLATOR;

	for (int32 i = 0; i < count; i++) {
		const translation_format *formats;
		int32 formatCount;
		bool matches = false;
		int32 id = ids ? ids[i] : infos[i].translator;
		if (fRoster->GetInputFormats(id, &formats, &formatCount) != B_OK)
			continue;

		// see if this translator is good enough for us
		for (int32 j = 0; j < formatCount; j++) {
			if (formats[j].type == inFormat) {
				matches = true;
				break;
			}
		}

		if (!matches)
			continue;

		// search output formats

		if (fRoster->GetOutputFormats(id, &formats, &formatCount) != B_OK)
			continue;

		typesSeen.Add(inFormat);

		for (int32 j = 0; j < formatCount; j++) {
			if (formats[j].group != group || typesSeen.FindType(formats[j].type))
				continue;

			double formatQuality = formats[j].quality * formats[j].capability;

			if (formats[j].type == fOutputFormat) {
				// we've found our target type, so we can stop the path here
				bestPath.Add(formats[j].type);
				bestQuality = formatQuality;
				status = B_OK;
				continue;
			}

			TypeList path;
			double quality;
			if (FindPath(&formats[j], stream, typesSeen, path, quality) == B_OK) {
				if (bestQuality < quality * formatQuality) {
					bestQuality = quality * formatQuality;
					bestPath.SetTo(path);
					bestPath.Add(formats[j].type);
					status = B_OK;
				}
			}
		}

		typesSeen.Remove(inFormat);
	}

	if (status == B_OK) {
		pathQuality = bestQuality;
		path.SetTo(bestPath);
	}
	delete[] infos;
	delete[] ids;
	return status;
}


status_t
Translator::GetMimeTypeFromCode(uint32 type, char *mimeType)
{
	translator_id *ids = NULL;
	int32 count;
	status_t status = fRoster->GetAllTranslators(&ids, &count);
	if (status != B_OK)
		return status;

	status = B_NO_TRANSLATOR;

	for (int32 i = 0; i < count; i++) {
		const translation_format *format = NULL;
		int32 formatCount = 0;
		fRoster->GetOutputFormats(ids[i], &format, &formatCount);

		for (int32 j = 0; j < formatCount; j++) {
			if (type == format[j].type) {
				strcpy(mimeType, format[j].MIME);
				status = B_OK;
				break;
			}
		}
	}

	delete[] ids;
	return status;
}


//	#pragma mark -


TranslateApp::TranslateApp(void)
	: BApplication("application/x-vnd.haiku-translate"),
	fGotArguments(false),
	fTranslatorRoster(BTranslatorRoster::Default()),
	fTranslatorCount(0),
	fTranslatorArray(NULL)
{
	fTranslatorRoster->GetAllTranslators(&fTranslatorArray, &fTranslatorCount);
}


TranslateApp::~TranslateApp()
{
	delete[] fTranslatorArray;
}


void
TranslateApp::ArgvReceived(int32 argc, char **argv)
{
	if (argc < 2
		|| !strcmp(argv[1], "--help"))
		return;

	if (!strcmp(argv[1], "--list")) {
		fGotArguments = true;
		
		uint32 type = B_TRANSLATOR_ANY_TYPE;
		if (argc > 2)
			type = GetTypeCodeFromString(argv[2]);

		ListTranslators(type);
		return;
	}

	if (!strcmp(argv[1], "--verbose")) {
		fGotArguments = true;
		argc--;
		argv++;
		gVerbose = true;
	}

	if (argc != 4)
		return;

	fGotArguments = true;

	// get typecode of output format
	uint32 outputFormat = 0;
	BMimeType mime(argv[3]);

	if (mime.IsValid() && !mime.IsSupertypeOnly()) {
		// MIME-string
		outputFormat = GetTypeCodeForOutputMime(argv[3]);
	} else
		outputFormat = GetTypeCodeFromString(argv[3]);

	if (outputFormat == 0) {
		fprintf(stderr, "%s: bad format: %s\nformat is 4-byte type code or full MIME type\n",
			gProgramName, argv[3]);
		exit(-1);
	}

	Translator translator(argv[1], argv[2], outputFormat);
	status_t status = translator.Translate();
	if (status < B_OK)
		exit(-1);
}


void
TranslateApp::ReadyToRun(void)
{
	if (fGotArguments == false)
		PrintUsage();

	PostMessage(B_QUIT_REQUESTED);
}


void
TranslateApp::PrintUsage(void)
{
	printf("usage: %s { --list [type] | input output format }\n"
		"\t\"format\" can expressed as 4-byte type code (ie. 'TEXT') or as MIME type.\n",
		gProgramName);
}


void
TranslateApp::ListTranslators(uint32 type)
{
	for (int32 i = 0; i < fTranslatorCount; i++) { 
		const char *name = NULL;
		const char *info = NULL; 
		int32 version = 0;
		if (fTranslatorRoster->GetTranslatorInfo(fTranslatorArray[i], &name, &info, &version) != B_OK)
			continue;

		const translation_format *inputFormats = NULL;
		const translation_format *outputFormats = NULL;
		int32 inCount = 0, outCount = 0;
		fTranslatorRoster->GetInputFormats(fTranslatorArray[i], &inputFormats, &inCount);
		fTranslatorRoster->GetOutputFormats(fTranslatorArray[i], &outputFormats, &outCount);

		// test if the translator has formats of the specified type
		
		if (type != B_TRANSLATOR_ANY_TYPE) {
			bool matches = false;

			for (int32 j = 0; j < inCount; j++) {
				if (inputFormats[j].group == type || inputFormats[j].type == type) {
					matches = true;
					break;
				}
			}

			for (int32 j = 0; j < outCount; j++) {
				if (outputFormats[j].group == type || outputFormats[j].type == type) {
					matches = true;
					break;
				}
			}
			
			if (!matches)
				continue;
		}

		printf("name: %s\ninfo: %s\nversion: %ld.%ld.%ld\n", name, info,
			B_TRANSLATION_MAJOR_VERSION(version),
			B_TRANSLATION_MINOR_VERSION(version),
			B_TRANSLATION_REVISION_VERSION(version)); 

		for (int32 j = 0; j < inCount; j++) {
			printf("  input:\t");
			print_translation_format(inputFormats[j]);
		}

		for (int32 j = 0; j < outCount; j++) {
			printf("  output:\t");
			print_translation_format(outputFormats[j]);
		}

		printf("\n");
	}
}


uint32
TranslateApp::GetTypeCodeForOutputMime(const char *mime)
{
	for (int32 i = 0; i < fTranslatorCount; i++) {
		const translation_format *format = NULL;
		int32 count = 0;
		fTranslatorRoster->GetOutputFormats(fTranslatorArray[i], &format, &count);

		for (int32 j = 0; j < count; j++) {
			if (!strcmp(mime, format[j].MIME))
				return format[j].type;
		}
	}

	return 0;
}


uint32
TranslateApp::GetTypeCodeFromString(const char *string)
{
	size_t length = strlen(string);
	if (length > 4)
		return 0;

	uint8 code[4] = {' ', ' ', ' ', ' '};

	for (uint32 i = 0; i < length; i++)
		code[i] = (uint8)string[i];

	return B_HOST_TO_BENDIAN_INT32(*(uint32 *)code);
}


//	#pragma mark -


int
main()
{
	new TranslateApp();
	be_app->Run();

	return B_OK;
}

