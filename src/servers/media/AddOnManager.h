/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Authors: Marcus Overhagen, Axel Dörfler
*/
#ifndef _ADD_ON_MANAGER_H
#define _ADD_ON_MANAGER_H

// Manager for codec add-ons (reader, writer, encoder, decoder)
// BMediaAddOn are handled in the NodeManager

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include "DataExchange.h"
#include "TList.h"

#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"

class AddOnManager {
	public:
		AddOnManager();
		~AddOnManager();

		void		LoadState();
		void		SaveState();

		status_t 	GetDecoderForFormat(xfer_entry_ref *out_ref,
						const media_format &in_format);

		status_t	GetReaders(xfer_entry_ref *out_res, int32 *out_count,
						int32 max_count);

	private:
		status_t	RegisterAddOn(BEntry &entry);
		void		RegisterAddOns();

		void		RegisterReader(ReaderPlugin *reader, const entry_ref &ref);
		void		RegisterDecoder(DecoderPlugin *decoder, const entry_ref &ref);

	private:
		struct reader_info {
			entry_ref ref;
		};
		struct writer_info {
			entry_ref ref;
		};
		struct decoder_info {
			entry_ref ref;
			List<media_format> formats;
		};
		struct encoder_info {
			entry_ref ref;
		};

		BLocker fLock;
		List<reader_info> fReaderList;
		List<writer_info> fWriterList;
		List<decoder_info> fDecoderList;
		List<encoder_info> fEncoderList;

		decoder_info	*fCurrentDecoder;

		AddOnMonitorHandler	*fHandler;
		AddOnMonitor		*fAddOnMonitor;
};

#endif // _ADD_ON_MANAGER_H
