/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/*	Parse the ASCII and raw versions of PPM.	*/

#include <Bitmap.h>
#include <BufferedDataIO.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locker.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StringView.h>
#include <TranslationKit.h>
#include <TranslatorAddOn.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "colorspace.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PPMTranslator"

#if DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif


#define PPM_TRANSLATOR_VERSION 0x100

/* These three data items are exported by every translator. */
char translatorName[] = "PPM images";
char translatorInfo[] = "PPM image translator v1.0.0, " __DATE__;
int32 translatorVersion = PPM_TRANSLATOR_VERSION;
// Revision: lowest 4 bits
// Minor: next 4 bits
// Major: highest 24 bits

/*	Be reserves all codes with non-lowercase letters in them.	*/
/*	Luckily, there is already a reserved code for PPM. If you	*/
/*	make up your own for a new type, use lower-case letters.	*/
#define PPM_TYPE 'PPM '


/* These two data arrays are a really good idea to export from Translators, but
 * not required. */
translation_format inputFormats[]
	= {{B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.8, "image/x-be-bitmap",
		   "Be Bitmap Format (PPMTranslator)"},
		{PPM_TYPE, B_TRANSLATOR_BITMAP, 0.3, 0.8, "image/x-portable-pixmap",
			"PPM image"},
		{0, 0, 0, 0, "\0",
			"\0"}}; /*	optional (else Identify is always called)	*/

translation_format outputFormats[]
	= {{B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.8, "image/x-be-bitmap",
		   "Be Bitmap Format (PPMTranslator)"},
		{PPM_TYPE, B_TRANSLATOR_BITMAP, 0.3, 0.8, "image/x-portable-pixmap",
			"PPM image"},
		{0, 0, 0, 0, "\0",
			"\0"}}; /*	optional (else Translate is called anyway)	*/

/*	Translators that don't export outputFormats 	*/
/*	will not be considered by files looking for 	*/
/*	specific output formats.	*/


/*	We keep our settings in a global struct, and wrap a lock around them.	*/
struct ppm_settings {
	color_space out_space;
	BPoint window_pos;
	bool write_ascii;
	bool settings_touched;
};
static BLocker g_settings_lock("PPM settings lock");
static ppm_settings g_settings;

BPoint get_window_origin();
void set_window_origin(BPoint pos);
BPoint
get_window_origin()
{
	BPoint ret;
	g_settings_lock.Lock();
	ret = g_settings.window_pos;
	g_settings_lock.Unlock();
	return ret;
}

void
set_window_origin(BPoint pos)
{
	g_settings_lock.Lock();
	g_settings.window_pos = pos;
	g_settings.settings_touched = true;
	g_settings_lock.Unlock();
}


