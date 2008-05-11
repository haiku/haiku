/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "exif_parser.h"

#include <stdio.h>
#include <string.h>

#include <BufferIO.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>

#include "ReadHelper.h"
#include "TIFF.h"


static status_t
process_exif(uint8* data, uint32 length)
{
	if (memcmp(data + 2, "Exif", 4))
		return B_BAD_TYPE;

	BMemoryIO source(data + 8, length - 8);
	BMessage exif;
	status_t status = convert_exif_to_message(source, exif);

	exif.PrintToStream();
		// even if it failed, some data might end up in the message

	return status;
}


static status_t
process_jpeg(BPositionIO& stream)
{
	enum {
		START_OF_IMAGE_MARKER = 0xd8,
		EXIF_MARKER = 0xe1
	};

	uint8 header[2];
	if (stream.Read(&header, 2) != 2)
		return B_IO_ERROR;
	if (header[0] != 0xff || header[1] != START_OF_IMAGE_MARKER)
		return B_BAD_TYPE;

	while (true) {
		// read marker
		uint8 marker;
		for (int32 i = 0; i < 7; i++) {
			if (stream.Read(&marker, 1) != 1)
				return B_BAD_TYPE;

			if (marker != 0xff)
				break;
		}

		if (marker == 0xff)
			return B_BAD_TYPE;

		// get length of section
		
		uint16 length;
		if (stream.Read(&length, 2) != 2)
			return B_BAD_TYPE;

		swap_data(B_UINT16_TYPE, &length, 2, B_SWAP_BENDIAN_TO_HOST);

		if (marker == EXIF_MARKER) {
			// read in section
			stream.Seek(-2, SEEK_CUR);

			uint8 exifData[length];
			if (stream.Read(exifData, length) == length
				&& process_exif(exifData, length) == B_OK)
				return B_OK;
		} else {
			// ignore section
			stream.Seek(length - 2, SEEK_CUR);
		}
	}

	return B_BAD_VALUE;
}


static status_t
process_file(entry_ref& ref)
{
	BFile file(&ref, B_READ_WRITE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	// read EXIF

	BBufferIO stream(&file, 65536, false);
	status = process_jpeg(file);
	if (status < B_OK) {
		fprintf(stderr, "%s: processing JPEG file failed: %s\n", ref.name,
			strerror(status));
		return status;
	}

	return B_OK;
}


int
main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		BEntry entry(argv[i]);
		entry_ref ref;
		if (entry.InitCheck() != B_OK || entry.GetRef(&ref) != B_OK)
			continue;

		process_file(ref);
	}
	return -1; 
}
