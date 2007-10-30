/* Container - message part container class
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <String.h>
#include <List.h>
#include <Mime.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

class _EXPORT BMIMEMultipartMailContainer;

#include <MailContainer.h>
#include <MailAttachment.h>

typedef struct message_part {
	message_part(off_t start, off_t end) { this->start = start; this->end = end; }

	// Offset where the part starts (includes MIME sub-headers but not the
	// boundary line) in the message file.
	int32 start;

	// Offset just past the last byte of data, so total length == end - start.
	// Note that the CRLF that starts the next boundary isn't included in the
	// data, the end points at the start of the next CRLF+Boundary.  This can
	// lead to weird things like the blank line ending the subheader being the
	// same as the boundary starting CRLF.  So if you have something malformed
	// like this:
	// ------=_NextPart_005_0040_ENBYSXVW.VACTSCVC
    // Content-Type: text/plain; charset="ISO-8859-1"
    //
    // ------=_NextPart_005_0040_ENBYSXVW.VACTSCVC
    // If you subtract the header length (which includes the blank line) from
    // the MIME part total length (which doesn't include the blank line - it's
    // part of the next boundary), you get -2.
	int32 end;
} message_part;


BMIMEMultipartMailContainer::BMIMEMultipartMailContainer(
	const char *boundary,
	const char *this_is_an_MIME_message_text,
	uint32 defaultCharSet)
	:
	BMailContainer (defaultCharSet),
	_boundary(NULL),
	_MIME_message_warning(this_is_an_MIME_message_text),
	_io_data(NULL)
{
	// Definition of the MIME version in the mail header should be enough
	SetHeaderField("MIME-Version","1.0");
	SetHeaderField("Content-Type","multipart/mixed");
	SetBoundary(boundary);
}

/*BMIMEMultipartMailContainer::BMIMEMultipartMailContainer(BMIMEMultipartMailContainer &copy) :
	BMailComponent(copy),
	_boundary(copy._boundary),
	_MIME_message_warning(copy._MIME_message_warning),
	_io_data(copy._io_data) {
		AddHeaderField("MIME-Version","1.0");
		AddHeaderField("Content-Type","multipart/mixed");
		SetBoundary(boundary);
	}*/


BMIMEMultipartMailContainer::~BMIMEMultipartMailContainer() {
	for (int32 i = 0; i < _components_in_raw.CountItems(); i++)
		delete (message_part *)_components_in_raw.ItemAt(i);

	for (int32 i = 0; i < _components_in_code.CountItems(); i++)
		delete (BMailComponent *)_components_in_code.ItemAt(i);

	free((void *)_boundary);
}


void BMIMEMultipartMailContainer::SetBoundary(const char *boundary) {
	free ((void *) _boundary);
	_boundary = NULL;
	if (boundary != NULL)
		_boundary = strdup(boundary);

	BMessage structured;
	HeaderField("Content-Type",&structured);

	if (_boundary == NULL)
		structured.RemoveName("boundary");
	else if (structured.ReplaceString("boundary",_boundary) != B_OK)
		structured.AddString("boundary",_boundary);

	SetHeaderField("Content-Type",&structured);
}


void BMIMEMultipartMailContainer::SetThisIsAnMIMEMessageText(const char *text) {
	_MIME_message_warning = text;
}


status_t BMIMEMultipartMailContainer::AddComponent(BMailComponent *component) {
	if (!_components_in_code.AddItem(component))
		return B_ERROR;
	if (_components_in_raw.AddItem(NULL))
		return B_OK;

	_components_in_code.RemoveItem(component);
	return B_ERROR;
}