class PrefsLoader
{
public:
	PrefsLoader(const char* str)
	{
		dprintf(("PPMTranslator: PrefsLoader()\n"));
		/* defaults */
		g_settings.out_space = B_NO_COLOR_SPACE;
		g_settings.window_pos = B_ORIGIN;
		g_settings.write_ascii = false;
		g_settings.settings_touched = false;
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path))
			path.SetTo("/tmp");
		path.Append(str);
		FILE* f = fopen(path.Path(), "r");
		/*	parse text settings file -- this should be a library...	*/
		if (f) {
			char line[1024];
			char name[32];
			char* ptr;
			while (true) {
				line[0] = 0;
				fgets(line, 1024, f);
				if (!line[0])
					break;
				/* remember: line ends with \n, so printf()s don't have to */
				ptr = line;
				while (isspace(*ptr))
					ptr++;
				if (*ptr == '#' || !*ptr) /* comment or blank */
					continue;
				if (sscanf(ptr, "%31[a-zA-Z_0-9] =", name) != 1) {
					syslog(LOG_ERR,
						"unknown PPMTranslator "
						"settings line: %s",
						line);
				} else {
					if (!strcmp(name, "color_space")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%d", (int*) &g_settings.out_space)
							!= 1) {
							syslog(LOG_ERR,
								"illegal color space "
								"in PPMTranslator settings: %s",
								ptr);
						}
					} else if (!strcmp(name, "window_pos")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%f,%f", &g_settings.window_pos.x,
								&g_settings.window_pos.y)
							!= 2) {
							syslog(LOG_ERR,
								"illegal window position "
								"in PPMTranslator settings: %s",
								ptr);
						}
					} else if (!strcmp(name, "write_ascii")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						int ascii = g_settings.write_ascii;
						if (sscanf(ptr, "%d", &ascii) != 1) {
							syslog(LOG_ERR,
								"illegal write_ascii value "
								"in PPMTranslator settings: %s",
								ptr);
						} else {
							g_settings.write_ascii = ascii;
						}
					} else {
						syslog(
							LOG_ERR, "unknown PPMTranslator setting: %s", line);
					}
				}
			}
			fclose(f);
		}
	}

	~PrefsLoader()
	{
		/*	No need writing settings if there aren't any	*/
		if (g_settings.settings_touched) {
			BPath path;
			if (find_directory(B_USER_SETTINGS_DIRECTORY, &path))
				path.SetTo("/tmp");
			path.Append("PPMTranslator_Settings");
			FILE* f = fopen(path.Path(), "w");
			if (f) {
				fprintf(f, "# PPMTranslator settings version %d.%d.%d\n",
					static_cast<int>(translatorVersion >> 8),
					static_cast<int>((translatorVersion >> 4) & 0xf),
					static_cast<int>(translatorVersion & 0xf));
				fprintf(f, "color_space = %d\n", g_settings.out_space);
				fprintf(f, "window_pos = %g,%g\n", g_settings.window_pos.x,
					g_settings.window_pos.y);
				fprintf(
					f, "write_ascii = %d\n", g_settings.write_ascii ? 1 : 0);
				fclose(f);
			}
		}
	}
};

PrefsLoader g_prefs_loader("PPMTranslator_Settings");

/*	Some prototypes for functions we use.	*/
status_t read_ppm_header(BDataIO* io, int* width, int* rowbytes, int* height,
	int* max, bool* ascii, color_space* space, bool* is_ppm, char** comment);
status_t read_bits_header(BDataIO* io, int skipped, int* width, int* rowbytes,
	int* height, int* max, bool* ascii, color_space* space);
status_t write_comment(const char* str, BDataIO* io);
status_t copy_data(BDataIO* in, BDataIO* out, int rowbytes, int out_rowbytes,
	int height, int max, bool in_ascii, bool out_ascii, color_space in_space,
	color_space out_space);


