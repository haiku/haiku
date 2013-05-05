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
	class Listener;

public:
								ImageListView(Team* team, Listener* listener);
								~ImageListView();

	static	ImageListView*		Create(Team* team, Listener* listener);
									// throws

			void				UnsetListener();

			void				SetImage(Image* image);

	virtual	void				MessageReceived(BMessage* message);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class ImagesTableModel;

private:
	// Team::Listener
	virtual	void				ImageAdded(const Team::ImageEvent& event);
	virtual	void				ImageRemoved(const Team::ImageEvent& event);

	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void				_Init();

private:
			Team*				fTeam;
			Image*				fImage;
			Table*				fImagesTable;
			ImagesTableModel*	fImagesTableModel;
			Listener*			fListener;
};


class ImageListView::Listener {
public:
	virtual						~Listener();

	virtual	void				ImageSelectionChanged(Image* image) = 0;
};


#endif	// IMAGE_LIST_VIEW_H
