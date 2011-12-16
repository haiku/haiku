/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "ImageListView.h"

#include <stdio.h>

#include <new>

#include <Looper.h>
#include <Message.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "GUISettingsUtils.h"
#include "table/TableColumns.h"
#include "Tracing.h"


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
				image->ReleaseReference();
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
				oldImage->ReleaseReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new images
		int32 countBefore = fImages.CountItems();
		while (newImage != NULL) {
			if (!fImages.AddItem(newImage))
				return false;

			newImage->AcquireReference();
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

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		Image* image = fImages.ItemAt(rowIndex);
		if (image == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(image->ID());
				return true;
			case 1:
				value.SetTo(image->Name(), B_VARIANT_DONT_COPY_DATA);
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


ImageListView::ImageListView(Team* team, Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fImage(NULL),
	fImagesTable(NULL),
	fImagesTableModel(NULL),
	fListener(listener)
{
	SetName("Images");

	fTeam->AddListener(this);
}


ImageListView::~ImageListView()
{
	SetImage(NULL);
	fTeam->RemoveListener(this);
	fImagesTable->SetTableModel(NULL);
	delete fImagesTableModel;
}


/*static*/ ImageListView*
ImageListView::Create(Team* team, Listener* listener)
{
	ImageListView* self = new ImageListView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ImageListView::UnsetListener()
{
	fListener = NULL;
}


void
ImageListView::SetImage(Image* image)
{
	if (image == fImage)
		return;

	TRACE_GUI("ImageListView::SetImage(%p)\n", image);

	if (fImage != NULL)
		fImage->ReleaseReference();

	fImage = image;

	if (fImage != NULL) {
		fImage->AcquireReference();

		for (int32 i = 0; Image* other = fImagesTableModel->ImageAt(i); i++) {
			if (fImage == other) {
				fImagesTable->SelectRow(i, false);

				TRACE_GUI("ImageListView::SetImage() done\n");
				return;
			}
		}
	}

	fImagesTable->DeselectAllRows();

	TRACE_GUI("ImageListView::SetImage() done\n");
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
ImageListView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("imagesTable", &tableSettings) == B_OK) {
		GUISettingsUtils::UnarchiveTableSettings(tableSettings,
			fImagesTable);
	}
}


status_t
ImageListView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GUISettingsUtils::ArchiveTableSettings(tableSettings,
		fImagesTable);
	if (result == B_OK)
		result = settings.AddMessage("imagesTable", &tableSettings);

	return result;
}


void
ImageListView::ImageAdded(const Team::ImageEvent& event)
{
	Looper()->PostMessage(MSG_SYNC_IMAGE_LIST, this);
}


void
ImageListView::ImageRemoved(const Team::ImageEvent& event)
{
	Looper()->PostMessage(MSG_SYNC_IMAGE_LIST, this);
}


void
ImageListView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL)
		return;

	Image* image = NULL;
	if (fImagesTableModel != NULL) {
		TableSelectionModel* selectionModel = table->SelectionModel();
		image = fImagesTableModel->ImageAt(selectionModel->RowAt(0));
	}

	fListener->ImageSelectionChanged(image);
}


void
ImageListView::_Init()
{
	fImagesTable = new Table("images list", 0, B_FANCY_BORDER);
	AddChild(fImagesTable->ToView());

	// columns
	fImagesTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fImagesTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fImagesTable->AddTableListener(this);

	fImagesTableModel = new ImagesTableModel(fTeam);
	fImagesTable->SetTableModel(fImagesTableModel);
	fImagesTable->ResizeAllColumnsToPreferred();
}


// #pragma mark - Listener


ImageListView::Listener::~Listener()
{
}
