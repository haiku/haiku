/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "convert.h"

#include <TranslatorFormats.h>

#include <Application.h>
#include <TextView.h>
#include <TypeConstants.h>
#include <ByteOrder.h>
#include <Node.h>
#include <Font.h>

#include <stdlib.h>
#include <string.h>


class AppServerConnection {
	public:
		AppServerConnection();
		~AppServerConnection();

	private:
		BApplication *fApplication;
};


AppServerConnection::AppServerConnection()
	:
	fApplication(NULL)
{
	// This is not nice, but it's the only we can provide all features on command
	// line tools that don't create a BApplication - without a BApplicatio, we
	// could not support any text styles (colors and fonts)

	if (be_app == NULL)
		fApplication = new BApplication("application/x-vnd.Haiku-RTF-Translator");
}


AppServerConnection::~AppServerConnection()
{
	delete fApplication;
}


//	#pragma mark -


static size_t
get_text_for_command(RTFCommand *command, char *text, size_t size)
{
	const char *name = command->Name();

	if (!strcmp(name, "\n")
		|| !strcmp(name, "par")
		|| !strcmp(name, "sect")) {
		if (text != NULL)
			strlcpy(text, "\n", size);
		return 1;
	}

	return 0;
}


static text_run *
get_style(BList &runs, text_run *run, int32 offset)
{
	if (run != NULL && offset == run->offset)
		return run;

	text_run *newRun = new text_run();
	if (newRun == NULL)
		throw (status_t)B_NO_MEMORY;

	// take over previous styles
	if (run != NULL)
		*newRun = *run;

	newRun->offset = offset;
	runs.AddItem(newRun);
	return newRun;
}


static text_run_array *
get_text_run_array(RTFHeader &header)
{
	// collect styles

	RTFIterator iterator(header, RTF_TEXT);
	text_run *current = NULL;
	int32 offset = 0;
	BList runs;

	while (iterator.HasNext()) {
		RTFElement *element = iterator.Next();
		if (RTFText *text = dynamic_cast<RTFText *>(element)) {
			offset += text->TextLength();
			continue;
		}

		RTFCommand *command = dynamic_cast<RTFCommand *>(element);
		if (command == NULL)
			continue;

		if (!strcmp(command->Name(), "cf")) {
			// foreground color
			current = get_style(runs, current, offset);
			current->color = header.Color(command->Option());
		} else if (!strcmp(command->Name(), "b")) {
			// bold style
			current = get_style(runs, current, offset);

			if (command->Option() != 0)
				current->font.SetFace(B_BOLD_FACE);
			else
				current->font.SetFace(B_REGULAR_FACE);
		} else
			offset += get_text_for_command(command, NULL, 0);
	}

	// are there any styles?
	if (runs.CountItems() == 0)
		return NULL;

	// create array

	text_run_array *array = (text_run_array *)malloc(sizeof(text_run_array)
		+ sizeof(text_run) * (runs.CountItems() - 1));
	if (array == NULL)
		throw (status_t)B_NO_MEMORY;

	array->count = runs.CountItems();

	for (int32 i = 0; i < array->count; i++) {
		text_run *run = (text_run *)runs.RemoveItem(0L);
		array->runs[i] = *run;
		delete run;
	}

	return array;
}


