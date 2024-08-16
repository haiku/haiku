#ifndef T_ATTRIBUTE_COLUMN_H
#define T_ATTRIBUTE_COLUMN_H


#include <View.h>


class BBox;
class BGroupView;
class BHandler;
class BMessage;
template <typename T> class BObjectList;
class BSize;


namespace BPrivate {

class BColumnTitle;
class TAttributeSearchField;


const uint32 kResizeHeight = 'Frht';

enum class AttrType;

// A BView Class that expands throughout the space occupied by its parent - padding
class TAttributeColumn : public BView {
public:
	virtual						~TAttributeColumn();

			status_t				SetSize(BSize);
			status_t				GetSize(BSize*) const;
			
			status_t				SetColumnTitle(BColumnTitle*);
			status_t				GetColumnTitle(BColumnTitle**) const;

protected:
								TAttributeColumn(BColumnTitle*);
	virtual	void				MessageReceived(BMessage*);

private:
			typedef BView __inherited;
			friend class TFindPanel;

private:
			status_t			HandleColumnUpdate(float height);

protected:
			BColumnTitle*		fColumnTitle;
			BSize				fSize;
};


enum class ColumnCombinationMode {
	AND,
	OR
};

class TAttributeSearchColumn : public TAttributeColumn {
public:
								TAttributeSearchColumn(BColumnTitle*, BView*, AttrType);
	virtual						~TAttributeSearchColumn();

			void				SetColumnCombinationMode(ColumnCombinationMode);
			status_t			GetColumnCombinationMode(ColumnCombinationMode*) const;

			status_t			AddSearchField(int32 index = -1);
			status_t			AddSearchField(TAttributeSearchField* after);
			status_t			RemoveSearchField(TAttributeSearchField*);
			
			status_t			GetRequiredHeight(BSize*) const;
			
			status_t			GetPredicateString(BString*) const;
			
			BMessage			ArchiveToMessage();
			void				RestoreFromMessage(const BMessage*);
protected:
	virtual	void				MessageReceived(BMessage*);

private:
			status_t			ResizeBox(BSize);
			status_t			GetBoxSize(BSize*) const;

public:
			AttrType			fAttrType;
			ColumnCombinationMode	fCombinationMode;
			BObjectList<TAttributeSearchField>* fSearchFields;
			
private:
			BHandler*			fParent;
			BGroupView*			fContainerView;
			BBox*				fBox;

			typedef TAttributeColumn __inherited;
			friend class TAttributeColumn;
};


class TDisabledSearchColumn : public TAttributeColumn {
public:
								TDisabledSearchColumn(BColumnTitle*);
								~TDisabledSearchColumn();

private:
			typedef TAttributeColumn __inherited;
};

}

#endif
