
// this header is similar to functions found in libmedia.so 
// and the media_kit writer add-ons
// we will use it as basic information, but are not trying to
// create a binary compatible interface
// this API will totally change, as we are creating our own API


namespace media_kit_private {

class Writer
{
public:
	Writer(void)
	virtual ~Writer(void);
	virtual status_t SetWriteBufferSize(unsigned long);

	virtual status_t SetSource(BDataIO *) = 0;
	virtual status_t AddTrack(BMediaTrack *) = 0;
	virtual status_t CommitHeader(void) = 0;
	virtual status_t WriteData(long, media_type, void const *, unsigned long, media_encode_info *) = 0;
	virtual status_t CloseFile(void) = 0;
	virtual status_t AddTrackInfo(long, unsigned long, char const *, unsigned long) = 0;
	virtual status_t AddCopyright(char const *) = 0;
	virtual status_t AddChunk(long, char const *, unsigned long) = 0;

private:
	virtual status_t Perform(long, void *);
	//virtual function and data stuffing will be done later
};

}; // namespace media_kit_private

extern "C" {
	status_t accepts_format();
	status_t get_mediawriter_info();
	status_t instantiate_mediawriter();                  
};