status_t
write_plain_text(RTFHeader &header, BDataIO &target)
{
	RTFIterator iterator(header, RTF_TEXT);

	while (iterator.HasNext()) {
		RTFElement *element = iterator.Next();
		char buffer[1024];
		const char *string = NULL;
		size_t size = 0;

		if (RTFText *text = dynamic_cast<RTFText *>(element)) {
			string = text->Text();
			size = text->TextLength();
		} else if (RTFCommand *command = dynamic_cast<RTFCommand *>(element)) {
			size = get_text_for_command(command, buffer, sizeof(buffer));
			if (size != 0)
				string = buffer;
		}

		if (size == 0)
			continue;

		ssize_t written = target.Write(string, size);
		if (written < B_OK)
			return written;
		if ((size_t)written != size)
			return B_IO_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


status_t
convert_to_stxt(RTFHeader &header, BDataIO &target)
{
	// count text bytes

	size_t textSize = 0;

	RTFIterator iterator(header, RTF_TEXT);
	while (iterator.HasNext()) {
		RTFElement *element = iterator.Next();
		if (RTFText *text = dynamic_cast<RTFText *>(element)) {
			textSize += text->TextLength();
		} else if (RTFCommand *command = dynamic_cast<RTFCommand *>(element)) {
			textSize += get_text_for_command(command, NULL, 0);
		}
	}

	// put out header

	TranslatorStyledTextStreamHeader stxtHeader;
	stxtHeader.header.magic = 'STXT';
	stxtHeader.header.header_size = sizeof(TranslatorStyledTextStreamHeader);
	stxtHeader.header.data_size = 0;
	stxtHeader.version = 100;
	status_t status = swap_data(B_UINT32_TYPE, &stxtHeader, sizeof(stxtHeader),
		B_SWAP_HOST_TO_BENDIAN);
	if (status != B_OK)
		return status;

	ssize_t written = target.Write(&stxtHeader, sizeof(stxtHeader));
	if (written < B_OK)
		return written;
	if (written != sizeof(stxtHeader))
		return B_IO_ERROR;

	TranslatorStyledTextTextHeader textHeader;
	textHeader.header.magic = 'TEXT';
	textHeader.header.header_size = sizeof(TranslatorStyledTextTextHeader);
	textHeader.header.data_size = textSize;
	textHeader.charset = B_UNICODE_UTF8;
	status = swap_data(B_UINT32_TYPE, &textHeader, sizeof(textHeader),
		B_SWAP_HOST_TO_BENDIAN);
	if (status != B_OK)
		return status;

	written = target.Write(&textHeader, sizeof(textHeader));
	if (written < B_OK)
		return written;
	if (written != sizeof(textHeader))
		return B_IO_ERROR;

	// put out main text

	status = write_plain_text(header, target);
	if (status < B_OK)
		return status;

	// prepare styles

	AppServerConnection connection;
		// we need that for the the text_run/BFont stuff

	text_run_array *runs = NULL;
	try {
		runs = get_text_run_array(header);
	} catch (status_t status) {
		return status;
	}

	if (runs == NULL)
		return B_OK;

	int32 flattenedSize;
	void *flattenedRuns = BTextView::FlattenRunArray(runs, &flattenedSize);
	if (flattenedRuns == NULL)
		return B_NO_MEMORY;

	// put out styles

	TranslatorStyledTextStyleHeader styleHeader;
	styleHeader.header.magic = 'STYL';
	styleHeader.header.header_size = sizeof(TranslatorStyledTextStyleHeader);
	styleHeader.header.data_size = flattenedSize;
	styleHeader.apply_offset = 0;
	styleHeader.apply_length = textSize;

	status = swap_data(B_UINT32_TYPE, &styleHeader, sizeof(styleHeader),
		B_SWAP_HOST_TO_BENDIAN);
	if (status != B_OK)
		return status;

	written = target.Write(&styleHeader, sizeof(styleHeader));
	if (written < B_OK)
		return written;
	if (written != sizeof(styleHeader))
		return B_IO_ERROR;

	// output actual style information
	written = target.Write(flattenedRuns, flattenedSize);
	if (written < B_OK)
		return written;
	if (written != flattenedSize)
		return B_IO_ERROR;

	return B_OK;
}


status_t
convert_to_plain_text(RTFHeader &header, BPositionIO &target)
{
	// put out main text

	status_t status = write_plain_text(header, target);
	if (status < B_OK)
		return status;

	// ToDo: this is not really nice, we should adopt the BPositionIO class
	//	from Dano/Zeta which has meta data support
	BNode *node = dynamic_cast<BNode *>(&target);
	if (node == NULL) {
		// we can't write the styles
		return B_OK;
	}

	// prepare styles

	AppServerConnection connection;
		// we need that for the the text_run/BFont stuff

	text_run_array *runs = NULL;
	try {
		runs = get_text_run_array(header);
	} catch (status_t status) {
		// doesn't matter too much if we could write the styles or not
		return B_OK;
	}

	if (runs == NULL)
		return B_OK;

	int32 flattenedSize;
	void *flattenedRuns = BTextView::FlattenRunArray(runs, &flattenedSize);
	if (flattenedRuns == NULL)
		return B_OK;

	// put out styles

	ssize_t written = node->WriteAttr("styles", B_RAW_TYPE, 0, flattenedRuns, flattenedSize);
	if (written >= B_OK && written != flattenedSize)
		node->RemoveAttr("styles");

	return B_OK;
}
