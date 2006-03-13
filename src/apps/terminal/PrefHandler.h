/*
 * Copyright (c) 2003-2006, Haiku, Inc. All Rights Reserved.
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT License.
 */
#ifndef PREF_HANDLER_H
#define PREF_HANDLER_H


#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <Message.h>

class BFont;
class BPath;


#define TP_MAGIC 0xf1f2f3f4
#define TP_VERSION 0x02
#define TP_FONT_NAME_SZ 128

struct termprefs {
	uint32 magic;
	uint32 version;
	float x;
	float y;
	uint32 cols;
	uint32 rows;
	uint32 tab_width;
	uint32 font_size;
	char font[TP_FONT_NAME_SZ]; // "Family/Style"
	uint32 cursor_blink_rate; // blinktime in µs = 1000000
	uint32 refresh_rate; // ??? = 0
	rgb_color bg;
	rgb_color fg;
	rgb_color curbg;
	rgb_color curfg;
	rgb_color selbg;
	rgb_color selfg;
	char encoding; // index in the menu (0 = UTF-8)
	char unknown[3];
};

struct pref_defaults {
	const char *key;
	char *item;
};

#define PREF_TRUE "true"
#define PREF_FALSE "false"

class BMessage;
class BEntry;

class PrefHandler {
	public:
		PrefHandler(const PrefHandler* p);
		PrefHandler();
		~PrefHandler();

		status_t    Open(const char *name);
		status_t    OpenText(const char *path);
		status_t    Save(const char *name);
		void        SaveAsText(const char *path, const char *minmtype = NULL,
						const char *signature = NULL);

		int32       getInt32(const char *key);
		float       getFloat(const char *key);
		const char* getString(const char *key);
		bool        getBool(const char *key);
		rgb_color   getRGB(const char *key);

		void        setInt32(const char *key, int32 data);
		void        setFloat(const char *key, float data);
		void        setString(const char *key, const char *data);
		void        setBool(const char *key, bool data);
		void        setRGB(const char *key, const rgb_color data);

		bool        IsEmpty() const;

		static status_t GetDefaultPath(BPath& path);

	private:
		void		_ConfirmFont(const char *key, const BFont *fallback);
		status_t    _LoadFromFile(const char* path);
		status_t    _LoadFromDefault(const pref_defaults* defaults = NULL);
		status_t    _LoadFromTextFile(const char * path);

		BMessage    fContainer;
};

#endif	// PREF_HANDLER_H
