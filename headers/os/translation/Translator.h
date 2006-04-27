/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H


#include <Archivable.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>


class BTranslator : public BArchivable {
	public:
		BTranslator();

		BTranslator* Acquire();
		BTranslator* Release();

		int32 ReferenceCount();
			// returns the refcount, THIS IS NOT THREAD SAFE, ITS ONLY FOR "FUN"

		virtual const char *TranslatorName() const = 0;
		virtual const char *TranslatorInfo() const = 0;
		virtual int32 TranslatorVersion() const = 0;

		virtual const translation_format *InputFormats(int32 *out_count)
			const = 0;
		virtual const translation_format *OutputFormats(int32 *out_count)
			const = 0;
		virtual status_t Identify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType) = 0;
		virtual status_t Translate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination) = 0;
		virtual status_t MakeConfigurationView(BMessage *ioExtension,
			BView **outView, BRect *outExtent);
		virtual status_t GetConfigurationMessage(BMessage *ioExtension);

	protected:
		virtual ~BTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user

	private:
		friend class BTranslatorRoster::Private;

		virtual status_t _Reserved_Translator_0(int32, void *);
		virtual status_t _Reserved_Translator_1(int32, void *);
		virtual status_t _Reserved_Translator_2(int32, void *);
		virtual status_t _Reserved_Translator_3(int32, void *);
		virtual status_t _Reserved_Translator_4(int32, void *);
		virtual status_t _Reserved_Translator_5(int32, void *);
		virtual status_t _Reserved_Translator_6(int32, void *);
		virtual status_t _Reserved_Translator_7(int32, void *);

		BTranslatorRoster::Private* fOwningRoster;
		translator_id	fID;
		int32			fRefCount;

		uint32			_reserved[7];
};

// The post-4.5 API suggests implementing this function in your translator
// add-on rather than the separate functions and variables of the previous
// API. You will be called for values of n starting at 0 and increasing;
// return 0 when you can't make another kind of translator (i.e. for n=1
// if you only implement one subclass of BTranslator). Ignore flags for now.
extern "C" _EXPORT BTranslator *make_nth_translator(int32 n, image_id you,
	uint32 flags, ...);

#endif /* _TRANSLATOR_H */
