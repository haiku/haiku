#ifndef T_ATTRIBUTE_SEARCH_FIELD_H
#define T_ATTRIBUTE_SEARCH_FIELD_H


#include <Catalog.h>
#include <Locale.h>
#include <Query.h>
#include <View.h>


class BButton;
class BMenu;
class BMenuItem;
class BMessage;
class BStringView;


namespace BPrivate {

class PopUpTextControl;
class TAttributeSearchColumn;


const uint32 kMenuOptionClicked = 'Fmcl';
const uint32 kAddSearchField = 'Fasf';
const uint32 kRemoveSearchField = 'Frsf';
const uint32 kModifiedField = 'Fmsf';


enum class AttrType {
	REGULAR_ATTRIBUTE,
	SIZE,
	MODIFIED
};


class TAttributeSearchField : public BView {
public:
								TAttributeSearchField(TAttributeSearchColumn*, AttrType,
									const char*);
								~TAttributeSearchField();

			status_t			GetPredicateString(BString*) const;
			status_t			GetAttributeOperator(query_op*) const;
			status_t			GetCombinationOperator(query_op*) const;

			void				EnableRemoveButton(bool enable = true);
			void				EnableAddButton(bool enable = true);
			
			status_t			GetSize(BSize*) const;
			status_t			SetSize(BSize);
			status_t			GetRequiredHeight(BSize*) const;

			BMessage			ArchiveToMessage();
			void				RestoreFromMessage(const BMessage*);

			void				UpdateLabel();

			PopUpTextControl*	TextControl() const {return fTextControl;};

protected:
	virtual	void				MessageReceived(BMessage*);
	virtual	void				AttachedToWindow();

private:
			status_t			HandleMenuOptionClicked(BMenu*, BMenuItem*, bool);

private:
			BButton*				fAddButton;
			BButton*				fRemoveButton;
			PopUpTextControl*		fTextControl;
			BStringView*			fLabel;
			TAttributeSearchColumn*	fParent;
			AttrType				fAttrType;
			const char*				fAttrName;
			
			BSize					fSize;
		
			typedef BView __inherited;
			friend class TAttributeSearchColumn;
};

}

#endif