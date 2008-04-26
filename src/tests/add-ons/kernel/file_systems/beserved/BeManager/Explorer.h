// ----- ExplorerFile ----------------------------------------------------------------------

class ExplorerFile : public ListItem
{
	public:
		ExplorerFile(BView *headerView, const char *text0, const char *text1, const char *text2);
		virtual ~ExplorerFile();

		bool ItemInvoked();
};


// ----- ExplorerFolder -----------------------------------------------------

class ExplorerFolder : public TreeItem
{
	public:
		ExplorerFolder(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *treeView, ColumnListView *listView, char *text);

		void ItemExpanding();
		void ItemSelected();

	private:
		ColumnListView *parentTreeView;
};
