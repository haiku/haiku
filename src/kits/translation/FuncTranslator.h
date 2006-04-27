/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _FUNC_TRANSLATOR_H
#define _FUNC_TRANSLATOR_H


#include <Translator.h>


// Structure used to hold the collection of function pointers and 
// public variables exported by function based translator add-ons.
struct translator_data {
	const char*	name;
	const char*	info;
	int32		version;
	const translation_format* input_formats;
	const translation_format* output_formats;

	status_t	(*identify_hook)(BPositionIO* source, const translation_format* format,
					BMessage* ioExtension, translator_info* outInfo, uint32 outType);

	status_t	(*translate_hook)(BPositionIO* source, const translator_info* info,
					BMessage* ioExtension, uint32 outType, BPositionIO* destination);

	status_t	(*make_config_hook)(BMessage* ioExtension, BView** _view, BRect* _extent);
	status_t	(*get_config_message_hook)(BMessage* ioExtension);
};

namespace BPrivate {

class BFuncTranslator : public BTranslator {
	public:
		BFuncTranslator(const translator_data& data);

		virtual const char *TranslatorName() const;
		virtual const char *TranslatorInfo() const;
		virtual int32 TranslatorVersion() const;

		virtual const translation_format *InputFormats(int32 *out_count) const;
		virtual const translation_format *OutputFormats(int32 *out_count) const;

		virtual status_t Identify(BPositionIO *inSource, 
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);
		virtual status_t Translate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
			BPositionIO * outDestination);
		virtual status_t MakeConfigurationView(BMessage *ioExtension,
			BView **outView, BRect *outExtent);
		virtual status_t GetConfigurationMessage(BMessage *ioExtension);

	protected:
		virtual ~BFuncTranslator();
			// This object is deleted by calling Release(),
			// it can not be deleted directly. See BTranslator in the Be Book

	private:
		translator_data fData;
};

}	// namespace BPrivate

#endif // _FUNC_TRANSLATOR_H
