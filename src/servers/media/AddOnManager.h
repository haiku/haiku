#ifndef _ADD_ON_MANAGER_H
#define _ADD_ON_MANAGER_H

// Manager for codec add-ons (reader, writer, encoder, decoder)
// BMediaAddOn are handled in the NodeManager

#include "TList.h"
#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include "DataExchange.h"

class AddOnManager
{
public:
				AddOnManager();
				~AddOnManager();
				
	void		LoadState();
	void		SaveState();
	
	status_t 	GetDecoderForFormat(xfer_entry_ref *out_ref,
									const media_format &in_format);
									
	status_t	GetReaders(xfer_entry_ref *out_res, int32 *out_count, int32 max_count);

	status_t	PublishDecoderCallback(DecoderPlugin *decoderplugin,
									   const char *meta_description,
									   const char *short_name,
									   const char *pretty_name,
									   const char *default_mapping /* = 0 */);

private:
	void		RegisterAddOns();
	void		RegisterReader(ReaderPlugin *reader, const entry_ref &ref);
	void		RegisterDecoder(DecoderPlugin *decoder, const entry_ref &ref);


	
private:
	struct reader_info
	{
		entry_ref ref;
	};
	struct writer_info
	{
		entry_ref ref;
	};
	struct decoder_info
	{
		entry_ref ref;
		List<media_meta_description> meta_desc;
	};
	struct encoder_info
	{
		entry_ref ref;
	};

	BLocker 	*fLocker;
	List<reader_info> *fReaderList;
	List<writer_info> *fWriterList;
	List<decoder_info> *fDecoderList;
	List<encoder_info> *fEncoderList;

	reader_info *fReaderInfo;
	writer_info *fWriterInfo;
	decoder_info *fDecoderInfo;
	encoder_info *fEncoderInfo;

};


#endif // _ADD_ON_MANAGER_H