BMailComponent *BMIMEMultipartMailContainer::GetComponent(int32 index, bool parse_now) {
	if (index >= CountComponents())
		return NULL;
	
	if (BMailComponent *component = (BMailComponent *)_components_in_code.ItemAt(index))
		return component;	//--- Handle easy case

	message_part *part = (message_part *)(_components_in_raw.ItemAt(index));
	if (part == NULL)
		return NULL;

	_io_data->Seek(part->start,SEEK_SET);

	BMailComponent component (_charSetForTextDecoding);
	if (component.SetToRFC822(_io_data,part->end - part->start) < B_OK)
		return NULL;

	BMailComponent *piece = component.WhatIsThis();

	/* Debug code
	_io_data->Seek(part->start,SEEK_SET);
	char *data = new char[part->end - part->start + 1];
	_io_data->Read(data,part->end - part->start);
	data[part->end - part->start] = 0;
	puts((char *)(data));
	printf("Instantiating from %d to %d (%d octets)\n",part->start, part->end, part->end - part->start);
	*/
	_io_data->Seek(part->start,SEEK_SET);
	if (piece->SetToRFC822(_io_data,part->end - part->start, parse_now) < B_OK)
	{
		delete piece;
		return NULL;
	}
	_components_in_code.ReplaceItem(index,piece);

	return piece;
}


int32
BMIMEMultipartMailContainer::CountComponents() const
{
	return _components_in_code.CountItems();
}


status_t
BMIMEMultipartMailContainer::RemoveComponent(BMailComponent *component)
{
	if (component == NULL)
		return B_BAD_VALUE;

	int32 index = _components_in_code.IndexOf(component);
	if (component == NULL)
		return B_ENTRY_NOT_FOUND;

	delete (BMailComponent *)_components_in_code.RemoveItem(index);
	delete (message_part *)_components_in_raw.RemoveItem(index);

	return B_OK;
}


status_t
BMIMEMultipartMailContainer::RemoveComponent(int32 index)
{
	if (index >= CountComponents())
		return B_BAD_INDEX;

	delete (BMailComponent *)_components_in_code.RemoveItem(index);
	delete (message_part *)_components_in_raw.RemoveItem(index);

	return B_OK;
}


status_t BMIMEMultipartMailContainer::GetDecodedData(BPositionIO *)
{
	return B_BAD_TYPE; //------We don't play dat
}


status_t BMIMEMultipartMailContainer::SetDecodedData(BPositionIO *) {
	return B_BAD_TYPE; //------We don't play dat
}