/*	Return B_NO_TRANSLATOR if not handling this data.	*/
/*	Even if inputFormats exists, may be called for data without hints	*/
/*	If outType is not 0, must be able to export in wanted format	*/
status_t
Identify(BPositionIO* inSource, const translation_format* inFormat,
	BMessage* ioExtension, translator_info* outInfo, uint32 outType)
{
	dprintf(("PPMTranslator: Identify()\n"));
	/* Silence compiler warnings. */
	(void)inFormat;
	(void)ioExtension;

	/* Check that requested format is something we can deal with. */
	if (outType == 0)
		outType = B_TRANSLATOR_BITMAP;

	if (outType != B_TRANSLATOR_BITMAP && outType != PPM_TYPE)
		return B_NO_TRANSLATOR;

	/* Check header. */
	int width, rowbytes, height, max;
	bool ascii, is_ppm;
	color_space space;
	status_t err = read_ppm_header(inSource, &width, &rowbytes, &height, &max,
		&ascii, &space, &is_ppm, NULL);
	if (err != B_OK)
		return err;

	/* Stuff info into info struct -- Translation Kit will do "translator" for
	 * us. */
	outInfo->group = B_TRANSLATOR_BITMAP;
	if (is_ppm) {
		outInfo->type = PPM_TYPE;
		outInfo->quality = 0.3; /* no alpha, etc */
		outInfo->capability
			= 0.8; /* we're pretty good at PPM reading, though */
		strlcpy(outInfo->name, B_TRANSLATE("PPM image"), sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/x-portable-pixmap");
	} else {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->quality = 0.4; /* B_TRANSLATOR_BITMAP can do alpha, at least */
		outInfo->capability
			= 0.8; /* and we might not know many variations thereof */
		strlcpy(outInfo->name, B_TRANSLATE("Be Bitmap Format (PPMTranslator)"),
			sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/x-be-bitmap"); /* this is the MIME type of
													   B_TRANSLATOR_BITMAP */
	}
	return B_OK;
}


/*	Return B_NO_TRANSLATOR if not handling the output format	*/
/*	If outputFormats exists, will only be called for those formats	*/
status_t
Translate(BPositionIO* inSource, const translator_info* /*inInfo*/,
	BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
{
	dprintf(("PPMTranslator: Translate()\n"));
	inSource->Seek(0, SEEK_SET); /* paranoia */
	//	inInfo = inInfo;	/* silence compiler warning */
	/* Check what we're being asked to produce. */
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	dprintf(("PPMTranslator: outType is '%c%c%c%c'\n", char(outType >> 24),
		char(outType >> 16), char(outType >> 8), char(outType)));
	if (outType != B_TRANSLATOR_BITMAP && outType != PPM_TYPE)
		return B_NO_TRANSLATOR;

	/* Figure out what we've been given (again). */
	int width, rowbytes, height, max;
	bool ascii, is_ppm;
	color_space space;
	/* Read_ppm_header() will always return with stream at beginning of data */
	/* for both B_TRANSLATOR_BITMAP and PPM_TYPE, and indicate what it is. */
	char* comment = NULL;
	status_t err = read_ppm_header(inSource, &width, &rowbytes, &height, &max,
		&ascii, &space, &is_ppm, &comment);
	if (comment != NULL) {
		if (ioExtension != NULL)
			ioExtension->AddString(B_TRANSLATOR_EXT_COMMENT, comment);
		free(comment);
	}
	if (err < B_OK) {
		dprintf(("read_ppm_header() error %s [%" B_PRIx32 "]\n", strerror(err),
			err));
		return err;
	}
	/* Check if we're configured to write ASCII format file. */
	bool out_ascii = false;
	if (outType == PPM_TYPE) {
		out_ascii = g_settings.write_ascii;
		if (ioExtension != NULL)
			ioExtension->FindBool("ppm /ascii", &out_ascii);
	}
	err = B_OK;
	/* Figure out which color space to convert to */
	color_space out_space;
	int out_rowbytes;
	g_settings_lock.Lock();
	if (outType == PPM_TYPE) {
		out_space = B_RGB24_BIG;
		out_rowbytes = 3 * width;
	} else { /*	When outputting to B_TRANSLATOR_BITMAP, follow user's wishes.
			  */
		if (!ioExtension
			|| ioExtension->FindInt32( B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE,
				(int32*) &out_space)
			|| (out_space == B_NO_COLOR_SPACE)) {
			if (g_settings.out_space == B_NO_COLOR_SPACE) {
				switch (space) { /*	The 24-bit versions are pretty silly, use 32
									instead.	*/
					case B_RGB24:
					case B_RGB24_BIG:
						out_space = B_RGB32;
						break;
					default:
						/* use whatever is there */
						out_space = space;
						break;
				}
			} else {
				out_space = g_settings.out_space;
			}
		}
		out_rowbytes = calc_rowbytes(out_space, width);
	}
	g_settings_lock.Unlock();
	/* Write file header */
	if (outType == PPM_TYPE) {
		dprintf(("PPMTranslator: write PPM\n"));
		/* begin PPM header */
		const char* magic;
		if (out_ascii)
			magic = "P3\n";
		else
			magic = "P6\n";
		err = outDestination->Write(magic, strlen(magic));
		if (err == (long) strlen(magic))
			err = 0;
		// comment = NULL;
		const char* fsComment;
		if ((ioExtension != NULL)
			&& !ioExtension->FindString(B_TRANSLATOR_EXT_COMMENT, &fsComment))
			err = write_comment(fsComment, outDestination);
		if (err == B_OK) {
			char data[40];
			sprintf(data, "%d %d %d\n", width, height, max);
			err = outDestination->Write(data, strlen(data));
			if (err == (long) strlen(data))
				err = 0;
		}
		/* header done */
	} else {
		dprintf(("PPMTranslator: write B_TRANSLATOR_BITMAP\n"));
		/* B_TRANSLATOR_BITMAP header */
		TranslatorBitmap hdr;
		hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
		hdr.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0);
		hdr.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0);
		hdr.bounds.right = B_HOST_TO_BENDIAN_FLOAT(width - 1);
		hdr.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(height - 1);
		hdr.rowBytes = B_HOST_TO_BENDIAN_INT32(out_rowbytes);
		hdr.colors = (color_space) B_HOST_TO_BENDIAN_INT32(out_space);
		hdr.dataSize = B_HOST_TO_BENDIAN_INT32(out_rowbytes * height);
		dprintf(("rowBytes is %d, width %d, out_space %x, space %x\n",
			out_rowbytes, width, out_space, space));
		err = outDestination->Write(&hdr, sizeof(hdr));
		dprintf(("PPMTranslator: Write() returns %" B_PRIx32 "\n", err));
#if DEBUG
		{
			BBitmap* map
				= new BBitmap(BRect(0, 0, width - 1, height - 1), out_space);
			printf("map rb = %" B_PRId32 "\n", map->BytesPerRow());
			delete map;
		}
#endif
		if (err == sizeof(hdr))
			err = 0;
		/* header done */
	}
	if (err != B_OK)
		return err > 0 ? B_IO_ERROR : err;
	/* Write data. Luckily, PPM and B_TRANSLATOR_BITMAP both scan from left to
	 * right, top to bottom. */
	return copy_data(inSource, outDestination, rowbytes, out_rowbytes, height,
		max, ascii, out_ascii, space, out_space);
}


class PPMView : public BView
{
public:
	PPMView(const char* name, uint32 flags) : BView(name, flags)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

		fTitle = new BStringView("title", B_TRANSLATE("PPM image translator"));
		fTitle->SetFont(be_bold_font);

		char detail[100];
		int ver = static_cast<int>(translatorVersion);
		sprintf(detail, B_TRANSLATE("Version %d.%d.%d, %s"), ver >> 8,
			((ver >> 4) & 0xf), (ver & 0xf), __DATE__);
		fDetail = new BStringView("detail", detail);

		fBasedOn = new BStringView(
			"basedOn", B_TRANSLATE("Based on PPMTranslator sample code"));

		fCopyright = new BStringView("copyright",
			B_TRANSLATE("Sample code copyright 1999, Be Incorporated"));

		fMenu = new BPopUpMenu("Color Space");
		fMenu->AddItem(
			new BMenuItem(B_TRANSLATE("None"), CSMessage(B_NO_COLOR_SPACE)));
		fMenu->AddItem(new BMenuItem(
			B_TRANSLATE("RGB 8:8:8 32 bits"), CSMessage(B_RGB32)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGBA 8:8:8:8 32 "
												 "bits"),
			CSMessage(B_RGBA32)));
		fMenu->AddItem(new BMenuItem(
			B_TRANSLATE("RGB 5:5:5 16 bits"), CSMessage(B_RGB15)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGBA 5:5:5:1 16 "
												 "bits"),
			CSMessage(B_RGBA15)));
		fMenu->AddItem(new BMenuItem(
			B_TRANSLATE("RGB 5:6:5 16 bits"), CSMessage(B_RGB16)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("System palette 8 "
												 "bits"),
			CSMessage(B_CMAP8)));
		fMenu->AddSeparatorItem();
		fMenu->AddItem(
			new BMenuItem(B_TRANSLATE("Grayscale 8 bits"), CSMessage(B_GRAY8)));
		fMenu->AddItem(
			new BMenuItem(B_TRANSLATE("Bitmap 1 bit"), CSMessage(B_GRAY1)));
		fMenu->AddItem(new BMenuItem(
			B_TRANSLATE("CMY 8:8:8 32 bits"), CSMessage(B_CMY32)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("CMYA 8:8:8:8 32 "
												 "bits"),
			CSMessage(B_CMYA32)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("CMYK 8:8:8:8 32 "
												 "bits"),
			CSMessage(B_CMYK32)));
		fMenu->AddSeparatorItem();
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGB 8:8:8 32 bits "
												 "big-endian"),
			CSMessage(B_RGB32_BIG)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGBA 8:8:8:8 32 "
												 "bits big-endian"),
			CSMessage(B_RGBA32_BIG)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGB 5:5:5 16 bits "
												 "big-endian"),
			CSMessage(B_RGB15_BIG)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGBA 5:5:5:1 16 "
												 "bits big-endian"),
			CSMessage(B_RGBA15_BIG)));
		fMenu->AddItem(new BMenuItem(B_TRANSLATE("RGB 5:6:5 16 bits "
												 "big-endian"),
			CSMessage(B_RGB16)));
		fField = new BMenuField(B_TRANSLATE("Input color space:"), fMenu);
		fField->SetViewColor(ViewColor());
		SelectColorSpace(g_settings.out_space);
		BMessage* msg = new BMessage(CHANGE_ASCII);
		fAscii = new BCheckBox(B_TRANSLATE("Write ASCII"), msg);
		if (g_settings.write_ascii)
			fAscii->SetValue(1);
		fAscii->SetViewColor(ViewColor());

		// Build the layout
		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fTitle)
			.Add(fDetail)
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
			.Add(fField)
			.AddGlue()
			.End()
			.Add(fAscii)
			.AddGlue()
			.Add(fBasedOn)
			.Add(fCopyright);

		BFont font;
		GetFont(&font);
		SetExplicitPreferredSize(
			BSize((font.Size() * 350) / 12, (font.Size() * 200) / 12));
	}
	~PPMView() { /* nothing here */ }

	enum { SET_COLOR_SPACE = 'ppm=', CHANGE_ASCII };

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == SET_COLOR_SPACE) {
			SetSettings(message);
		} else if (message->what == CHANGE_ASCII) {
			BMessage msg;
			msg.AddBool("ppm /ascii", fAscii->Value());
			SetSettings(&msg);
		} else {
			BView::MessageReceived(message);
		}
	}
	virtual void AllAttached()
	{
		BView::AllAttached();
		BMessenger msgr(this);
		/*	Tell all menu items we're the man.	*/
		for (int ix = 0; ix < fMenu->CountItems(); ix++) {
			BMenuItem* i = fMenu->ItemAt(ix);
			if (i)
				i->SetTarget(msgr);
		}
		fAscii->SetTarget(msgr);
	}

	void SetSettings(BMessage* message)
	{
		g_settings_lock.Lock();
		color_space space;
		if (!message->FindInt32("space", (int32*) &space)) {
			g_settings.out_space = space;
			SelectColorSpace(space);
			g_settings.settings_touched = true;
		}
		bool ascii;
		if (!message->FindBool("ppm /ascii", &ascii)) {
			g_settings.write_ascii = ascii;
			g_settings.settings_touched = true;
		}
		g_settings_lock.Unlock();
	}

