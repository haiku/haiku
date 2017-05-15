/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include "JsonEvent.h"

#include <stdlib.h>
#include <stdio.h>

#include <String.h>


BJsonEvent::BJsonEvent(json_event_type eventType, const char* content)
	:
	fEventType(eventType),
	fContent(content),
	fOwnedContent(NULL)
{
}


BJsonEvent::BJsonEvent(const char* content)
	:
	fEventType(B_JSON_STRING),
	fContent(content),
	fOwnedContent(NULL)
{
}


BJsonEvent::BJsonEvent(double content) {
	fEventType = B_JSON_NUMBER;
	fContent = NULL;

	int actualLength = snprintf(0, 0, "%f", content) + 1;
	char* buffer = (char*) malloc(sizeof(char) * actualLength);

	if (buffer == NULL) {
		fprintf(stderr, "memory exhaustion\n");
			// given the risk, this is the only sensible thing to do here.
		exit(EXIT_FAILURE);
	}

	sprintf(buffer, "%f", content);
	fOwnedContent = buffer;
}


BJsonEvent::BJsonEvent(int64 content) {
	fEventType = B_JSON_NUMBER;
	fContent = NULL;
	fOwnedContent = NULL;

	static const char* zeroValue = "0";
	static const char* oneValue = "1";

	switch (content) {
		case 0:
			fContent = zeroValue;
			break;
		case 1:
			fContent = oneValue;
			break;
		default:
		{
			int actualLength = snprintf(0, 0, "%" B_PRId64, content) + 1;
			char* buffer = (char*) malloc(sizeof(char) * actualLength);

			if (buffer == NULL) {
				fprintf(stderr, "memory exhaustion\n");
					// given the risk, this is the only sensible thing to do
					// here.
				exit(EXIT_FAILURE);
			}

			sprintf(buffer, "%" B_PRId64, content);
			fOwnedContent = buffer;
			break;
		}
	}
}


BJsonEvent::BJsonEvent(json_event_type eventType)
	:
	fEventType(eventType),
	fContent(NULL),
	fOwnedContent(NULL)
{
}


BJsonEvent::~BJsonEvent()
{
	if (NULL != fOwnedContent)
		free(fOwnedContent);
}


json_event_type
BJsonEvent::EventType() const
{
	return fEventType;
}


const char*
BJsonEvent::Content() const
{
	if (NULL != fOwnedContent)
		return fOwnedContent;
	return fContent;
}


double
BJsonEvent::ContentDouble() const
{
	return strtod(Content(), NULL);
}


int64
BJsonEvent::ContentInteger() const
{
	return strtoll(Content(), NULL, 10);
}
