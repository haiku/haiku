
// this header is similar to functions found in libmedia.so 
// and the media_kit decoder add-ons
// we will use it as basic information, but are not trying to
// create a binary compatible interface
// this API will totally change, as we are creating our own API

#include <MediaDefs.h>
#include <MediaFormats.h>

namespace media_kit_private {

class Decoder
{
public:
	virtual ~Decoder(void); /* this needs to be virtual */
	Decoder(void);

	/* not sure if all three need to be virtual */
	virtual status_t SetTrack(BMediaTrack *track);
	/* what does this one do? strange parameters */
	virtual status_t Reset(long, long long, long long *, long long, long long *);
	virtual status_t GetNextChunk(void const **chunkData, size_t *chunkSize, 
								  media_header *mh, media_decode_info *mdi);

	virtual status_t Decode(void *out_buffer, int64 *out_frameCount,
							media_header *out_mh, media_decode_info *info) = 0;
	virtual status_t Format(media_format *) = 0;
	virtual status_t Sniff(media_format const *, void const *in_buffer, unsigned long in_buffersize) = 0;
	virtual status_t GetCodecInfo(media_codec_info *out_mci) const = 0;

private:
	virtual status_t Perform(long, void *);
	//virtual function and data stuffing will be done later
};

}; // namespace media_kit_private


extern "C" status_t register_decoder();
extern "C" BPrivate::Decoder * instantiate_decoder();

