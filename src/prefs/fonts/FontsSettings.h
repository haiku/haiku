#ifndef FONTS_SETTINGS_H
#define FONTS_SETTINGS_H

class FontsSettings {
	public:
		FontsSettings(void);
		~FontsSettings(void);
		
		BPoint WindowCorner(void) const { return f_corner; }
		void SetWindowCorner(BPoint);
	private:
		static const char kSettingsFile[];
		BPoint f_corner;
};

#endif //FONTS_SETTINGS_H
