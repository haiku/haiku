#ifndef _UserProperties_h_
#define _UserProperties_h_

class BTextControl;
class BCheckBox;
class BMenu;
class BMenuField;

class UserPropertiesView : public BView
{
	public:
		UserPropertiesView(BRect rect, const char *name);
		~UserPropertiesView();

		void Draw(BRect rect);
		void UpdateInfo();
		void SetPath(const char *path);

		char *getUser()			{ return user; }
		char *getFullName()		{ return fullName; }
		char *getDesc()			{ return desc; }
		char *getPassword()		{ return password; }
		char *getHome()			{ return home; }
		char *getGroup()		{ return group; }
		uint32 getFlags()		{ return flags; }
		uint32 getDays()		{ return days; }

	private:
		BBitmap *icon;
		BTextControl *editName;
		BTextControl *editFullName;
		BTextControl *editDesc;
		BTextControl *editPassword;
		BTextControl *editPath;
		BTextControl *editDays;
		BCheckBox *chkDisabled;
		BCheckBox *chkExpiresFirst;
		BCheckBox *chkExpiresEvery;
		BCheckBox *chkCantChange;
		BMenu *mnuGroups;
		BMenuField *mnuDefaultGroup;

		char user[33];
		char fullName[64];
		char desc[64];
		char password[33];
		char home[B_PATH_NAME_LENGTH];
		char group[33];
		uint32 flags;
		uint32 days;

		bool newUser;
};


class UserPropertiesPanel : public BWindow
{
	public:
		UserPropertiesPanel(BRect frame, const char *name, BWindow *parent);

		void MessageReceived(BMessage *msg);

		char *getUser()				{ return user; }
		char *getFullName()			{ return fullName; }
		char *getDesc()				{ return desc; }
		bool isCancelled()			{ return cancelled; }

	private:
		UserPropertiesView *infoView;
		BWindow *shareWin;
		char user[33];
		char fullName[64];
		char desc[64];

		bool newUser;
		bool cancelled;
};
#endif
