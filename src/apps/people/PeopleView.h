/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PEOPLE_VIEW_H
#define PEOPLE_VIEW_H


#include <GridView.h>
#include <ObjectList.h>
#include <String.h>


class AttributeTextControl;
class BFile;
class BPopUpMenu;

enum {
	M_SAVE			= 'save',
	M_REVERT		= 'rvrt',
	M_SELECT		= 'slct',
	M_GROUP_MENU	= 'grmn',
};


class TPeopleView : public BGridView {
public:
								TPeopleView(const char* name,
									const char* categoryAttribute,
									const entry_ref* ref);
	virtual						~TPeopleView();

	virtual	void				MakeFocus(bool focus = true);
	virtual	void				MessageReceived(BMessage* message);

			void				AddAttribute(const char* label,
									const char* attribute);

			void				BuildGroupMenu();

			void				CreateFile(const entry_ref* ref);

			bool				IsSaved() const;
			void				Save();

			const char*			AttributeValue(const char* attribute) const;
			void				SetAttribute(const char* attribute, bool update);
			void				SetAttribute(const char* attribute,
									const char* value, bool update);

			bool				IsTextSelected() const;

private:
			BFile*				fFile;
			BPopUpMenu*			fGroups;
			typedef BObjectList<AttributeTextControl> AttributeList;
			AttributeList		fControls;

			BString				fCategoryAttribute;
};

#endif // PEOPLE_VIEW_H
