#ifndef FONTSERVER_H_
#define FONTSERVER_H_

#include <OS.h>
#include <List.h>
#include <SupportDefs.h>
#include <Font.h>
#include <freetype.h>
#include <ftcache.h>

class FontFamily;
class FontStyle;
class ServerFont;

class FontServer
{
public:
	FontServer(void);
	~FontServer(void);
	void Lock(void);
	void Unlock(void);
	bool IsInitialized(void) { return init; }
	int32 CountFamilies(void);
	int32 CountStyles(const char *family);
	void RemoveFamily(const char *family);
	status_t ScanDirectory(const char *path);
	void SaveList(void);
	FontStyle *GetStyle(font_family family, font_style face);
	ServerFont *GetSystemPlain(void);
	ServerFont *GetSystemBold(void);
	ServerFont *GetSystemFixed(void);
	bool SetSystemPlain(const char *family, const char *style, float size);
	bool SetSystemBold(const char *family, const char *style, float size);
	bool SetSystemFixed(const char *family, const char *style, float size);
	bool FontsNeedUpdated(void) { return need_update; }
	void FontsUpdated(void) { need_update=false; }
//	FontInstance *GetInstance(font_family family, font_style face, int16 size, int16 rotation, int16 shear);

	FontFamily *_FindFamily(const char *name);
	FT_CharMap _GetSupportedCharmap(const FT_Face &face);
	bool init;
	sem_id lock;
	BList *families;
	ServerFont *plain, *bold, *fixed;
	bool need_update;
};

extern FTC_Manager ftmanager; 
extern FT_Library ftlib;
extern FontServer *fontserver;

#endif