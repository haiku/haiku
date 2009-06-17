/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageListView.h"

#include <stdio.h>

#include <new>

#include <Looper.h>
#include <Message.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "table/TableColumns.h"


enum {
	MSG_SYNC_IMAGE_LIST	= 'sytl'
};


// #pragma mark - ImagesTableModel


class ImageListView::ImagesTableModel : public TableModel {
public:
	ImagesTableModel(Team* team)
		:
		fTeam(team)
	{
		Update();
	}

	~ImagesTableModel()
	{
		fTeam = NULL;
		Update();
	}

	bool Update()
	{
		if (fTeam == NULL) {
			for (int32 i = 0; Image* image = fImages.ItemAt(i); i++)
				image->RemoveReference();
			fImages.MakeEmpty();

			return true;
		}

		AutoLocker<Team> locker(fTeam);

		ImageList::ConstIterator it = fTeam->Images().GetIterator();
		Image* newImage = it.Next();
		int32 index = 0;

		// remove no longer existing images
		while (Image* oldImage = fImages.ItemAt(index)) {
			if (oldImage == newImage) {
				index++;
				newImage = it.Next();
			} else {
				// TODO: Not particularly efficient!
				fImages.RemoveItemAt(index);
				oldImage->RemoveReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new images
		int32 countBefore = fImages.CountItems();
		while (newImage != NULL) {
			if (!fImages.AddItem(newImage))
				return false;

			newImage->AddReference();
			newImage = it.Next();
		}

		int32 count = fImages.CountItems();
		if (count > countBefore)
			NotifyRowsAdded(countBefore, count - countBefore);

		return true;
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual int32 CountRows() const
	{
		return fImages.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, Variant& value)
	{
		Image* image = fImages.ItemAt(rowIndex);
		if (image == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(image->ID());
				return true;
			case 1:
				value.SetTo(image->Name(), VARIANT_DONT_COPY_DATA);
				return true;
			default:
				return false;
		}
	}

	Image* ImageAt(int32 index) const
	{
		return fImages.ItemAt(index);
	}

private:
	Team*				fTeam;
	BObjectList<Image>	fImages;
};


// #pragma mark - ImageListView


ImageListView::ImageListView()
	:
	BGroupView(B_VERTICAL),
	fTeam(NULL),
	fImagesTable(NULL),
	fImagesTableModel(NULL)
{
	SetName("Images");
}


ImageListView::~ImageListView()
{
	SetTeam(NULL);
	fImagesTable->SetTableModel(NULL);
	delete fImagesTableModel;
}


/*static*/ ImageListView*
ImageListView::Create()
{
	ImageListView* self = new ImageListView;

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ImageListView::SetTeam(Team* team)
{
	if (team == fTeam)
		return;

	if (fTeam != NULL) {
		fTeam->RemoveListener(this);
		fImagesTable->SetTableModel(NULL);
		delete fImagesTableModel;
		fImagesTableModel = NULL;
	}

	fTeam = team;

	if (fTeam != NULL) {
		fImagesTableModel = new(std::nothrow) ImagesTableModel(fTeam);
		fImagesTable->SetTableModel(fImagesTableModel);
		fImagesTable->ResizeAllColumnsToPreferred();
		fTeam->AddListener(this);
	}
}


void
ImageListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SYNC_IMAGE_LIST:
			if (fImagesTableModel != NULL)
				fImagesTableModel->Update();
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ImageListView::ImageAdded(Team* team, Image* image)
{
	Looper()->PostMessage(MSG_SYNC_IMAGE_LIST, this);
}


void
ImageListView::ImageRemoved(Team* team, Image* image)
{
	Looper()->PostMessage(MSG_SYNC_IMAGE_LIST, this);
}


void
ImageListView::TableRowInvoked(Table* table, int32 rowIndex)
{
//	if (fImagesTableModel != NULL) {
//		Image* image = fImagesTableModel->ImageAt(rowIndex);
//		if (image != NULL)
//			fParent->OpenImageWindow(image);
//	}
}


void
ImageListView::_Init()
{
	fImagesTable = new Table("images list", 0);
	AddChild(fImagesTable->ToView());
	
	// columns
	fImagesTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fImagesTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	
	fImagesTable->AddTableListener(this);
}
