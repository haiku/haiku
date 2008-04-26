class TreeItem : public CLVListItem
{
	public:
		TreeItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char* text);
		~TreeItem();
		void DrawItemColumn(BView* owner, BRect item_column_rect, int32 column_index, bool complete);
		void Update(BView *owner, const BFont *font);
		static int MyCompare(const CLVListItem* a_Item1, const CLVListItem* a_Item2, int32 KeyColumn);

		ColumnListView *GetAssociatedList()				{ return assocListView; }
		void SetAssociatedList(ColumnListView *list)	{ assocListView = list; }

		BView *GetAssociatedHeader()					{ return assocHeaderView; }
		void SetAssociatedHeader(BView *header)			{ assocHeaderView = header; }

		virtual void ItemSelected();
		virtual void ItemExpanding();
		virtual void ListItemSelected();
		virtual void ListItemDeselected();
		virtual void ListItemUpdated(int index, CLVListItem *item);
		virtual bool HeaderMessageReceived(BMessage *msg);

	protected:
		void PurgeRows();
		void PurgeColumns();
		void PurgeHeader();

	private:
		void setIcon(int32 resourceID);

		BView *assocHeaderView;
		ColumnListView *assocListView;
		BBitmap* fIcon;
		char *fText;
		float fTextOffset;
};

// ----- SmartTreeListView ----------------------------------------------------------------

class SmartTreeListView : public ColumnListView
{
	public:
		SmartTreeListView(BRect Frame, CLVContainerView** ContainerView,
			const char* Name = NULL, uint32 ResizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
			list_view_type Type = B_SINGLE_SELECTION_LIST,
			bool hierarchical = false, bool horizontal = true, bool vertical = true,
			bool scroll_view_corner = true, border_style border = B_NO_BORDER,
			const BFont* LabelFont = be_plain_font);

		virtual ~SmartTreeListView();

		void MouseUp(BPoint point);
		void Expand(TreeItem *item);

	private:
		CLVContainerView *container;
		int lastSelection;
};
