#ifndef DEC_VIEW_H_
#define DEC_VIEW_H_

#include <View.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include "ColorSet.h"
#include "LayerData.h"
class PreviewDriver;
class Decorator;

class DecView : public BView
{
public:
	DecView(BRect frame, const char *name, int32 resize, int32 flags);
	~DecView(void);
	void AllAttached(void);
	void MessageReceived(BMessage *msg);
	void SaveSettings(void);
	void LoadSettings(void);
	void SetDefaults(void);
	void NotifyServer(void);
	void GetDecorators(void);
	void SetColors(const BMessage &message);
	bool LoadDecorator(const char *path);
	BString ConvertIndexToPath(int32 index);
protected:
	bool UnpackSettings(ColorSet *set, const BMessage *msg);
	BButton *apply;
	BListView *declist;
	BMessage settings;
	BScrollView *scrollview;
	PreviewDriver *driver;
	BView *preview;
	Decorator *decorator;
	image_id decorator_id;
	BString decpath;
	LayerData ldata;
	uint64 pat_solid_high;
	ColorSet colorset;
};

#endif
