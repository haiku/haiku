
// this header is similar to functions found in libmedia.so 
// and the media_kit encoder add-ons
// we will use it as basic information, but are not trying to
// create a binary compatible interface
// this API will totally change, as we are creating our own API

namespace media_kit_private {

class Encoder
{
public:
	virtual ~Encoder(void); /* this needs to be virtual */
	Encoder(void);

	/* not sure if all these need to be virtual */
	virtual status_t Flush(void);
	virtual status_t StartEncoder(void);
	virtual status_t SetEncodeParameters(encode_parameters *);
	virtual status_t GetEncodeParameters(encode_parameters *) const;
	virtual status_t GetParameterView(void);
	virtual status_t SetParameterValue(long, void const *, unsigned long);
	virtual status_t GetParameterValue(long, void *, unsigned long *);
	virtual status_t WriteChunk(void const *, unsigned long, media_encode_info *);
	virtual status_t AddTrackInfo(unsigned long, char const *, unsigned long);
	virtual status_t AddCopyright(char const *);
	virtual status_t Web(void);
	virtual status_t AttachedToTrack(void);
	virtual status_t SetTrack(BMediaTrack *);
	
	virtual status_t GetCodecInfo(media_codec_info *) const = 0;
	virtual status_t Encode(void const *, long long, media_encode_info *) = 0;
	virtual status_t SetFormat(media_file_format *, media_format *, media_format *) = 0;

private:
	virtual status_t Perform(long, void *);
	//virtual function and data stuffing will be done later
};

}; // namespace media_kit_private

extern "C" status_t register_encoder();
extern "C" BPrivate::Encoder *instantiate_nth_encoder();
extern "C" BPrivate::Encoder *instantiate_encoder();