private:
	BStringView* fTitle;
	BStringView* fDetail;
	BStringView* fBasedOn;
	BStringView* fCopyright;
	BPopUpMenu* fMenu;
	BMenuField* fField;
	BCheckBox* fAscii;

	BMessage* CSMessage(color_space space)
	{
		BMessage* ret = new BMessage(SET_COLOR_SPACE);
		ret->AddInt32("space", space);
		return ret;
	}

	void SelectColorSpace(color_space space)
	{
		for (int ix = 0; ix < fMenu->CountItems(); ix++) {
			int32 s;
			BMenuItem* i = fMenu->ItemAt(ix);
			if (i) {
				BMessage* m = i->Message();
				if (m && !m->FindInt32("space", &s) && (s == space)) {
					fMenu->Superitem()->SetLabel(i->Label());
					break;
				}
			}
		}
	}
};


/*	The view will get resized to what the parent thinks is 	*/
/*	reasonable. However, it will still receive MouseDowns etc.	*/
/*	Your view should change settings in the translator immediately, 	*/
/*	taking care not to change parameters for a translation that is 	*/
/*	currently running. Typically, you'll have a global struct for 	*/
/*	settings that is atomically copied into the translator function 	*/
/*	as a local when translation starts.	*/
/*	Store your settings wherever you feel like it.	*/
status_t
MakeConfig(BMessage* ioExtension, BView** outView, BRect* outExtent)
{
	PPMView* v
		= new PPMView(B_TRANSLATE("PPMTranslator Settings"), B_WILL_DRAW);
	*outView = v;
	v->ResizeTo(v->ExplicitPreferredSize());
	;
	*outExtent = v->Bounds();
	if (ioExtension)
		v->SetSettings(ioExtension);
	return B_OK;
}


