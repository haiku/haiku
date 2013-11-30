/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEXT_TABLE_H
#define TEXT_TABLE_H


#include <InterfaceDefs.h>
#include <ObjectList.h>
#include <StringList.h>


namespace BPrivate {


class TextTable {
public:
								TextTable();
								~TextTable();

			int32				CountColumns() const;
			void				AddColumn(const BString& title,
									enum alignment align = B_ALIGN_LEFT,
									bool canTruncate = false);

			int32				CountRows() const;
			BString				TextAt(int32 rowIndex, int32 columnIndex) const;
			void				SetTextAt(int32 rowIndex, int32 columnIndex,
									const BString& text);

			void				Print(int32 maxWidth);

private:
			struct Column;
			typedef BObjectList<Column> ColumnList;
			typedef BObjectList<BStringList> RowList;

private:
			ColumnList			fColumns;
			RowList				fRows;
};


} // namespace BPrivate


using ::BPrivate::TextTable;


#endif	// TEXT_TABLE_H
