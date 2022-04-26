/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		John Scipione <jscipione@gmail.com>
 */
#include "Thumbnails.h"

#include <fs_attr.h>

#include <Application.h>
#include <BitmapStream.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <TypeConstants.h>
#include <View.h>
#include <Volume.h>

#include "Attributes.h"
#include "Commands.h"
#include "FSUtils.h"
#include "TrackerSettings.h"


#ifdef B_XXL_ICON
#	undef B_XXL_ICON
#endif
#define B_XXL_ICON 128


namespace BPrivate {


status_t
GetThumbnailFromAttr(Model* model, BBitmap* icon, icon_size which)
{
	if (model == NULL || icon == NULL)
		return B_BAD_VALUE;

	status_t result = model->InitCheck();
	if (result != B_OK)
		return result;

	result = icon->InitCheck();
	if (result != B_OK)
		return result;

	BNode* node = model->Node();
	if (node == NULL)
		return B_BAD_VALUE;

	// look for a thumbnail in an attribute
	time_t modtime;
	bigtime_t created;
	if (node->GetModificationTime(&modtime) == B_OK
		&& node->ReadAttr(kAttrThumbnailCreationTime, B_TIME_TYPE, 0,
			&created, sizeof(bigtime_t)) == sizeof(bigtime_t)) {
		if (created > (bigtime_t)modtime) {
			// file has not changed, try to return an existing thumbnail
			attr_info attrInfo;
			if (node->GetAttrInfo(kAttrThumbnail, &attrInfo) == B_OK) {
				uint8 webpData[attrInfo.size];
				if (node->ReadAttr(kAttrThumbnail, attrInfo.type, 0,
						webpData, attrInfo.size) == attrInfo.size) {
					BMemoryIO stream((const void*)webpData, attrInfo.size);
					BBitmap thumb(BTranslationUtils::GetBitmap(&stream));

					// convert thumb to icon size
					if (which == B_XXL_ICON) {
						// import icon data from attribute without resizing
						result = icon->ImportBits(&thumb);
					} else {
						// down-scale thumb to icon size
						// TODO don't make a copy, allow icon to accept views
						BBitmap tmp = BBitmap(icon->Bounds(),
							icon->ColorSpace(), true);
						BView view(tmp.Bounds(), "", B_FOLLOW_NONE,
							B_WILL_DRAW);
						tmp.AddChild(&view);
						if (view.LockLooper()) {
							// fill with transparent
							view.SetLowColor(B_TRANSPARENT_COLOR);
							view.FillRect(view.Bounds(), B_SOLID_LOW);
							// draw bitmap
							view.SetDrawingMode(B_OP_ALPHA);
							view.SetBlendingMode(B_PIXEL_ALPHA,
								B_ALPHA_COMPOSITE);
							view.DrawBitmap(&thumb, thumb.Bounds(),
								tmp.Bounds(), B_FILTER_BITMAP_BILINEAR);
							view.Sync();
							view.UnlockLooper();
						}
						tmp.RemoveChild(&view);

						// copy tmp bitmap into icon
						result = icon->ImportBits(&tmp);
					}
					// we found a thumbnail
					if (result == B_OK)
						return result;
				}
			}
			// else we did not find a thumbnail
		} else {
			// file changed, remove all thumb attrs
			char attrName[B_ATTR_NAME_LENGTH];
			while (node->GetNextAttrName(attrName) == B_OK) {
				if (BString(attrName).StartsWith(kAttrThumbnail))
					node->RemoveAttr(attrName);
			}
		}
	}

	if (ShouldGenerateThumbnail(model->MimeType())) {
		// try to fetch a new thumbnail icon
		result = GetThumbnailIcon(model, icon, which);
		if (result == B_OK) {
			// icon ready
			return B_OK;
		} else if (result == B_BUSY) {
			// working on icon, come back later
			return B_BUSY;
		}
	}

	return ENOENT;
}

//	#pragma mark - image thumbnails


struct ThumbGenParams {
	ThumbGenParams(Model* _model, BFile* _file, icon_size _which,
		color_space _colorSpace, port_id _port);
	virtual	~ThumbGenParams();

	status_t InitCheck() { return fInitStatus; };

