
// this header is similar to functions found in libmedia.so 
// and the media_kit extractor add-ons
// we will use it as basic information, but are not trying to
// create a binary compatible interface
// this API will totally change, as we are creating our own API

namespace media_kit_private {

class Extractor
{
public:
	Extractor(void);
	virtual ~Extractor(void);

	virtual status_t Copyright(void);
	virtual status_t Extractor(void);
	virtual status_t Perform(long, void *);
	virtual status_t SniffFormat(media_format const *, long *, long *);
	virtual status_t Source(void) const;

	virtual status_t find_stream_extractor(long *, media_format *, BPrivate::Extractor *, long, long *, long *);

	virtual status_t Sniff(long *, long *) = 0;
	virtual status_t GetDuration(long, long long *) = 0;
	virtual status_t SplitNext(long, void *, long long *, char *, long *, char **, long *, media_header *) = 0;
	virtual status_t Seek(long, void *, long, long, long long *, long long *, long long *, char *, long *, bool *) = 0;
	virtual status_t FreeCookie(long, void *) = 0;
	virtual status_t AllocateCookie(long, void **) = 0;
	virtual status_t CountFrames(long, long long *) = 0;
	virtual status_t TrackInfo(long, media_format *, void **, long *) = 0;
	virtual status_t GetFileFormatInfo(media_file_format *) = 0;

private:
	virtual status_t Perform(long, void *);
	//virtual function and data stuffing will be done later
};

}; // namespace media_kit_private

extern "C" {
	status_t instantiate_extractor();
}