/*	Copy your current settings to a BMessage that may be passed 	*/
/*	to BTranslators::Translate at some later time when the user wants to 	*/
/*	use whatever settings you're using right now.	*/
status_t
GetConfigMessage(BMessage* ioExtension)
{
	status_t err = B_OK;
	const char* name = B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE;
	g_settings_lock.Lock();
	(void) ioExtension->RemoveName(name);
	err = ioExtension->AddInt32(name, g_settings.out_space);
	g_settings_lock.Unlock();
	return err;
}


status_t
read_ppm_header(BDataIO* inSource, int* width, int* rowbytes, int* height,
	int* max, bool* ascii, color_space* space, bool* is_ppm, char** comment)
{
	/* check for PPM magic number */
	char ch[2];
	bool monochrome = false;
	bool greyscale = false;
	if (inSource->Read(ch, 2) != 2)
		return B_NO_TRANSLATOR;
	/* look for magic number */
	if (ch[0] != 'P') {
		/* B_TRANSLATOR_BITMAP magic? */
		if (ch[0] == 'b' && ch[1] == 'i') {
			*is_ppm = false;
			return read_bits_header(
				inSource, 2, width, rowbytes, height, max, ascii, space);
		}
		return B_NO_TRANSLATOR;
	}
	*is_ppm = true;
	if (ch[1] == '6' || ch[1] == '5' || ch[1] == '4') {
		*ascii = false;
	} else if (ch[1] == '3' || ch[1] == '2' || ch[1] == '1') {
		*ascii = true;
	} else {
		return B_NO_TRANSLATOR;
	}

	if (ch[1] == '4' || ch[1] == '1')
		monochrome = true;
	else if (ch[1] == '5' || ch[1] == '2')
		greyscale = true;

	// status_t err = B_NO_TRANSLATOR;
	enum scan_state {
		scan_width,
		scan_height,
		scan_max,
		scan_white
	} state
		= scan_width;
	int* scan = NULL;
	bool in_comment = false;
	if (monochrome)
		*space = B_GRAY1;
	else if (greyscale)
		*space = B_GRAY8;
	else
		*space = B_RGB24_BIG;
	/* The description of PPM is slightly ambiguous as far as comments */
	/* go. We choose to allow comments anywhere, in the spirit of laxness. */
	/* See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/PPM.txt */
	int comlen = 0;
	int comptr = 0;
	while (inSource->Read(ch, 1) == 1) {
		if (in_comment && (ch[0] != 10) && (ch[0] != 13)) {
			if (comment) { /* collect comment(s) into comment string */
				if (comptr >= comlen - 1) {
					char* n = (char*) realloc(*comment, comlen + 100);
					if (!n) {
						free(*comment);
						*comment = NULL;
					}
					*comment = n;
					comlen += 100;
				}
				(*comment)[comptr++] = ch[0];
				(*comment)[comptr] = 0;
			}
			continue;
		}
		in_comment = false;
		if (ch[0] == '#') {
			in_comment = true;
			continue;
		}
		/* once we're done with whitespace after max, we're done with header */
		if (isdigit(ch[0])) {
			if (!scan) { /* first digit for this value */
				switch (state) {
					case scan_width:
						scan = width;
						break;
					case scan_height:
						if (monochrome)
							*rowbytes = (*width + 7) / 8;
						else if (greyscale)
							*rowbytes = *width;
						else
							*rowbytes = *width * 3;
						scan = height;
						break;
					case scan_max:
						scan = max;
						break;
					default:
						return B_OK; /* done with header, all OK */
				}
				*scan = 0;
			}
			*scan = (*scan) * 10 + (ch[0] - '0');
		} else if (isspace(ch[0])) {
			if (scan) { /* are we done with one value? */
				scan = NULL;
				state = (enum scan_state)(state + 1);

				/* in monochrome ppm, there is no max in the file, so we
				 * skip that step. */
				if ((state == scan_max) && monochrome) {
					state = (enum scan_state)(state + 1);
					*max = 1;
				}
			}

			if (state == scan_white) { /* we only ever read one whitespace,
										  since we skip space */
				return B_OK; /* when reading ASCII, and there is a single
								whitespace after max in raw mode */
			}
		} else {
			if (state != scan_white)
				return B_NO_TRANSLATOR;
			return B_OK; /* header done */
		}
	}
	return B_NO_TRANSLATOR;
}


