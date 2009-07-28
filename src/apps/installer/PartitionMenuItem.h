/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PARTITION_MENU_ITEM_H
#define PARTITION_MENU_ITEM_H

#include <MenuItem.h>
#include <Partition.h>


const uint32 SOURCE_PARTITION = 'iSPT';
const uint32 TARGET_PARTITION = 'iTPT';


class PartitionMenuItem : public BMenuItem {
public:
								PartitionMenuItem(const char* name,
									const char* label, const char* menuLabel,
									BMessage* msg, partition_id id);
	virtual						~PartitionMenuItem();

			partition_id		ID() const;
			const char*			MenuLabel() const;
			const char*			Name() const;

			void				SetIsValidTarget(bool isValidTarget);
			bool				IsValidTarget() const;

private:
			partition_id		fID;
			char*				fMenuLabel;
			char*				fName;
			bool				fIsValidTarget;
};

#endif // PARTITION_MENU_ITEM_H_
