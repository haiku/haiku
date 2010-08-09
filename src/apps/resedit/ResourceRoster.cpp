/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResourceRoster.h"

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <image.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "InternalEditors.h"
#include "ResourceData.h"
#include "ResFields.h"

// For the MakeFieldForType temp code
#include <TranslatorFormats.h>
#include <TranslationUtils.h>
#include <DataIO.h>
#include <Mime.h>
#include <TypeConstants.h>

class EditorInfo
{
public:
					EditorInfo(const image_id &id, const char *name,
								create_editor *allocator);
					~EditorInfo(void);
	
	status_t		ID(void) const { return fID; }
	const char *	Name(void) const { return fName.String(); }
	Editor *		Instantiate(void);

private:
	image_id		fID;
	BString			fName;
	create_editor *	fAllocator;
};

EditorInfo::EditorInfo(const image_id &id, const char *name,
						create_editor *allocator)
  :	fID(id),
  	fName(name),
  	fAllocator(allocator)
{
}


EditorInfo::~EditorInfo(void)
{
}

	
Editor *
EditorInfo::Instantiate(void)
{
	return fAllocator();
}


ResourceRoster::ResourceRoster(void)
{
}


ResourceRoster::~ResourceRoster(void)
{
}


BField *
ResourceRoster::MakeFieldForType(const int32 &type, const char *data,
								const size_t &length)
{
	// temporary code until editors are done
	switch (type) {
		case B_MIME_STRING_TYPE:
			return new StringPreviewField(data);
		case B_GIF_FORMAT:
		case B_PPM_FORMAT:
		case B_TGA_FORMAT:
		case B_BMP_FORMAT:
		case B_TIFF_FORMAT:
		case B_PNG_FORMAT:
		case B_JPEG_FORMAT: {
			BMemoryIO memio(data, length);
			BBitmap *bitmap = BTranslationUtils::GetBitmap(&memio);
			if (bitmap) {
				BitmapPreviewField *field = new BitmapPreviewField(bitmap);
				return field;
			}
			break;
		}
		default:
			return NULL;
	}
	
	return NULL;
}


void
ResourceRoster::LoadEditors(void)
{
	app_info info;
	be_app->GetAppInfo(&info);
	
	BDirectory dir;
	BEntry entry(&info.ref);
	entry.GetParent(&dir);
	entry.SetTo(&dir, "addons");
	dir.SetTo(&entry);
	
	entry_ref ref;
	dir.Rewind();
	while (dir.GetNextRef(&ref) == B_OK) {
		BPath path(&ref);
		
		image_id addon = load_add_on(path.Path());
		if (addon < 0)
			continue;
		
		char *temp;
		if (get_image_symbol(addon, "description", B_SYMBOL_TYPE_DATA, (void **)(&temp)) != B_OK) {
			unload_add_on(addon);
			continue;
		}
		
		create_editor *createFunc;
		if (get_image_symbol(addon, "instantiate_editor", B_SYMBOL_TYPE_TEXT, (void **)(&createFunc)) != B_OK) {
			delete temp;
			unload_add_on(addon);
			continue;
		}
		
		if (createFunc && temp)
			fList.AddItem(new EditorInfo(addon, temp, createFunc));
		
		delete temp;
	}
}

void
ResourceRoster::SpawnEditor(ResourceData *data, BHandler *handler)
{
	// temporary code until editors are done
	switch (data->GetType()) {
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE: {
			StringEditor *strEd = new StringEditor(BRect(100, 100, 400, 200),
													data, handler);
			strEd->Show();
			break;
		}
		case B_GIF_FORMAT:
		case B_PPM_FORMAT:
		case B_TGA_FORMAT:
		case B_BMP_FORMAT:
		case B_TIFF_FORMAT:
		case B_PNG_FORMAT:
		case B_JPEG_FORMAT: {
			ImageEditor *imgEd = new ImageEditor(BRect(100, 100, 300, 200),
													data, handler);
			imgEd->Show();
			break;
		}
		default:
			break;
	}
}