status_t
read_bits_header(BDataIO* io, int skipped, int* width, int* rowbytes,
	int* height, int* max, bool* ascii, color_space* space)
{
	/* read the rest of a possible B_TRANSLATOR_BITMAP header */
	if (skipped < 0 || skipped > 4)
		return B_NO_TRANSLATOR;
	int rd = sizeof(TranslatorBitmap) - skipped;
	TranslatorBitmap hdr;
	/* pre-initialize magic because we might have skipped part of it already */
	hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	char* ptr = (char*) &hdr;
	if (io->Read(ptr + skipped, rd) != rd)
		return B_NO_TRANSLATOR;
	/* swap header values */
	hdr.magic = B_BENDIAN_TO_HOST_INT32(hdr.magic);
	hdr.bounds.left = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.left);
	hdr.bounds.right = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.right);
	hdr.bounds.top = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.top);
	hdr.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(hdr.bounds.bottom);
	hdr.rowBytes = B_BENDIAN_TO_HOST_INT32(hdr.rowBytes);
	hdr.colors = (color_space) B_BENDIAN_TO_HOST_INT32(hdr.colors);
	hdr.dataSize = B_BENDIAN_TO_HOST_INT32(hdr.dataSize);
	/* sanity checking */
	if (hdr.magic != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;
	if (hdr.colors & 0xffff0000) { /* according to <GraphicsDefs.h> this is a
									  reasonable check. */
		return B_NO_TRANSLATOR;
	}
	if (hdr.rowBytes * (hdr.bounds.Height() + 1) > hdr.dataSize)
		return B_NO_TRANSLATOR;
	/* return information about the data in the stream */
	*width = (int) hdr.bounds.Width() + 1;
	*rowbytes = hdr.rowBytes;
	*height = (int) hdr.bounds.Height() + 1;
	*max = 255;
	*ascii = false;
	*space = hdr.colors;
	return B_OK;
}


