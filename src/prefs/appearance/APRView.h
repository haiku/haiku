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
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <FilePanel.h>
#include <Invoker.h>

class ColorWell;
class APRWindow;

class APRView : public BView
{
public:
	APRView(const BRect &frame, const char *name, int32 resize, int32 flags);
	~APRView(void);
	void AllAttached(void);
	void MessageReceived(BMessage *msg);
	color_which SelectionToAttribute(int32 index);
	const char *AttributeToString(const color_which &attr);
	const char *SelectionToString(int32 index);
	void SaveSettings(void);
	void LoadSettings(void);
	void SetDefaults(void);
	void NotifyServer(void);
	rgb_color GetColorFromMessage(BMessage *msg, const char *name, int32 index=0);
protected:
	friend APRWindow;
	BMenu *LoadColorSets(void);
	void SaveColorSet(const BString &name);
	void LoadColorSet(const BString &name);
	void SetColorSetName(const char *name);
	BColorControl *picker;
	BButton *apply,*revert,*defaults,*try_settings;
	BListView *attrlist;
	color_which attribute;
	BMessage settings;
	BString attrstring;
	BScrollView *scrollview;
	BStringView *colorset_label;
	BMenu *colorset_menu,*settings_menu;
	BFilePanel *savepanel;
	ColorWell *colorwell;
	BString *colorset_name;
};

#endif