status_t BMIMEMultipartMailContainer::SetToRFC822(BPositionIO *data, size_t length, bool copy_data)
{
	typedef enum LookingForEnum {
		FIRST_NEWLINE,
		INITIAL_DASHES,
		BOUNDARY_BODY,
		LAST_NEWLINE,
		MAX_LOOKING_STATES
	} LookingFor;

	ssize_t     amountRead;
	ssize_t     amountToRead;
	ssize_t     boundaryLength;
	char        buffer [4096];
	ssize_t     bufferIndex;
	off_t       bufferOffset;
	ssize_t     bufferSize;
	BMessage    content_type;
	const char *content_type_string;
	bool        finalBoundary = false;
	bool        finalComponentCompleted = false;
	int         i;
	off_t       lastBoundaryOffset;
	LookingFor  state;
	off_t       startOfBoundaryOffset;
	off_t       topLevelEnd;
	off_t       topLevelStart;

	// Clear out old components.  Maybe make a MakeEmpty method?

	for (i = _components_in_code.CountItems(); i-- > 0;)
		delete (BMailComponent *)_components_in_code.RemoveItem(i);

	for (i = _components_in_raw.CountItems(); i-- > 0;)
		delete (message_part *)_components_in_raw.RemoveItem(i);

	// Start by reading the headers and getting the boundary string.

	_io_data = data;
	topLevelStart = data->Position();
	topLevelEnd = topLevelStart + length;

	BMailComponent::SetToRFC822(data,length);

	HeaderField("Content-Type",&content_type);
	content_type_string = content_type.FindString("unlabeled");
	if (content_type_string == NULL ||
		strncasecmp(content_type_string,"multipart",9) != 0)
		return B_BAD_TYPE;

	if (!content_type.HasString("boundary"))
		return B_BAD_TYPE;
	free ((void *) _boundary);
	_boundary = strdup(content_type.FindString("boundary"));
	boundaryLength = strlen(_boundary);
	if (boundaryLength > (ssize_t) sizeof (buffer) / 2)
		return B_BAD_TYPE; // Boundary is way too long, should be max 70 chars.

	//	Find container parts by scanning through the given portion of the file
	//	for the boundary marker lines.  The stuff between the header and the
	//	first boundary is ignored, the same as the stuff after the last
	//	boundary.  The rest get stored away as our sub-components.  See RFC2046
	//	section 5.1 for details.

	bufferOffset = data->Position(); // File offset of the start of the buffer.
	bufferIndex = 0; // Current position we are examining in the buffer.
	bufferSize = 0; // Amount of data actually in the buffer, not including NUL.
	startOfBoundaryOffset = -1;
	lastBoundaryOffset = -1;
	state = INITIAL_DASHES; // Starting just after a new line so don't search for it.
	while (((bufferOffset + bufferIndex < topLevelEnd)
		|| (state == LAST_NEWLINE /* No EOF test in LAST_NEWLINE state */))
		&& !finalComponentCompleted)
	{
		// Refill the buffer if the remaining amount of data is less than a
		// boundary's worth, plus four dashes and two CRLFs.
		if (bufferSize - bufferIndex < boundaryLength + 8)
		{
			// Shuffle the remaining bit of data in the buffer over to the front.
			if (bufferSize - bufferIndex > 0)
				memmove (buffer, buffer + bufferIndex, bufferSize - bufferIndex);
			bufferOffset += bufferIndex;
			bufferSize = bufferSize - bufferIndex;
			bufferIndex = 0;

			// Fill up the rest of the buffer with more data.  Also leave space
			// for a NUL byte just past the last data in the buffer so that
			// simple string searches won't go off past the end of the data.
			amountToRead = topLevelEnd - (bufferOffset + bufferSize);
			if (amountToRead > (ssize_t) sizeof (buffer) - 1 - bufferSize)
				amountToRead = sizeof (buffer) - 1 - bufferSize;
			if (amountToRead > 0) {
				amountRead = data->Read (buffer + bufferSize, amountToRead);
				if (amountRead < 0)
					return amountRead;
				bufferSize += amountRead;
			}
			buffer [bufferSize] = 0; // Add an end of string NUL byte.
		}

		// Search for whatever parts of the boundary we are currently looking
		// for in the buffer.  It starts with a newline (officially CRLF but we
		// also accept just LF for off-line e-mail files), followed by two
		// hyphens or dashes "--", followed by the unique boundary string
		// specified earlier in the header, followed by two dashes "--" for the
		// final boundary (or zero dashes for intermediate boundaries),
		// followed by white space (possibly including header style comments in
		// brackets), and then a newline.

		switch (state) {
			case FIRST_NEWLINE:
				// The newline before the boundary is considered to be owned by
				// the boundary, not part of the previous MIME component.
				startOfBoundaryOffset = bufferOffset + bufferIndex;
				if (buffer[bufferIndex] == '\r' && buffer[bufferIndex + 1] == '\n') {
					bufferIndex += 2;
					state = INITIAL_DASHES;
				} else if (buffer[bufferIndex] == '\n') {
					bufferIndex += 1;
					state = INITIAL_DASHES;
				} else
					bufferIndex++;
				break;

			case INITIAL_DASHES:
				if (buffer[bufferIndex] == '-' && buffer[bufferIndex + 1] == '-') {
					bufferIndex += 2;
					state = BOUNDARY_BODY;
				} else
					state = FIRST_NEWLINE;
				break;

			case BOUNDARY_BODY:
				if (strncmp (buffer + bufferIndex, _boundary, boundaryLength) != 0) {
					state = FIRST_NEWLINE;
					break;
				}
				bufferIndex += boundaryLength;
				finalBoundary = false;
				if (buffer[bufferIndex] == '-' && buffer[bufferIndex + 1] == '-') {
					bufferIndex += 2;
					finalBoundary = true;
				}
				state = LAST_NEWLINE;
				break;

			case LAST_NEWLINE:
				// Just keep on scanning until the next new line or end of file.
				if (buffer[bufferIndex] == '\r' && buffer[bufferIndex + 1] == '\n')
					bufferIndex += 2;
				else if (buffer[bufferIndex] == '\n')
					bufferIndex += 1;
				else if (buffer[bufferIndex] != 0 /* End of file is like a newline */) {
					// Not a new line or end of file, just skip over
					// everything.  White space or not, we don't really care.
					bufferIndex += 1;
					break;
				}
				// Got to the end of the boundary line and maybe now have
				// another component to add.
				if (lastBoundaryOffset >= 0) {
					_components_in_raw.AddItem (new message_part (lastBoundaryOffset, startOfBoundaryOffset));
					_components_in_code.AddItem (NULL);
				}
				// Next component's header starts just after the boundary line.
				lastBoundaryOffset = bufferOffset + bufferIndex;
				if (finalBoundary)
					finalComponentCompleted = true;
				state = FIRST_NEWLINE;
				break;

			default: // Should not happen.
				state = FIRST_NEWLINE;
		}
	}

	// Some bad MIME encodings (usually spam, or damaged files) don't put on
	// the trailing boundary.  Dump whatever is remaining into a final
	// component if there wasn't a trailing boundary and there is some data
	// remaining.

	if (!finalComponentCompleted
		&& lastBoundaryOffset >= 0 && lastBoundaryOffset < topLevelEnd) {
		_components_in_raw.AddItem (new message_part (lastBoundaryOffset, topLevelEnd));
		_components_in_code.AddItem (NULL);
	}

	// If requested, actually read the data inside each component, otherwise
	// only the positions in the BPositionIO are recorded.

	if (copy_data) {
		for (i = 0; GetComponent(i, true /* parse_now */) != NULL; i++) {}
	}
	
	data->Seek (topLevelEnd, SEEK_SET);
	return B_OK;
}