/*	String may contain newlines, after which we need to insert extra hash signs.
 */
status_t
write_comment(const char* str, BDataIO* io)
{
	const char* end = str + strlen(str);
	const char* ptr = str;
	status_t err = B_OK;
	/* write each line as it's found */
	while ((ptr < end) && !err) {
		if ((*ptr == 10) || (*ptr == 13)) {
			err = io->Write("# ", 2);
			if (err == 2) {
				err = io->Write(str, ptr - str);
				if (err == ptr - str) {
					if (io->Write("\n", 1) == 1)
						err = 0;
				}
			}
			str = ptr + 1;
		}
		ptr++;
	}
	/* write the last data, if any, as a line */
	if (ptr > str) {
		err = io->Write("# ", 2);
		if (err == 2) {
			err = io->Write(str, ptr - str);
			if (err == ptr - str) {
				if (io->Write("\n", 1) == 1)
					err = 0;
			}
		}
	}
	if (err > 0)
		err = B_IO_ERROR;
	return err;
}


static status_t
read_ascii_line(BDataIO* in, int max, unsigned char* data, int rowbytes)
{
	char ch;
	status_t err;
	// int nread = 0;
	bool comment = false;
	int val = 0;
	bool dig = false;
	while ((err = in->Read(&ch, 1)) == 1) {
		if (comment) {
			if ((ch == '\n') || (ch == '\r'))
				comment = false;
		}
		if (isdigit(ch)) {
			dig = true;
			val = val * 10 + (ch - '0');
		} else {
			if (dig) {
				*(data++) = val * 255 / max;
				val = 0;
				rowbytes--;
				dig = false;
			}
			if (ch == '#') {
				comment = true;
				continue;
			}
		}
		if (rowbytes < 1)
			break;
	}
	if (dig) {
		*(data++) = val * 255 / max;
		val = 0;
		rowbytes--;
		dig = false;
	}
	if (rowbytes < 1)
		return B_OK;
	return B_IO_ERROR;
}