	Model* model;
	BFile* file;
	icon_size which;
	color_space colorSpace;
	port_id port;

private:
	status_t fInitStatus;
};


ThumbGenParams::ThumbGenParams(Model* _model, BFile* _file, icon_size _which,
	color_space _colorSpace, port_id _port)
{
	model = new(std::nothrow) Model(*_model);
	file = new(std::nothrow) BFile(*_file);
	which = _which;
	colorSpace = _colorSpace;
	port = _port;

	fInitStatus = (model == NULL || file == NULL ? B_NO_MEMORY : B_OK);
}


ThumbGenParams::~ThumbGenParams()
{
	delete file;
	delete model;
}


status_t get_thumbnail(void* castToParams);
static const int32 kMsgIconData = 'ICON';


BRect
ThumbBounds(BBitmap* icon, float aspectRatio)
{
	BRect thumbBounds;

	if (aspectRatio > 1) {
		// wide
		thumbBounds = BRect(0, 0, icon->Bounds().IntegerWidth() - 1,
			floorf((icon->Bounds().IntegerHeight() - 1) / aspectRatio));
		thumbBounds.OffsetBySelf(0, floorf((icon->Bounds().IntegerHeight()
			- thumbBounds.IntegerHeight()) / 2.0f));
	} else if (aspectRatio < 1) {
		// tall
		thumbBounds = BRect(0, 0, floorf((icon->Bounds().IntegerWidth() - 1)
			* aspectRatio), icon->Bounds().IntegerHeight() - 1);
		thumbBounds.OffsetBySelf(floorf((icon->Bounds().IntegerWidth()
			- thumbBounds.IntegerWidth()) / 2.0f), 0);
	} else {
		// square
		thumbBounds = icon->Bounds();
	}

	return thumbBounds;
}


status_t
GetThumbnailIcon(Model* model, BBitmap* icon, icon_size which)
{
	status_t result = B_ERROR;

	// create a name for the node icon generator thread (32 chars max)
	icon_size w = (icon_size)B_XXL_ICON;
	dev_t d = model->NodeRef()->device;
	ino_t n = model->NodeRef()->node;
	BString genThreadName = BString("_thumbgen_w")
		<< w << "_d" << d << "_n" << n << "_";

	bool volumeReadOnly = true;
	BVolume volume(model->NodeRef()->device);
	if (volume.InitCheck() == B_OK)
		volumeReadOnly = volume.IsReadOnly() || !volume.KnowsAttr();

	port_id port = B_NAME_NOT_FOUND;
	if (volumeReadOnly) {
		// look for a port with some icon data
		port = find_port(genThreadName.String());
			// give the port the same name as the generator thread
		if (port != B_NAME_NOT_FOUND && port_count(port) > 0) {
			// a generator thread has written some data to the port, fetch it
			uint8 iconData[icon->BitsLength()];
			int32 msgCode;
			int32 bytesRead = read_port(port, &msgCode, iconData,
				icon->BitsLength());
			if (bytesRead == icon->BitsLength() && msgCode == kMsgIconData
				&& iconData != NULL) {
				// fill icon data into the passed in icon
				result = icon->ImportBits(iconData, icon->BitsLength(),
					icon->BytesPerRow(), 0, icon->ColorSpace());
			}

			if (result == B_OK) {
				// make a new port next time
				delete_port(port);
				port = B_NAME_NOT_FOUND;
			}
		}
	}

	// we found an icon from a generator thread
	if (result == B_OK)
		return B_OK;

	// look for an existing generator thread before spawning a new one
	if (find_thread(genThreadName.String()) == B_NAME_NOT_FOUND) {
		// no generater thread found, spawn one
		BFile* file = dynamic_cast<BFile*>(model->Node());
		if (file == NULL)
			result = B_NOT_SUPPORTED; // node must be a file
		else {
			// create a new port if one doesn't already exist
			if (volumeReadOnly && port == B_NAME_NOT_FOUND)
				port = create_port(1, genThreadName.String());

			ThumbGenParams* params = new ThumbGenParams(model, file, which,
				icon->ColorSpace(), port);
			if (params->InitCheck() == B_OK) {
				// generator thread will delete params, it makes copies
				resume_thread(spawn_thread(get_thumbnail,
					genThreadName.String(), B_LOW_PRIORITY, params));
				result = B_BUSY; // try again later
			} else
				delete params;
		}
	}

	return result;
}


bool
ShouldGenerateThumbnail(const char* type)
{
	// check generate thumbnail setting,
	// mime type must be an image (for now)
	return TrackerSettings().GenerateImageThumbnails()
		&& type != NULL && BString(type).IStartsWith("image");
}


//	#pragma mark - thumbnail generator thread


status_t
get_thumbnail(void* castToParams)
{
	ThumbGenParams* params = (ThumbGenParams*)castToParams;
	Model* model = params->model;
	BFile* file = params->file;
	icon_size which = params->which;
	color_space colorSpace = params->colorSpace;
	port_id port = params->port;

	// get the mime type from the model
	const char* type = model->MimeType();

	// check if attributes can be written to
	bool volumeReadOnly = true;
	BVolume volume(model->NodeRef()->device);
	if (volume.InitCheck() == B_OK)
		volumeReadOnly = volume.IsReadOnly() || !volume.KnowsAttr();

	// see if we have a thumbnail attribute
	attr_info attrInfo;
	status_t result = file->GetAttrInfo(kAttrThumbnail, &attrInfo);
	if (result != B_OK) {
		// create a new thumbnail

		// check to see if we have a translator that works
		BBitmapStream imageStream;
		BBitmap* image;
		if (BTranslatorRoster::Default()->Translate(file, NULL, NULL,
				&imageStream, B_TRANSLATOR_BITMAP, 0, type) == B_OK
			&& imageStream.DetachBitmap(&image) == B_OK) {
			// we have translated the image file into a BBitmap

			// check if we can write attrs
			if (!volumeReadOnly) {
				// write image width to an attribute
				int32 width = image->Bounds().IntegerWidth();
				file->WriteAttr("Media:Width", B_INT32_TYPE, 0, &width,
					sizeof(int32));

				// write image height to an attribute
				int32 height = image->Bounds().IntegerHeight();
				file->WriteAttr("Media:Height", B_INT32_TYPE, 0, &height,
					sizeof(int32));

				// convert image into a 128x128 WebP image and stash it
				BBitmap thumb = BBitmap(BRect(0, 0, B_XXL_ICON - 1,
					B_XXL_ICON - 1), colorSpace, true);
				BView view(thumb.Bounds(), "", B_FOLLOW_NONE,
					B_WILL_DRAW);
				thumb.AddChild(&view);
				if (view.LockLooper()) {
					// fill with transparent
					view.SetLowColor(B_TRANSPARENT_COLOR);
					view.FillRect(view.Bounds(), B_SOLID_LOW);
					// draw bitmap
					view.SetDrawingMode(B_OP_ALPHA);
					view.SetBlendingMode(B_PIXEL_ALPHA,
						B_ALPHA_COMPOSITE);
					view.DrawBitmap(image, image->Bounds(),
						ThumbBounds(&thumb, image->Bounds().Width()
							/ image->Bounds().Height()),
						B_FILTER_BITMAP_BILINEAR);
					view.Sync();
					view.UnlockLooper();
				}
				thumb.RemoveChild(&view);

				BBitmap* thumbPointer = &thumb;
				BBitmapStream thumbStream(thumbPointer);
				BMallocIO stream;
				if (BTranslatorRoster::Default()->Translate(&thumbStream,
						NULL, NULL, &stream, B_WEBP_FORMAT) == B_OK
					&& thumbStream.DetachBitmap(&thumbPointer) == B_OK) {
					// write WebP image data into an attribute
					file->WriteAttr(kAttrThumbnail, B_RAW_TYPE, 0,
						stream.Buffer(), stream.BufferLength());

					// write thumbnail creation time into an attribute
					bigtime_t created = system_time();
					file->WriteAttr(kAttrThumbnailCreationTime, B_TIME_TYPE,
						0, &created, sizeof(bigtime_t));

					// we wrote thumbnail to an attribute
					result = B_OK;
				}
			} else if (port != B_NAME_NOT_FOUND) {
				// create a thumb at the requested icon size
				BBitmap thumb = BBitmap(BRect(0, 0, which - 1, which - 1),
					colorSpace, true);
				// copy image into a view bitmap, scaled and centered
				BView view(thumb.Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
				thumb.AddChild(&view);
				if (view.LockLooper()) {
					// fill with transparent
					view.SetLowColor(B_TRANSPARENT_COLOR);
					view.FillRect(view.Bounds(), B_SOLID_LOW);
					// draw bitmap
					view.SetDrawingMode(B_OP_ALPHA);
					view.SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
					view.DrawBitmap(image, image->Bounds(),
						ThumbBounds(&thumb, image->Bounds().Width()
							/ image->Bounds().Height()),
						B_FILTER_BITMAP_BILINEAR);
					view.Sync();
					view.UnlockLooper();
				}
				thumb.RemoveChild(&view);

				// send icon back to the calling thread through the port
				result = write_port(port, kMsgIconData, (void*)thumb.Bits(),
					thumb.BitsLength());
			}
		}

		delete image;
	}

	if (result == B_OK) {
		// trigger an icon refresh
		if (!volumeReadOnly)
			model->Mimeset(true); // only works on read-write volumes
		else {
			// send Tracker a message to tell it to update the thumbnail
			BMessage message(kUpdateThumbnail);
			if (message.AddInt32("device", model->NodeRef()->device) == B_OK
				&& message.AddUInt64("node", model->NodeRef()->node) == B_OK) {
				be_app->PostMessage(&message);
			}
		}
	}

	delete params;

	return result;
}


} // namespace BPrivate
