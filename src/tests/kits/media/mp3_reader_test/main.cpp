#include <OS.h>
#include <File.h>
#include <stdio.h>
#include <DataIO.h>
#include <string.h>
#include "MediaPlugin.h"
#include "ReaderPlugin.h"
#include "FileDataIO.h"

const bool		SEEKABLE = 1;
const char *	FILENAME = "/boot/home/Desktop/mediastuff/Apocalyptica Feat. Nina Hagen - Seemann.mp3";

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

void *cookies[100];

int main(int argc, char *argv[])
{
	TRACE("\n\n");
	
	TRACE("main: creating plugin...\n");

	MediaPlugin *plugin = instantiate_plugin();

	TRACE("main: creating reader...\n");
	
	ReaderPlugin *readerplugin;
	readerplugin = dynamic_cast<ReaderPlugin *>(plugin);
	
	Reader *reader = readerplugin->NewReader();
	
	TRACE("main: opening source file...\n");
	
	BDataIO *source;
	if (SEEKABLE)
		source = new BFile(argc == 2 ? argv[1] : FILENAME, O_RDWR);
	else
		source = new FileDataIO(argc == 2 ? argv[1] : FILENAME, O_RDWR);

	TRACE("main: calling setup...\n");

	reader->Setup(source);
	
	TRACE("main: creating ...\n");

	TRACE("main: copyright: \"%s\"\n", reader->Copyright());
	
	status_t s;
	int32 streamCount;
	
	s = reader->Sniff(&streamCount);
	if (s != B_OK) {
		TRACE("main: Sniff() failed with error %lx (%s)\n", s, strerror(s));
		goto err;
	}

	TRACE("main: Sniff() found %ld streams\n", streamCount);


	for (int i = 0; i < streamCount; i++) {
		TRACE("main: calling AllocateCookie(stream = %d)\n", i);
		s = reader->AllocateCookie(i, &cookies[i]);
		if (s != B_OK) {
			TRACE("main: AllocateCookie(stream = %d) failed with error %lx (%s)\n", i,  s, strerror(s));
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
		s = reader->GetStreamInfo(cookies[i], &frameCount, &duration, &format, &infoBuffer, &infoSize);
		if (s != B_OK) {
			TRACE("main: GetStreamInfo(stream = %d) failed with error %lx (%s)\n", i,  s, strerror(s));
			goto err;
		}
		TRACE("main: GetStreamInfo(stream = %d) result: %Ld frames, %.6f sec\n", i, frameCount, duration / 1000000.0);
	}

	for (int i = 0; i < streamCount; i++) {
		const void *chunkBuffer;
		size_t chunkSize;
		media_header mediaHeader;
		for (int j = 0; j < 5; j++) {
			s = reader->GetNextChunk(cookies[i], &chunkBuffer, &chunkSize, &mediaHeader);
			if (s != B_OK) {
				TRACE("main: GetNextChunk(stream = %d, chunk = %d) failed with error %lx (%s)\n", i, j,  s, strerror(s));
				break;
			}
		}

		int64 frame;
		bigtime_t time;
		
		time = 1000000; // 1 sec
		TRACE("main: calling Seek(stream = %d, time %.6f forward)\n", i, time / 1000000.0);
		s = reader->Seek(cookies[i], B_MEDIA_SEEK_TO_TIME | B_MEDIA_SEEK_CLOSEST_FORWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		frame = 1000;
		TRACE("main: calling Seek(stream = %d, frame %Ld forward)\n", i, frame);
		s = reader->Seek(cookies[i], B_MEDIA_SEEK_TO_FRAME | B_MEDIA_SEEK_CLOSEST_FORWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		time = 1000000; // 1 sec
		TRACE("main: calling Seek(stream = %d, time %.6f backward)\n", i, time / 1000000.0);
		s = reader->Seek(cookies[i], B_MEDIA_SEEK_TO_TIME | B_MEDIA_SEEK_CLOSEST_BACKWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);

		frame = 1000;
		TRACE("main: calling Seek(stream = %d, frame %Ld backward)\n", i, frame);
		s = reader->Seek(cookies[i], B_MEDIA_SEEK_TO_FRAME | B_MEDIA_SEEK_CLOSEST_BACKWARD, &frame, &time);
		TRACE("main: Seek result: time %.6f, frame %Ld\n", time / 1000000.0, frame);
	}

	for (int i = 0; i < streamCount; i++) {
		TRACE("main: calling FreeCookie(stream = %d)\n", i);
		s = reader->FreeCookie(cookies[i]);
		if (s != B_OK) {
			TRACE("main: FreeCookie(stream = %d) failed with error %lx (%s)\n", i,  s, strerror(s));
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
