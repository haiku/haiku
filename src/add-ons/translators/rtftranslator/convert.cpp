/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "convert.h"
#include "Stack.h"

#include <TranslatorFormats.h>

#include <Application.h>
#include <TextView.h>
#include <TypeConstants.h>
#include <ByteOrder.h>
#include <Node.h>
#include <Font.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct conversion_context {
	conversion_context()
	{
		Reset();
	}

	void Reset();

	int32	section;
	int32	page;
	int32	start_page;
	int32	first_line_indent;
	bool	new_line;
};

struct group_context {
	RTF::Group	*group;
	text_run	*run;
};

class AppServerConnection {
	public:
		AppServerConnection();
		~AppServerConnection();

	private:
		BApplication *fApplication;
};


void
conversion_context::Reset()
{
	section = 1;
	page = 1;
	start_page = page;
	first_line_indent = 0;
	new_line = true;
}


//	#pragma mark -


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
write_text(conversion_context &context, const char *text, size_t length,
	BDataIO *target = NULL) throw (status_t)
{
	size_t prefix = 0;
	if (context.new_line) {
		prefix = context.first_line_indent;
		context.new_line = false;
	}

	if (target == NULL)
		return prefix + length;

	for (uint32 i = 0; i < prefix; i++) {
		write_text(context, " ", 1, target);
	}

	ssize_t written = target->Write(text, length);
	if (written < B_OK)
		throw (status_t)written;
	else if ((size_t)written != length)
		throw (status_t)B_IO_ERROR;

	return prefix + length;
}


static size_t
write_text(conversion_context &context, const char *text,
	BDataIO *target = NULL) throw (status_t)
{
	return write_text(context, text, strlen(text), target);
}


static size_t
next_line(conversion_context &context, const char *prefix,
	BDataIO *target) throw (status_t)
{
	size_t length = strlen(prefix);
	context.new_line = true;

	if (target != NULL) {
		ssize_t written = target->Write(prefix, length);
		if (written < B_OK)
			throw (status_t)written;
		else if ((size_t)written != length)
			throw (status_t)B_IO_ERROR;
	}

	return length;
}


static size_t
process_command(conversion_context &context, RTF::Command *command,
	BDataIO *target) throw (status_t)
{
	const char *name = command->Name();

	if (!strcmp(name, "par")) {
		// paragraph ended
		return next_line(context, "\n", target);
	}
	if (!strcmp(name, "sect")) {
		// section ended
		context.section++;
		return next_line(context, "\n", target);
	}
	if (!strcmp(name, "page")) {
		// we just insert two carriage returns for a page break
		context.page++;
		return next_line(context, "\n\n", target);
	}
	if (!strcmp(name, "tab")) {
		return write_text(context, "\t", target);
	}
	if (!strcmp(name, "pard")) {
		// reset paragraph
		context.first_line_indent = 0;
		return 0;
	}
	if (!strcmp(name, "fi") || !strcmp(name, "cufi")) {
		// "cufi" first line indent in 1/100 space steps
		// "fi" is most probably specified in 1/20 pts
		// Currently, we don't differentiate between the two...
		context.first_line_indent = (command->Option() + 50) / 100;
		if (context.first_line_indent < 0)
			context.first_line_indent = 0;
		if (context.first_line_indent > 8)
			context.first_line_indent = 8;

		return 0;
	}

	// document variables

	if (!strcmp(name, "sectnum")) {
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%ld", context.section);
		return write_text(context, buffer, target);
	}
	if (!strcmp(name, "pgnstarts")) {
		context.start_page = command->HasOption() ? command->Option() : 1;
		return 0;
	}
	if (!strcmp(name, "pgnrestart")) {
		context.page = context.start_page;
		return 0;
	}
	if (!strcmp(name, "chpgn")) {
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%ld", context.page);
		return write_text(context, buffer, target);
	}
	return 0;
}


static text_run *
get_style(BList &runs, text_run *run, int32 offset) throw (status_t)
{
	static const rgb_color kBlack = {0, 0, 0, 255};

	if (run != NULL && offset == run->offset)
		return run;

	text_run *newRun = new text_run();
	if (newRun == NULL)
		throw (status_t)B_NO_MEMORY;

	// take over previous styles
	if (run != NULL) {
		newRun->font = run->font;
		newRun->color = run->color;
	} else
		newRun->color = kBlack;

	newRun->offset = offset;

	runs.AddItem(newRun);
	return newRun;
}


static void
set_font_face(BFont &font, uint16 face, bool on)
{
	// Special handling for B_REGULAR_FACE, since BFont::SetFace(0)
	// just doesn't do anything

	if (font.Face() == B_REGULAR_FACE && on)
		font.SetFace(face);
	else if ((font.Face() & ~face) == 0 && !on)
		font.SetFace(B_REGULAR_FACE);
	else if (on)
		font.SetFace(font.Face() | face);
	else
		font.SetFace(font.Face() & ~face);
}