static status_t
write_ascii_line(BDataIO* out, unsigned char* data, int rowbytes)
{
	char buffer[20];
	int linelen = 0;
	while (rowbytes > 2) {
		sprintf(buffer, "%d %d %d ", data[0], data[1], data[2]);
		rowbytes -= 3;
		int l = strlen(buffer);
		if (l + linelen > 70) {
			out->Write("\n", 1);
			linelen = 0;
		}
		if (out->Write(buffer, l) != l)
			return B_IO_ERROR;
		linelen += l;
		data += 3;
	}
	out->Write("\n", 1); /* terminate each scanline */
	return B_OK;
}


static unsigned char*
make_scale_data(int max)
{
	unsigned char* ptr = (unsigned char*) malloc(max);
	for (int ix = 0; ix < max; ix++)
		ptr[ix] = ix * 255 / max;
	return ptr;
}


static void
scale_data(unsigned char* scale, unsigned char* data, int bytes)
{
	for (int ix = 0; ix < bytes; ix++)
		data[ix] = scale[data[ix]];
}


status_t
copy_data(BDataIO* in, BDataIO* out, int rowbytes, int out_rowbytes, int height,
	int max, bool in_ascii, bool out_ascii, color_space in_space,
	color_space out_space)
{
	dprintf(("copy_data(%d, %d, %d, %d, %x, %x)\n", rowbytes, out_rowbytes,
		height, max, in_space, out_space));

	// We will be reading 1 char at a time, so use a buffer
	BBufferedDataIO inBuffer(*in, 65536, false);

	/*	We read/write one scanline at a time.	*/
	unsigned char* data = (unsigned char*) malloc(rowbytes);
	unsigned char* out_data = (unsigned char*) malloc(out_rowbytes);
	if (data == NULL || out_data == NULL) {
		free(data);
		free(out_data);
		return B_NO_MEMORY;
	}
	unsigned char* scale = NULL;
	if (max != 255 && in_space != B_GRAY1)
		scale = make_scale_data(max);
	status_t err = B_OK;
	/*	There is no data format conversion, so we can just copy data.	*/
	while ((height-- > 0) && !err) {
		if (in_ascii) {
			err = read_ascii_line(&inBuffer, max, data, rowbytes);
		} else {
			err = inBuffer.Read(data, rowbytes);
			if (err == rowbytes)
				err = B_OK;
			if (scale) /* for reading PPM that is smaller than 8 bit */
				scale_data(scale, data, rowbytes);
		}
		if (err == B_OK) {
			unsigned char* wbuf = data;
			if (in_space != out_space) {
				err = convert_space(
					in_space, out_space, data, rowbytes, out_data);
				wbuf = out_data;
			}
			if (!err && out_ascii) {
				err = write_ascii_line(out, wbuf, out_rowbytes);
			} else if (!err) {
				err = out->Write(wbuf, out_rowbytes);
				if (err == out_rowbytes)
					err = B_OK;
			}
		}
	}
	free(data);
	free(out_data);
	free(scale);
	return err > 0 ? B_IO_ERROR : err;
}
