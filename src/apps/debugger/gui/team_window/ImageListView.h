/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_LIST_VIEW_H
#define IMAGE_LIST_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class ImageListView : public BGroupView, private Team::Listener,
	private TableListener {
public:
								ImageListView();
								~ImageListView();

	static	ImageListView*		Create();
									// throws

			void				SetTeam(Team* team);

	virtual	void				MessageReceived(BMessage* message);

private:
			class ImagesTableModel;

private:
	// Team::Listener
	virtual	void				ImageAdded(const Team::ImageEvent& event);
	virtual	void				ImageRemoved(const Team::ImageEvent& event);

	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

			void				_Init();

private:
			Team*				fTeam;
			Table*				fImagesTable;
			ImagesTableModel*	fImagesTableModel;
};


#endif	// IMAGE_LIST_VIEW_H
