/*
 * Copyright 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "HPGSTranslator.h"
#include "ConfigView.h"
#include "ReadHelper.h"

#include "hpgsimage.h"

#include <Catalog.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HPGSTranslator"


typedef struct my_hpgs_png_image_st {
	hpgs_png_image image;
	BPositionIO *target;
} my_hpgs_png_image;


int
my_pim_write(hpgs_image *_this, const char *filename)
{
	BPositionIO* target = ((my_hpgs_png_image *)_this)->target;
	unsigned char *row;
	int stride, depth;
	_this->vtable->get_data(_this, &row, &stride, &depth);
	for (int i = 0; i< _this->height; ++i) {
		ssize_t bytesWritten = target->Write(row, stride);
		if (bytesWritten < B_OK)
			return bytesWritten;
		row += stride;
	}

	return 0;
}


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		HPGS_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		HPGS_IN_QUALITY,
		HPGS_IN_CAPABILITY,
		"vector/x-hpgl2",
		"HP-GL/2"
	},
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (HPGSTranslator)"
	},
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


//	#pragma mark -


HPGSTranslator::HPGSTranslator()
	: BaseTranslator(B_TRANSLATE("HPGS images"), 
		B_TRANSLATE("HPGS image translator"),
		HPGS_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"HPGSTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, HPGS_IMAGE_FORMAT)
{
	hpgs_init("/usr/local");
}


HPGSTranslator::~HPGSTranslator()
{
	hpgs_cleanup();
}


status_t
HPGSTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *settings,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	hpgs_istream *istream = hpgs_new_wrapper_istream(stream);

	status_t err = B_OK;
	int verbosity = 1;
	hpgs_bool multipage = HPGS_FALSE;
	hpgs_bool ignore_ps = HPGS_FALSE;
	hpgs_bool do_linewidth = HPGS_TRUE;
	hpgs_device *size_dev = (hpgs_device *)hpgs_new_plotsize_device(ignore_ps, 
		do_linewidth);
	hpgs_reader *reader = hpgs_new_reader(istream, size_dev, multipage, 
		verbosity);
	if (hpgs_read(reader, HPGS_FALSE) == B_OK) {
		info->type = HPGS_IMAGE_FORMAT;
		info->group = B_TRANSLATOR_BITMAP;
		info->quality = HPGS_IN_QUALITY;
		info->capability = HPGS_IN_CAPABILITY;
		snprintf(info->name, sizeof(info->name), B_TRANSLATE("HPGS image"));
		strcpy(info->MIME, "vector/x-hpgl2");
	} else
		err = B_NO_TRANSLATOR;

	free(reader);
	free(size_dev);
	hpgs_free_wrapper_istream(istream);

	return err;
}


status_t
HPGSTranslator::DerivedTranslate(BPositionIO* source,
	const translator_info* info, BMessage* settings,
	uint32 outType, BPositionIO* target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP || baseType != 0)
		return B_NO_TRANSLATOR;

	status_t err = B_OK;
	hpgs_istream *istream = hpgs_new_wrapper_istream(source);

	TranslatorBitmap header;
	ssize_t bytesWritten;
	uint32 dataSize;

	double paper_angle = 180.0;
	double paper_border = 0.0;
	double paper_width = 0.0;
	double paper_height = 0.0;
	int verbosity = 0;
	int depth = 32;
	int palette = 0;
	hpgs_bool multipage = HPGS_FALSE;
	hpgs_bool ignore_ps = HPGS_FALSE;
	hpgs_bool do_linewidth = HPGS_TRUE;
	hpgs_bool do_rop3 = HPGS_TRUE;
	hpgs_bool antialias = HPGS_FALSE;
	int image_interpolation = 0;
	double thin_alpha = 0.25;
	hpgs_device *size_dev = 0;
	hpgs_bbox bbox = { 0.0, 0.0, 0.0, 0.0 };
	hpgs_image *image;
	hpgs_paint_device *pdv;
	hpgs_device *plot_dev;

	size_dev = (hpgs_device *)hpgs_new_plotsize_device(ignore_ps, do_linewidth);
	hpgs_reader *reader = hpgs_new_reader(istream, size_dev, multipage, verbosity);
	if (hpgs_read(reader, HPGS_FALSE)) {
		fprintf(stderr, B_TRANSLATE("no hpgs\n"));
		err = B_NO_TRANSLATOR;
		goto err1;
	}

	if (hpgs_getplotsize(size_dev,1,&bbox)<0) {
		fprintf(stderr, B_TRANSLATE("no hpgs\n"));
		err = B_NO_TRANSLATOR;
		goto err1;
	}

	// set the appropriate page placement.
	if (paper_width > 0.0 && paper_height > 0.0) {
		hpgs_reader_set_fixed_page(reader,&bbox,
			paper_width, paper_height, paper_border, paper_angle);
	} else {
		paper_width = 200.0 * 72.0;
		paper_height = 200.0 * 72.0;
		hpgs_reader_set_dynamic_page(reader,&bbox,
			paper_width, paper_height, paper_border, paper_angle);
	}

	image = (hpgs_image*)hpgs_new_png_image((int)bbox.urx, (int)bbox.ury, depth, palette, do_rop3);
	image->vtable->write = &my_pim_write;
	image = (hpgs_image *)realloc(image, sizeof(my_hpgs_png_image));
	((my_hpgs_png_image *)image)->target = target;

	pdv = hpgs_new_paint_device(image, NULL, &bbox, antialias);
	hpgs_paint_device_set_image_interpolation(pdv, image_interpolation);
	hpgs_paint_device_set_thin_alpha(pdv, thin_alpha);

	plot_dev = (hpgs_device *)pdv;
	if (hpgs_reader_imbue(reader, plot_dev)) {
		fprintf(stderr, hpgs_i18n(B_TRANSLATE("Error: Cannot imbue plot "
			"device to reader: %s\n")), hpgs_get_error());
		err = B_NO_TRANSLATOR;
		goto err2;
	}

	dataSize = (int)bbox.urx * 4 * (int)bbox.ury;

	header.magic = B_TRANSLATOR_BITMAP;
	header.bounds.Set(0, 0, (int)bbox.urx - 1, bbox.ury - 1);
	header.rowBytes = (int)bbox.urx * 4;
	header.colors = B_RGBA32;
	header.dataSize = dataSize;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	bytesWritten = target->Write(&header, sizeof(TranslatorBitmap));
	if (bytesWritten < B_OK) {
		fprintf(stderr, B_TRANSLATE("Write error %s\n"), 
			strerror(bytesWritten));
		err = bytesWritten;
		goto err2;
	}

	if ((size_t)bytesWritten != sizeof(TranslatorBitmap)) {
		fprintf(stderr, "TranslatorBitmap\n");
		err = B_IO_ERROR;
	}

	if (err == B_OK && hpgs_read(reader, HPGS_FALSE)) {
		fprintf(stderr, hpgs_i18n(B_TRANSLATE("Error: Cannot process plot "
			"data %s\n")), hpgs_get_error());
		err = B_NO_TRANSLATOR;
	}

err2:
	free(pdv);
	free(image);

err1:
	free(reader);
	free(size_dev);
	free(istream->stream);
	free(istream);
	return err;
}


BView *
HPGSTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView(BRect(0, 0, 225, 175));
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new HPGSTranslator();
}

