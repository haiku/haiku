/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "LibRAW.h"

#include <Message.h>
#include <TranslationErrors.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libraw/libraw.h>

#include "StreamBuffer.h"


//#define TRACE(x...) printf(x)
#define TRACE(x...)
#define CALLED() TRACE("CALLED %s\n", __PRETTY_FUNCTION__)
#define RETURN(x, y) { TRACE("RETURN %s %s\n", __PRETTY_FUNCTION__, x); \
	return y; }

static const uint32 kImageBufferCount = 10;
static const uint32 kDecodeBufferCount = 2048;


#define P1 fRaw->imgdata.idata
#define S fRaw->imgdata.sizes
#define C fRaw->imgdata.color
#define T fRaw->imgdata.thumbnail
#define P2 fRaw->imgdata.other
#define OUT fRaw->imgdata.params


//	#pragma mark -


class LibRaw_haiku_datastream : public LibRaw_abstract_datastream
{
public:
	BPositionIO* fStream;
	virtual ~LibRaw_haiku_datastream();
			LibRaw_haiku_datastream(BPositionIO* stream);
	virtual int valid();
	virtual int read(void *ptr, size_t size, size_t nmemb);
	virtual int eof();
	virtual int seek(INT64 o, int whence);
	virtual INT64 tell();
	virtual INT64 size();
	virtual int get_char();
	virtual char *gets(char *str, int sz);
	virtual int scanf_one(const char *fmt, void *val);
	virtual void *make_jas_stream();

private:
	StreamBuffer *buffer;
	off_t	fSize;
	status_t iseof;
};


LibRaw_haiku_datastream::LibRaw_haiku_datastream(BPositionIO* stream)
{
	CALLED();
	iseof = 0;
	buffer = new StreamBuffer(stream, 2048);
	fStream = stream;
	fStream->GetSize(&fSize);
}

LibRaw_haiku_datastream::~LibRaw_haiku_datastream()
{
	delete buffer;
}


int
LibRaw_haiku_datastream::read(void *ptr, size_t sz, size_t nmemb)
{
	CALLED();
	TRACE("read %ld %ld\n", sz, nmemb);
	size_t to_read = sz * nmemb;

	to_read = buffer->Read(ptr, to_read);
	if (to_read < B_OK)
		RETURN("ERROR2", to_read);
	return int((to_read + sz - 1) / (sz > 0 ? sz : 1));
}


int
LibRaw_haiku_datastream::seek(INT64 o, int whence)
{
	CALLED();
	TRACE("seek %lld %d\n", o, whence);
	return buffer->Seek(o, whence) < 0;
}


INT64
LibRaw_haiku_datastream::tell()
{
	CALLED();
	off_t position = buffer->Position();
	TRACE("RETURN LibRaw_haiku_datastream::tell %ld\n", position);
	return position;
}


INT64
LibRaw_haiku_datastream::size()
{
	CALLED();
	TRACE("RETURN size %ld\n", fSize);
	return fSize;
}


int
LibRaw_haiku_datastream::get_char()
{
	CALLED();
	unsigned char value;
	ssize_t error;
	iseof = 0;
	error = buffer->Read((void*)&value, sizeof(unsigned char));
	if (error >= 0)
		return value;
	iseof = EOF;
	return EOF;
}


char*
LibRaw_haiku_datastream::gets(char* s, int sz)
{
	CALLED();
	if (sz<1)
		return NULL;
	int pos = 0;
	bool found = false ;
	while (!found && pos < (sz - 1)) {
		char buffer;
		if (this->buffer->Read(&buffer, 1) < 1)
			break;
		s[pos++] = buffer ;
		if (buffer == '\n') {
			found = true;
			break;
		}
	}
	if (found) {
		s[pos] = 0;
	} else
		s[sz - 1] = 0 ;
	return s;
}


int
LibRaw_haiku_datastream::scanf_one(const char *fmt, void *val)
{
	CALLED();
  int scanf_res = 0;
/*  if (streampos > streamsize)
    return 0;
  scanf_res = sscanf_s((char *)(buf + streampos), fmt, val);
  if (scanf_res > 0)
  {
    int xcnt = 0;
    while (streampos < streamsize)
    {
      streampos++;
      xcnt++;
      if (buf[streampos] == 0 || buf[streampos] == ' ' ||
          buf[streampos] == '\t' || buf[streampos] == '\n' || xcnt > 24)
        break;
    }
  }*/
  return scanf_res;
}


