/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef RTF_TRANSLATOR_H
#define RTF_TRANSLATOR_H


#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <fs_attr.h>


#define RTF_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 6, 0)
#define RTF_TEXT_FORMAT		'RTF '
#define RTF_IN_QUALITY		0.7
#define RTF_IN_CAPABILITY	0.9

#define TEXT_OUT_QUALITY	0.3
#define TEXT_OUT_CAPABILITY	0.6
#define STXT_OUT_QUALITY	0.5
#define STXT_OUT_CAPABILITY	0.5


class RTFTranslator : public BTranslator {
	public:
		RTFTranslator();

		virtual const char *TranslatorName() const;
		virtual const char *TranslatorInfo() const;
		virtual int32 TranslatorVersion() const;

		virtual const translation_format *InputFormats(int32 *_outCount) const;
		virtual const translation_format *OutputFormats(int32 *_outCount) const;

		virtual status_t Identify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t Translate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination);

		virtual status_t MakeConfigurationView(BMessage *ioExtension,
			BView **outView, BRect *outExtent);

	protected:
		virtual ~RTFTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user

	private:
		char	*fInfo;
};

#endif	/* RTF_TRANSLATOR_H */
