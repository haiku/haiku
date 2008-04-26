// ----- UserItem ----------------------------------------------------------------------

class UserItem : public ListItem
{
	public:
		UserItem(BView *headerView, const char *text0, const char *text1, const char *text2);
		~UserItem();

		bool ItemInvoked();
		void ItemSelected();
		void ItemDeselected();
		void ItemDeleted();
		bool IsDeleteable();
};


// ----- UsersItem -----------------------------------------------------

class UsersItem : public TreeItem
{
	public:
		UsersItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text);

		void ItemSelected();
		void ListItemSelected();
		void ListItemDeselected();
		void ListItemUpdated(int index, ListItem *item);
		bool HeaderMessageReceived(BMessage *msg);

	private:
		void BuildHeader();

		BButton *btnEdit;
		BButton *btnRemove;
};
