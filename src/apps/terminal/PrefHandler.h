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


struct pref_defaults {
	const char *key;
	const char *item;
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

	static	PrefHandler *Default();
	static	void DeleteDefault();
	static	void SetDefault(PrefHandler *handler);

		status_t    OpenText(const char *path);
		void		SaveDefaultAsText();
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
		status_t    _LoadFromDefault(const pref_defaults* defaults = NULL);
		status_t    _LoadFromTextFile(const char * path);

		BMessage    fContainer;

	static	PrefHandler *sPrefHandler;
};

#endif	// PREF_HANDLER_H
