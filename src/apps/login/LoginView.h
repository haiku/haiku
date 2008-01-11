#include <View.h>
#include <ListItem.h>
#include <ListView.h>
#include <TextControl.h>

const uint32 kLoginEdited = 'logc';
const uint32 kPasswordEdited = 'pasc';
const uint32 kAddNextUser = 'adnu';
const uint32 kSetProgress = 'setp';

class LoginView : public BView {
	public:
		LoginView(BRect frame);
		virtual ~LoginView();
		void AttachedToWindow();
		void MessageReceived(BMessage *message);
	private:
		void AddNextUser();
		BTextControl *fLoginControl;
		BTextControl *fPasswordControl;
		BListView *fUserList;
};

