// FontManager.h

#ifndef FT_FONT_MANAGER_H
#define FT_FONT_MANAGER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <Entry.h>
#include <Looper.h>

class BMenu;

enum {
	MSG_FONTS_CHANGED	= 'fnch',
	MSG_SET_FONT		= 'stfn',
	MSG_UPDATE	= 'updt',
};

enum {
	FONTS_CHANGED		= 0x01,
};

class FontManager : public BLooper {
 public:
								FontManager(bool scanFontsInline);
	virtual						~FontManager();

								// BLooper
	virtual	void				MessageReceived(BMessage* message);

								// FontManager
	static	void				CreateDefault(bool scanFontsInline = false);
	static	void				DeleteDefault();
	static	FontManager*		Default();

								// lock the object!
	virtual	const entry_ref*	FontFileAt(int32 index) const;
	virtual	const entry_ref*	FontFileFor(const char* family,
											const char* style) const;

	virtual	const char*			FamilyFor(const entry_ref* fontFile) const;
	virtual	const char*			StyleFor(const entry_ref* fontFile) const;

	virtual	const char*			FullFamilyFor(const entry_ref* fontFile) const;
	virtual	const char*			PostScriptNameFor(const entry_ref* fontFile) const;

	virtual	int32				CountFontFiles() const;

 private:
			void				_MakeEmpty();
	static	int32				_update(void* cookie);
			void				_Update(BDirectory* fontFolder);

	struct font_file {
		char*			family_name;
		char*			style_name;
		char*			full_family_name;
		char*			ps_name;
		entry_ref		ref;
	};

			font_file*			_FontFileFor(const entry_ref* ref) const;

			FT_Library			fLibrary;			// the FreeType fLibrary

			BList				fFontFiles;

	static	FontManager*		fDefaultManager;
};

#endif // FT_FONT_MANAGER_H
