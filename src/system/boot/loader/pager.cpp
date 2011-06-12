/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "pager.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>

#include <boot/platform/generic/text_console.h>


// #pragma mark - PagerTextSource


PagerTextSource::~PagerTextSource()
{
}


// #pragma mark -


static size_t
next_line(const PagerTextSource& textSource, size_t width, size_t offset,
	char* buffer, size_t bufferSize)
{
	size_t bytesRead = textSource.Read(offset, buffer, bufferSize - 1);
	if (bytesRead == 0)
		return 0;

	buffer[bytesRead] = '\0';

	// replace all '\0's by spaces
	for (size_t i = 0; i < bytesRead; i++) {
		if (buffer[i] == '\0')
			buffer[i] = ' ';
	}

	if (const char* lineEnd = strchr(buffer, '\n'))
		bytesRead = lineEnd - buffer;

	if (bytesRead > (size_t)width)
		bytesRead = width;

	// replace unprintables by '.'
	for (size_t i = 0; i < bytesRead; i++) {
		if (!isprint(buffer[i]))
			buffer[i] = '.';
	}

	bool lineBreak = buffer[bytesRead] == '\n';

	buffer[bytesRead] = '\0';

	return bytesRead + (lineBreak ? 1 : 0);
}


static int32
count_lines(const PagerTextSource& textSource, size_t width, char* buffer,
	size_t bufferSize)
{
	int32 lineCount = 0;
	size_t offset = 0;

	while (true) {
		size_t bytesRead = next_line(textSource, width, offset, buffer,
			bufferSize);
		if (bytesRead == 0)
			break;

		offset += bytesRead;
		lineCount++;
	}

	return lineCount;
}


static size_t
offset_of_line(const PagerTextSource& textSource, size_t width, char* buffer,
	size_t bufferSize, int32 line)
{
	int32 lineCount = 0;
	size_t offset = 0;

	while (true) {
		if (line == lineCount)
			return offset;

		size_t bytesRead = next_line(textSource, width, offset, buffer,
			bufferSize);
		if (bytesRead == 0)
			break;

		offset += bytesRead;
		lineCount++;
	}

	return offset;
}


// #pragma mark -


void
pager(const PagerTextSource& textSource)
{
	console_set_cursor(0, 0);

	int32 width = console_width();
	int32 height = console_height();

	char lineBuffer[256];

	int32 lineCount = count_lines(textSource, width, lineBuffer,
		sizeof(lineBuffer));
	int32 topLine = 0;

	bool quit = false;
	while (!quit) {
		// get the text offset for the top line
		size_t offset = offset_of_line(textSource, width, lineBuffer,
			sizeof(lineBuffer), topLine);

		// clear the screen and print the lines
		console_clear_screen();

		int32 screenLine = 0;
		while (screenLine + 1 < height) {
			size_t bytesRead = next_line(textSource, width, offset, lineBuffer,
				sizeof(lineBuffer));
			if (bytesRead == 0)
				break;

			console_set_cursor(0, screenLine);
			puts(lineBuffer);

			offset += bytesRead;
			screenLine++;
		}

		// print the statistics line at the bottom
		console_set_cursor(0, height - 1);
		console_set_color(BLACK, GRAY);
		int32 bottomLine = std::min(topLine + height - 2, lineCount - 1);
		printf("%" B_PRIuSIZE " - %" B_PRIuSIZE "  %" B_PRIuSIZE "%%",
			topLine, bottomLine, (bottomLine + 1) * 100 / lineCount);
		console_set_color(WHITE, BLACK);

		// wait for a key that changes the position
		int32 previousTopLine = topLine;

		while (!quit && topLine == previousTopLine) {
			switch (console_wait_for_key()) {
				case TEXT_CONSOLE_KEY_ESCAPE:
				case 'q':
				case 'Q':
					// quit
					quit = true;
					break;

				case TEXT_CONSOLE_KEY_DOWN:
				case TEXT_CONSOLE_KEY_RETURN:
					// next line
					topLine++;
					break;

				case TEXT_CONSOLE_KEY_UP:
					// previous line
					topLine--;
					break;

				case TEXT_CONSOLE_KEY_PAGE_UP:
					// previous page
					topLine -= height - 1;
					break;

				case TEXT_CONSOLE_KEY_PAGE_DOWN:
					// next page
					topLine += height - 1;
					break;

				case TEXT_CONSOLE_KEY_HOME:
					// beginning of text
					topLine = 0;
					break;

				case TEXT_CONSOLE_KEY_END:
					// end of text
					topLine = lineCount;
					break;
			}

			if (topLine > lineCount - (height - 1))
				topLine = lineCount - (height - 1);
			if (topLine < 0)
				topLine = 0;
		}
	}
}
