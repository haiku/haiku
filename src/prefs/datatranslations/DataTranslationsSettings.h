#ifndef DATA_TRANSLATIONS_SETTINGS_H_
#define DATA_TRANSLATIONS_SETTINGS_H_

class DataTranslationsSettings{
public :
	DataTranslationsSettings();
	~DataTranslationsSettings();
	
	BPoint WindowCorner() const { return fCorner; }
	void SetWindowCorner(BPoint corner);
	
private:
	static const char 	kDataTranslationsSettingsFile[];
	BPoint				fCorner;
};

#endif