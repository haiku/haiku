/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SubTitlesSRT.h"

#include <new>

#include <stdlib.h>

#include <File.h>
#include <TextEncoding.h>

#include "FileReadWrite.h"


SubTitlesSRT::SubTitlesSRT(BFile* file, const char* name)
	:
	SubTitles(),
	fName(name),
	fSubTitles(64)
{
	if (file == NULL)
		return;
	if (file->InitCheck() != B_OK)
		return;

	FileReadWrite lineProvider(file);
	BString line;
	enum {
		EXPECT_SEQUENCE_NUMBER = 0,
		EXPECT_TIME_CODE,
		EXPECT_TEXT
	};

	SubTitle subTitle;
	int32 lastSequenceNumber = 0;
	int32 currentLine = 0;

	BPrivate::BTextEncoding* decoder = NULL;

	int32 state = EXPECT_SEQUENCE_NUMBER;
	while (lineProvider.Next(line)) {
		line.RemoveAll("\n");
		line.RemoveAll("\r");
		switch (state) {
			case EXPECT_SEQUENCE_NUMBER:
			{
				if (line.IsEmpty())
					continue;

				line.Trim();
				int32 sequenceNumber = atoi(line.String());
				if (sequenceNumber != lastSequenceNumber + 1) {
					fprintf(stderr, "Warning: Wrong sequence number in SRT "
						"file: %" B_PRId32 ", expected: %" B_PRId32 ", line %"
						B_PRId32 "\n", sequenceNumber, lastSequenceNumber + 1,
						currentLine);
				}
				state = EXPECT_TIME_CODE;
				lastSequenceNumber = sequenceNumber;
				break;
			}

			case EXPECT_TIME_CODE:
			{
				line.Trim();
				int32 separatorPos = line.FindFirst(" --> ");
				if (separatorPos < 0) {
					fprintf(stderr, "Error: Time code expected on line %"
						B_PRId32 ", got '%s'\n", currentLine, line.String());
					return;
				}
				BString timeCode(line.String(), separatorPos);
				if (separatorPos != 12) {
					fprintf(stderr, "Warning: Time code broken on line %"
						B_PRId32 " (%s)?\n", currentLine, timeCode.String());
				}
				int hours;
				int minutes;
				int seconds;
				int milliSeconds;
				if (sscanf(timeCode.String(), "%d:%d:%d,%d", &hours, &minutes,
					&seconds, &milliSeconds) != 4) {
					fprintf(stderr, "Error: Failed to parse start time on "
						"line %" B_PRId32 "\n", currentLine);
					return;
				}
				subTitle.startTime = (bigtime_t)hours * 60 * 60 * 1000000LL
					+ (bigtime_t)minutes * 60 * 1000000LL
					+ (bigtime_t)seconds * 1000000LL
					+ (bigtime_t)milliSeconds * 1000;

				int32 endTimePos = separatorPos + 5;
				timeCode.SetTo(line.String() + endTimePos);
				if (sscanf(timeCode.String(), "%d:%d:%d,%d", &hours, &minutes,
					&seconds, &milliSeconds) != 4) {
					fprintf(stderr, "Error: Failed to parse end time on "
						"line %" B_PRId32 "\n", currentLine);
					return;
				}
				bigtime_t endTime = (bigtime_t)hours * 60 * 60 * 1000000LL
					+ (bigtime_t)minutes * 60 * 1000000LL
					+ (bigtime_t)seconds * 1000000LL
					+ (bigtime_t)milliSeconds * 1000;

				subTitle.duration = endTime - subTitle.startTime;

				state = EXPECT_TEXT;
				break;
			}

			case EXPECT_TEXT:
				if (line.IsEmpty()) {
					int32 index = _IndexFor(subTitle.startTime);
					SubTitle* clone = new(std::nothrow) SubTitle(subTitle);
					if (clone == NULL || !fSubTitles.AddItem(clone, index)) {
						delete clone;
						return;
					}
					subTitle.text = "";
					subTitle.placement = BPoint(-1, -1);
					subTitle.startTime = 0;
					subTitle.duration = 0;

					state = EXPECT_SEQUENCE_NUMBER;
				} else {
					if (decoder == NULL) {
						// We try to guess the encoding from the first line of
						// text in the subtitle file.
						decoder = new BPrivate::BTextEncoding(line.String(),
							line.Length());
					}
					char buffer[line.Length() * 4];
					size_t inLength = line.Length();
					size_t outLength = line.Length() * 4;
					decoder->Decode(line.String(), inLength, buffer, outLength);
					buffer[outLength] = 0;
					subTitle.text << buffer << '\n';
				}
				break;
		}
		line.SetTo("");
		currentLine++;
	}

	delete decoder;
}


SubTitlesSRT::~SubTitlesSRT()
{
	for (int32 i = fSubTitles.CountItems() - 1; i >= 0; i--)
		delete reinterpret_cast<SubTitle*>(fSubTitles.ItemAtFast(i));
}


const char*
SubTitlesSRT::Name() const
{
	return fName.String();
}


const SubTitle*
SubTitlesSRT::SubTitleAt(bigtime_t time) const
{
	int32 index = _IndexFor(time);
	SubTitle* subTitle
		= reinterpret_cast<SubTitle*>(fSubTitles.ItemAt(index));
	if (subTitle != NULL && subTitle->startTime > time)
		subTitle = reinterpret_cast<SubTitle*>(fSubTitles.ItemAt(index - 1));
	if (subTitle != NULL && subTitle->startTime <= time
		&& subTitle->startTime + subTitle->duration > time) {
		return subTitle;
	}
	return NULL;
}


int32
SubTitlesSRT::_IndexFor(bigtime_t startTime) const
{
	// binary search index
	int32 lower = 0;
	int32 upper = fSubTitles.CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		SubTitle* subTitle = reinterpret_cast<SubTitle*>(
			fSubTitles.ItemAtFast(mid));
		if (startTime < subTitle->startTime)
			upper = mid;
		else
			lower = mid + 1;
	}
	return lower;
}



