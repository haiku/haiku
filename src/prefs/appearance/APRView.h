#ifndef APR_VIEW_H_
#define APR_VIEW_H_

#include <View.h>
#include <ColorControl.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>

class APRView : public BView
{
public:
	APRView(BRect frame, const char *name, int32 resize, int32 flags);
	~APRView(void);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);
	color_which SelectionToAttribute(int32 index);
	const char *AttributeToString(color_which attr);
	const char *SelectionToString(int32 index);
	void SaveSettings(void);
	void LoadSettings(void);
	void SetDefaults(void);
	void NotifyServer(void);
	rgb_color GetColorFromMessage(BMessage *msg, const char *name, int32 index=0);

	BColorControl *picker;
	BButton *apply,*revert,*defaults,*try_settings;
	BListView *attrlist;
	BView *colorview;
	color_which attribute;
	BMessage settings;
	BString attrstring;
	BScrollView *scrollview;
};

#endif
