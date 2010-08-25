/*
 * Copyright 2004-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marcus Overhagen
 *		Axel Dörfler
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef _ADD_ON_MANAGER_H
#define _ADD_ON_MANAGER_H


/*!	Manager for codec add-ons (reader, writer, encoder, decoder)
	BMediaAddOn are handled in the NodeManager
*/


#include "DataExchange.h"
#include "TList.h"

#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"
#include "DecoderPlugin.h"
#include "EncoderPlugin.h"
#include "ReaderPlugin.h"
#include "WriterPlugin.h"


class AddOnManager {
public:
								AddOnManager();
								~AddOnManager();

			void				LoadState();
			void				SaveState();

			status_t			GetDecoderForFormat(xfer_entry_ref* _ref,
									const media_format& format);

			status_t			GetReaders(xfer_entry_ref* _ref,
									int32* _count, int32 maxCount);

			status_t			GetEncoder(xfer_entry_ref* _ref, int32 id);

			status_t			GetWriter(xfer_entry_ref* _ref,
									uint32 internalID);

			status_t			GetFileFormat(media_file_format* _fileFormat,
									int32 cookie);
			status_t			GetCodecInfo(media_codec_info* _codecInfo,
									media_format_family* _formatFamily,
									media_format* _inputFormat,
									media_format* _outputFormat, int32 cookie);

private:
			void				_RegisterAddOns();

			status_t			_RegisterAddOn(const entry_ref& ref);
			status_t			_UnregisterAddOn(const entry_ref& ref);

			void				_RegisterReader(ReaderPlugin* reader,
									const entry_ref& ref);
			void				_RegisterDecoder(DecoderPlugin* decoder,
									const entry_ref& ref);

			void				_RegisterWriter(WriterPlugin* writer,
									const entry_ref& ref);
			void				_RegisterEncoder(EncoderPlugin* encoder,
									const entry_ref& ref);

			bool				_FindDecoder(const media_format& format,
									const BPath& path,
									xfer_entry_ref* _decoderRef);
			void				_GetReaders(const BPath& path,
									xfer_entry_ref* outRefs, int32* outCount,
									int32 maxCount);

private:
			struct reader_info {
				entry_ref			ref;
			};
			struct writer_info {
				entry_ref			ref;
				uint32				internalID;
			};
			struct decoder_info {
				entry_ref			ref;
				List<media_format>	formats;
			};
			struct encoder_info {
				entry_ref			ref;
				uint32				internalID;
				media_codec_info	codecInfo;
				media_format_family	formatFamily;
				media_format		intputFormat;
				media_format		outputFormat;
			};

			BLocker				fLock;
			List<reader_info>	fReaderList;
			List<writer_info>	fWriterList;
			List<decoder_info>	fDecoderList;
			List<encoder_info>	fEncoderList;

			List<media_file_format> fWriterFileFormats;

			uint32				fNextWriterFormatFamilyID;
			uint32				fNextEncoderCodecInfoID;

			AddOnMonitorHandler* fAddOnMonitorHandler;
			AddOnMonitor*		fAddOnMonitor;
};

#endif // _ADD_ON_MANAGER_H