int
LibRaw_haiku_datastream::eof()
{
	CALLED();
	return iseof;
}


int
LibRaw_haiku_datastream::valid()
{
	CALLED();
	return buffer != NULL ? 1 : 0;
}


void *
LibRaw_haiku_datastream::make_jas_stream()
{
	return NULL;
}


//	#pragma mark -


LibRAW::LibRAW(BPositionIO& stream)
	:
	fProgressMonitor(NULL),
	fRaw(new LibRaw()),
	fDatastream(new LibRaw_haiku_datastream(&stream))
{
	CALLED();
}


LibRAW::~LibRAW()
{
	delete fRaw;
	delete fDatastream;
}



//	#pragma mark -


status_t
LibRAW::Identify()
{
	CALLED();
	fDatastream->fStream->Seek(0, SEEK_SET);

	status_t status = B_NO_TRANSLATOR;
	int ret = fRaw->open_datastream(fDatastream);
	if (ret != LIBRAW_SUCCESS) {
		TRACE("LibRAW::Identify() open_datastream returned %s\n",
			libraw_strerror(ret));
		return status;
	}

	return B_OK;
}


status_t
LibRAW::ReadImageAt(uint32 index, uint8*& outputBuffer, size_t& bufferSize)
{
	CALLED();
	if (index >= P1.raw_count)
		return B_BAD_VALUE;
	OUT.shot_select = index;
	OUT.output_bps = 8;
	OUT.output_tiff = 1;
	OUT.no_auto_bright = 1;
	OUT.use_auto_wb = 1;
	OUT.user_qual = 3;

	if (fRaw->unpack() != LIBRAW_SUCCESS)
		return B_BAD_DATA;
	if (fRaw->dcraw_process() != LIBRAW_SUCCESS)
		return B_BAD_DATA;

	libraw_processed_image_t* img = fRaw->dcraw_make_mem_image();
	if (img == NULL)
		return B_BAD_DATA;
	bufferSize = img->data_size;
	outputBuffer = (uint8*)malloc(bufferSize);
	if (outputBuffer == NULL) {
		fRaw->dcraw_clear_mem(img);
		throw (status_t)B_NO_MEMORY;
	}

	memcpy(outputBuffer, img->data, bufferSize);

	fRaw->dcraw_clear_mem(img);
	return B_OK;
}


void
LibRAW::GetMetaInfo(image_meta_info& metaInfo) const
{
	strlcpy(metaInfo.manufacturer, fRaw->imgdata.idata.make,
		sizeof(metaInfo.manufacturer));
	strlcpy(metaInfo.model, fRaw->imgdata.idata.model, sizeof(metaInfo.model));
}


uint32
LibRAW::CountImages() const
{
	return fRaw->imgdata.idata.raw_count;
}


status_t
LibRAW::ImageAt(uint32 index, image_data_info& info) const
{
	if (index >= fRaw->imgdata.idata.raw_count)
		return B_BAD_VALUE;

	info.width = S.width;
	info.height = S.height;
	info.output_width = S.flip > 4 ? S.iheight : S.iwidth;
	info.output_height = S.flip > 4 ? S.iwidth : S.iheight;
	info.flip = S.flip;
	info.bytes = S.raw_pitch;
	info.is_raw = 1;
	return B_OK;
}


status_t
LibRAW::GetEXIFTag(off_t& offset, size_t& length, bool& bigEndian) const
{
	return B_ENTRY_NOT_FOUND;
}


void
LibRAW::SetProgressMonitor(monitor_hook hook, void* data)
{
	fProgressMonitor = hook;
	fProgressData = data;
	fRaw->set_progress_handler(hook != NULL ? ProgressCallback : NULL, this);
}


void
LibRAW::SetHalfSize(bool half)
{
	OUT.half_size = half;
}


int
LibRAW::ProgressCallback(void *data, enum LibRaw_progress p, int iteration,
	int expected)
{
	return ((LibRAW*)data)->_ProgressCallback(p, iteration, expected);
}


int
LibRAW::_ProgressCallback(enum LibRaw_progress p,int iteration, int expected)
{
	fProgressMonitor(libraw_strprogress(p), iteration * 100 / expected,
		fProgressData);
	return 0;
}