status_t BMIMEMultipartMailContainer::RenderToRFC822(BPositionIO *render_to) {
	BMailComponent::RenderToRFC822(render_to);

	BString delimiter;
	delimiter << "\r\n--" << _boundary << "\r\n";

	if (_MIME_message_warning != NULL) {
		render_to->Write(_MIME_message_warning,strlen(_MIME_message_warning));
		render_to->Write("\r\n",2);
	}

	for (int32 i = 0; i < _components_in_code.CountItems() /* both have equal length, so pick one at random */; i++) {
		render_to->Write(delimiter.String(),delimiter.Length());
		if (_components_in_code.ItemAt(i) != NULL) { //---- _components_in_code has precedence

			BMailComponent *code = (BMailComponent *)_components_in_code.ItemAt(i);
			status_t status = code->RenderToRFC822(render_to); //----Easy enough
			if (status < B_OK)
				return status;
		} else {
			// copy message contents

			uint8 buffer[1024];
			ssize_t amountWritten, length;
			message_part *part = (message_part *)_components_in_raw.ItemAt(i);

			for (off_t begin = part->start; begin < part->end; begin += sizeof(buffer)) {
				length = ((part->end - begin) >= sizeof(buffer)) ? sizeof(buffer) : (part->end - begin);

				_io_data->ReadAt(begin,buffer,length);
				amountWritten = render_to->Write(buffer,length);
				if (amountWritten < 0)
					return amountWritten; // IO error of some sort.
			}
		}
	}

	render_to->Write(delimiter.String(),delimiter.Length() - 2);	// strip CRLF
	render_to->Write("--\r\n",4);

	return B_OK;
}

void BMIMEMultipartMailContainer::_ReservedMultipart1() {}
void BMIMEMultipartMailContainer::_ReservedMultipart2() {}
void BMIMEMultipartMailContainer::_ReservedMultipart3() {}

void BMailContainer::_ReservedContainer1() {}
void BMailContainer::_ReservedContainer2() {}
void BMailContainer::_ReservedContainer3() {}
void BMailContainer::_ReservedContainer4() {}

