/* 
** Copyright 2004, Marcus Overhagen. All rights reserved.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
**
** Distributed under the terms of the MIT License.
*/


#include "MediaPlugin.h"
#include "ReaderPlugin.h"
#include "MediaExtractor.h"

#include <OS.h>
#include <File.h>

#include <string.h>
#include <stdio.h>


#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

void *cookies[100];


int
main(int argc, char *argv[])
{
	if (argc < 2) {
		// missing argument
		char *name = strrchr(argv[0], '/');
		name = name ? name + 1 : argv[0];
		fprintf(stderr, "usage: %s <media-file>\n", name);
		return -1;
	}

	TRACE("\n\n");
	
	TRACE("main: creating plugin...\n");

	MediaPlugin *plugin = instantiate_plugin();

	TRACE("main: creating reader...\n");
	
	ReaderPlugin *readerplugin;
	readerplugin = dynamic_cast<ReaderPlugin *>(plugin);
	
	Reader *reader = readerplugin->NewReader();
	
	TRACE("main: opening source file...\n");

	BDataIO *source = new BFile(argv[1], B_READ_ONLY);

	TRACE("main: calling setup...\n");

	reader->Setup(source);

	TRACE("main: creating ...\n");

	TRACE("main: copyright: \"%s\"\n", reader->Copyright());

	status_t status;
	int32 streamCount;

	status = reader->Sniff(&streamCount);
	if (status != B_OK) {
		TRACE("main: Sniff() failed with error %lx (%s)\n", status, strerror(status));
		goto err;
	}

	TRACE("main: Sniff() found %ld streams\n", streamCount);

	for (int i = 0; i < streamCount; i++) {
		TRACE("main: calling AllocateCookie(stream = %d)\n", i);
		status = reader->AllocateCookie(i, &cookies[i]);
		if (status != B_OK) {
			TRACE("main: AllocateCookie(stream = %d) failed with error 0x%lx (%s)\n",
				i, status, strerror(status));
			goto err;
		}
	}

	for (int i = 0; i < streamCount; i++) {
		TRACE("main: calling GetStreamInfo(stream = %d)\n", i);
		int64 frameCount;
		bigtime_t duration;
		media_format format;
		const void *infoBuffer;
		size_t infoSize;
		status = reader->GetStreamInfo(cookies[i], &frameCount, &duration, &format,
			&infoBuffer, &infoSize);
		if (status != B_OK) {
			TRACE("main: GetStreamInfo(stream = %d) failed with error 0x%lx (%s)\n",
				i, status, strerror(status));
			goto err;
		}
		TRACE("main: GetStreamInfo(stream = %d) result: %Ld frames, %.6f sec\n",
			i, frameCount, duration / 1000000.0);
	}

	for (int i = 0; i < streamCount; i++) {
		const void *chunkBuffer;
		size_t chunkSize;
		media_header mediaHeader;
		for (int j = 0; j < 5; j++) {
			status = reader->GetNextChunk(cookies[i], &chunkBuffer, &chunkSize, &mediaHeader);
			if (status != B_OK) {
				TRACE("main: GetNextChunk(stream = %d, chunk = %d) failed with error 0x%lx (%s)\n",
					i, j, status, strerror(status));
				break;
			}
		}

		int64 frame;
		bigtime_t time;
		
		time = 1000000; // 1 sec
		TRACE("main: calling Seek(stream = %d, time %.6f forward)\n", i, time / 1000000.0);
		status = reader->Seek(cookies[i],
			B_MEDIA_SEEK_TO_TIME | B_MEDIA_SEEK_CLOSEST_FORWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		frame = 1000;
		TRACE("main: calling Seek(stream = %d, frame %Ld forward)\n", i, frame);
		status = reader->Seek(cookies[i],
			B_MEDIA_SEEK_TO_FRAME | B_MEDIA_SEEK_CLOSEST_FORWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		time = 1000000; // 1 sec
		TRACE("main: calling Seek(stream = %d, time %.6f backward)\n", i, time / 1000000.0);
		status = reader->Seek(cookies[i],
			B_MEDIA_SEEK_TO_TIME | B_MEDIA_SEEK_CLOSEST_BACKWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		frame = 1000;
		TRACE("main: calling Seek(stream = %d, frame %Ld backward)\n", i, frame);
		status = reader->Seek(cookies[i],
			B_MEDIA_SEEK_TO_FRAME | B_MEDIA_SEEK_CLOSEST_BACKWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);
	}

	for (int i = 0; i < streamCount; i++) {
		TRACE("main: calling FreeCookie(stream = %d)\n", i);
		status = reader->FreeCookie(cookies[i]);
		if (status != B_OK) {
			TRACE("main: FreeCookie(stream = %d) failed with error 0x%lx (%s)\n",
				i, status, strerror(status));
			goto err;
		}
	}

err:
	delete reader;
	delete plugin;
	delete source;

	return 0;
}


status_t
_get_format_for_description(media_format *out_format, const media_format_description &in_desc)
{
	return B_OK;
}

namespace BPrivate {
namespace media {

status_t
MediaExtractor::GetNextChunk(int32 stream,
	const void **chunkBuffer, size_t *chunkSize,
	media_header *mediaHeader)
{
	return B_ERROR;
}

} // namespace media
} // namespace BPrivate
