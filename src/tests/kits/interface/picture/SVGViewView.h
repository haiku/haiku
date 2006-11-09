#ifndef _SVG_VIEW_VIEW_H
#define _SVG_VIEW_VIEW_H

#include <String.h>
#include <View.h>


class BPicture;
class Svg2PictureView : public BView {
public:
	Svg2PictureView(BRect frame, const char *filename);
	~Svg2PictureView();

	virtual void AttachedToWindow();
        virtual void Draw(BRect updateRect);

private:
	BString     fFilename;
	BPicture    *fPicture;
};

#endif // _SVG_VIEW_VIEW_H
