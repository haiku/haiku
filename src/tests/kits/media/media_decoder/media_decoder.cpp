#include <File.h>
#include <MediaDecoder.h>
#include <MediaTrack.h>
#include <MediaFile.h>
#include <StorageDefs.h>
#include <stdio.h>
#include <string.h>

class FileDecoder : public BMediaDecoder {
private:
	BMediaTrack * track;
	char buffer[8192];
public:
	FileDecoder(BMediaTrack * _track, const media_format *inFormat,
                const void *info = NULL, size_t infoSize = 0) 
    : BMediaDecoder(inFormat,info,infoSize) {
	 	track = _track;
	}
protected:
	virtual status_t GetNextChunk(const void **chunkData, size_t *chunkLen,
		                              media_header *mh) {
		memset(mh,0,sizeof(media_header));
		status_t result = track->ReadChunk((char**)chunkData,(int32*)chunkLen,mh);
		const void * data = *chunkData;
		(void)data;
		return result;
	}
};

int main (int argc, const char ** argv) {
	if (argc == 0) {
		return -1;
	}
	if (argc < 3) {
		fprintf(stderr,"%s: invalid usage\n",argv[0]);
		fprintf(stderr,"supply an input file and an output file:\n");
		fprintf(stderr,"  media_decoder input.mp3 output.raw\n");
		return -1;
	}
	// open the file using BMediaFile
	BFile * file = new BFile(argv[1],B_READ_ONLY);
	BMediaFile * mf = new BMediaFile(file);
	if (mf->CountTracks() == 0) {
		fprintf(stderr,"no tracks found in %s\n",argv[1]);
		return -1;
	}
	media_format format;
	memset(&format,0,sizeof(format));
	// find an audio track
	BMediaTrack * track = 0;
	for (int i = 0; i < mf->CountTracks() ; i++) {
		track = mf->TrackAt(i);
		track->EncodedFormat(&format);
		if (format.IsAudio()) {
			break;
		}
		track = 0;
	}
	if (track == 0) {
		fprintf(stderr,"no audio stream found in %s\n",argv[1]);
		return -1;	
	}
	// create a BMediaDecoder and initialize it
	FileDecoder * fd = new FileDecoder(track,&format);
//	fd->SetInputFormat(&format);
	memset(&format,0,sizeof(format));
	track->DecodedFormat(&format);
	fd->SetOutputFormat(&format);

	// open the output file
	BFile * file2 = new BFile(argv[2],B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);

	// decode until we hit an error
	uint8 * buffer = new uint8[format.u.raw_audio.buffer_size];
	int64 size = 0;
	media_header mh;
	memset(&mh,0,sizeof(mh));
	while (fd->Decode((void*)buffer,&size,&mh,0) == B_OK) {
		file2->Write(buffer,format.u.raw_audio.buffer_size);
	}
	return 0;
}
