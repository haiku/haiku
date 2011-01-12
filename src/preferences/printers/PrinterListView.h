/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _PRINTERS_LISTVIEW_H
#define _PRINTERS_LISTVIEW_H


#include <Directory.h>
#include <Entry.h>
#include <Messenger.h>
#include <ListView.h>
#include <String.h>

#include "FolderWatcher.h"


class SpoolFolder;
class PrinterItem;
class PrinterListView;
class BBitmap;
class PrintersWindow;


struct PrinterListLayoutData
{
	float	fLeftColumnMaximumWidth;
	float	fRightColumnMaximumWidth;
};



class PrinterListView : public BListView, public FolderListener {
public:
								PrinterListView(BRect frame);
								~PrinterListView();

			void				AttachedToWindow();
			bool				QuitRequested();

			void				BuildPrinterList();
			PrinterItem*		SelectedItem() const;
			void				UpdateItem(PrinterItem* item);

			PrinterItem*		ActivePrinter() const;
			void 				SetActivePrinter(PrinterItem* item);

private:
		typedef BListView Inherited;

			void 				_AddPrinter(BDirectory& printer, bool calculateLayout);
			void				_LayoutPrinterItems();
			PrinterItem*		_FindItem(node_ref* node) const;

			void				EntryCreated(node_ref* node,
									entry_ref* entry);
			void				EntryRemoved(node_ref* node);
			void				AttributeChanged(node_ref* node);

			FolderWatcher*		fFolder;
			PrinterItem*		fActivePrinter;
			PrinterListLayoutData	fLayoutData;
};


class PrinterItem : public BListItem {
public:
								PrinterItem(PrintersWindow* window,
									const BDirectory& node,
									PrinterListLayoutData& layoutData);
								~PrinterItem();

			void				GetColumnWidth(BView* view, float& leftColumn,
									float& rightColumn);

			void				DrawItem(BView* owner, BRect bounds,
									bool complete);
			void				Update(BView* owner, const BFont* font);

			bool				Remove(BListView* view);
			bool				IsActivePrinter() const;
			bool				HasPendingJobs() const;

			const char* 		Name() const { return fName.String(); }
			const char*			Driver() const { return fDriverName.String(); }
			const char*			Transport() const { return fTransport.String(); }
			const char*			TransportAddress() const
									{ return fTransportAddress.String(); }

			SpoolFolder* 		Folder() const;
			BDirectory* 		Node();
			void				UpdatePendingJobs();

private:
			void				_GetStringProperty(const char* propName,
									BString& outString);

			SpoolFolder*		fFolder;
			BDirectory			fNode;
			BString				fComments;
			BString				fTransport;
			BString				fTransportAddress;
			BString				fDriverName;
			BString				fName;
			BString				fPendingJobs;
			PrinterListLayoutData& fLayoutData;

	static	BBitmap*			sIcon;
	static	BBitmap*			sSelectedIcon;
};

#endif // _PRINTERS_LISTVIEW_H
