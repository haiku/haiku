#ifndef COLOR_MENU_ITEM_H
#define COLOR_MENU_ITEM_H

class ColorMenuItem: public BMenuItem {
	public:
		ColorMenuItem(const char *label, rgb_color color, BMessage *message);
	protected:
		virtual	void	DrawContent();
	private:	
		rgb_color		fItemColor;
	
};
#endif

