#ifndef __ABOUTWINDOW_H__
#define __ABOUTWINDOW_H__

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <StringView.h>

class CTextView;
class BitmapView;

typedef struct {
uint32 v1;
uint32 v2;
uint32 v3;
uint32 status;
uint32 rel;
} app_version_info;

/**********************************************************************
 * HAboutView
 **********************************************************************/
class HAboutView : public BView
{
public:
				HAboutView(BRect rect,BBitmap *icon = NULL);
	virtual 	~HAboutView();
	void		SetBitmap(BBitmap* bitmap);
protected:
			
	virtual void Draw(BRect);
private:
	BBitmap*	fIcon;
};
/**********************************************************************
 * HAboutWindow
 **********************************************************************/
class HAboutWindow :public BWindow {
public:					
					HAboutWindow(const char* app_name,
								const char* built_data,
								const char* comment,
								const char* url = NULL,
								const char* mail = NULL);	
protected:
			void	InitGUI();
			void	SetAppName(const char* app_name);
			void	SetBuiltDate(const char* text);
			void	SetVersion(const char* version);
			void	SetComment(const char* text);
			void	LoadIcon();
			void	LoadVersion();
			void	CalcFontHeights();
private:
		BStringView*	fAppName;
		BStringView*	fVersion;
		BStringView*	fBuiltDate;
		HAboutView*		fAboutView;
		CTextView*		fComment;		
		float			fBoldHeight;
		float			fPlainHeight;	
};



#endif