static text_run_array *
get_text_run_array(RTF::Header &header) throw (status_t)
{
	// collect styles

	RTF::Iterator iterator(header, RTF::TEXT_DESTINATION);
	RTF::Group *lastParent = &header;
	Stack<group_context> stack;
	conversion_context context;
	text_run *current = NULL;
	int32 offset = 0;
	BList runs;

	while (iterator.HasNext()) {
		RTF::Element *element = iterator.Next();

		if (RTF::Group *group = dynamic_cast<RTF::Group *>(element)) {
			// add new group context
			group_context newContext = {lastParent, current};
			stack.Push(newContext);

			lastParent = group;
			continue;
		}

		if (element->Parent() != lastParent) {
			// a group has been closed

			lastParent = element->Parent();

			group_context lastContext;
			while (stack.Pop(&lastContext)) {
				// find current group context
				if (lastContext.group == lastParent)
					break;
			}

			if (lastContext.run != NULL && current != NULL) {
				// has the style been changed?
				if (lastContext.run->offset != current->offset)
					current = get_style(runs, lastContext.run, offset);
			}
		}

		if (RTF::Text *text = dynamic_cast<RTF::Text *>(element)) {
			offset += write_text(context, text->String(), text->Length());
			continue;
		}

		RTF::Command *command = dynamic_cast<RTF::Command *>(element);
		if (command == NULL)
			continue;

		const char *name = command->Name();

		if (!strcmp(name, "cf")) {
			// foreground color
			current = get_style(runs, current, offset);
			current->color = header.Color(command->Option());
		} else if (!strcmp(name, "b")
			|| !strcmp(name, "embo") || !strcmp(name, "impr")) {
			// bold style ("emboss" and "engrave" are currently the same, too)
			current = get_style(runs, current, offset);
			set_font_face(current->font, B_BOLD_FACE, command->Option() != 0);
		} else if (!strcmp(name, "i")) {
			// bold style
			current = get_style(runs, current, offset);
			set_font_face(current->font, B_ITALIC_FACE, command->Option() != 0);
		} else if (!strcmp(name, "ul")) {
			// bold style
			current = get_style(runs, current, offset);
			set_font_face(current->font, B_UNDERSCORE_FACE, command->Option() != 0);
		} else if (!strcmp(name, "fs")) {
			// font size in half points
			current = get_style(runs, current, offset);
			current->font.SetSize(command->Option() / 2.0);
		} else if (!strcmp(name, "plain")) {
			// reset font to plain style
			current = get_style(runs, current, offset);
			current->font = be_plain_font;
		} else if (!strcmp(name, "f")) {
			// font number
			RTF::Group *fonts = header.FindGroup("fonttbl");
			if (fonts == NULL)
				continue;

			current = get_style(runs, current, offset);
			BFont font;
				// missing font info will be replaced by the default font

			RTF::Command *info;
			for (int32 index = 0; (info = fonts->FindDefinition("f", index)) != NULL; index++) {
				if (info->Option() != command->Option())
					continue;

				// ToDo: really try to choose font by name and serif/sans-serif
				// ToDo: the font list should be built before once

				// For now, it only differentiates fixed fonts from proportional ones
				if (fonts->FindDefinition("fmodern", index) != NULL)
					font = be_fixed_font;
			}

			font_family family;
			font_style style;
			font.GetFamilyAndStyle(&family, &style);

			current->font.SetFamilyAndFace(family, current->font.Face());
		} else
			offset += process_command(context, command, NULL);
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
write_plain_text(RTF::Header &header, BDataIO &target)
{
	RTF::Iterator iterator(header, RTF::TEXT_DESTINATION);
	conversion_context context;

	try {
		while (iterator.HasNext()) {
			RTF::Element *element = iterator.Next();
	
			if (RTF::Text *text = dynamic_cast<RTF::Text *>(element)) {
				write_text(context, text->String(), text->Length(), &target);
			} else if (RTF::Command *command = dynamic_cast<RTF::Command *>(element)) {
				process_command(context, command, &target);
			}
		}
	} catch (status_t status) {
		return status;
	}

	return B_OK;
}


//	#pragma mark -


status_t
convert_to_stxt(RTF::Header &header, BDataIO &target)
{
	// count text bytes

	size_t textSize = 0;

	RTF::Iterator iterator(header, RTF::TEXT_DESTINATION);
	conversion_context context;

	try {
		while (iterator.HasNext()) {
			RTF::Element *element = iterator.Next();
			if (RTF::Text *text = dynamic_cast<RTF::Text *>(element)) {
				textSize += write_text(context, text->String(), text->Length(), NULL);
			} else if (RTF::Command *command = dynamic_cast<RTF::Command *>(element)) {
				textSize += process_command(context, command, NULL);
			}
		}
	} catch (status_t status) {
		return status;
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
convert_to_plain_text(RTF::Header &header, BPositionIO &target)
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
