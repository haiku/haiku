/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <TextTable.h>

#include <stdio.h>

#include <algorithm>


namespace BPrivate {


// #pragma mark - Column


struct TextTable::Column {
	Column(const BString& title, enum alignment align, bool canTruncate)
		:
		fTitle(title),
		fAlignment(align),
		fCanBeTruncated(canTruncate),
		fNeededWidth(0),
		fWidth(0)
	{
		UpdateNeededWidth(fTitle);
		fMinWidth = fNeededWidth;
	}

	const BString& Title() const
	{
		return fTitle;
	}

	enum alignment Alignment() const
	{
		return fAlignment;
	}

	bool CanBeTruncated() const
	{
		return fCanBeTruncated;
	}

	int32 NeededWidth() const
	{
		return fNeededWidth;
	}

	int32 MinWidth() const
	{
		return fMinWidth;
	}

	int32 Width() const
	{
		return fWidth;
	}

	void SetWidth(int32 width)
	{
		fWidth = width;
	}

	void UpdateNeededWidth(const BString& text)
	{
		// TODO: Full-width character support.
		int32 textWidth = text.CountChars();
		if (textWidth > fNeededWidth)
			fNeededWidth = textWidth;
	}

	BString Format(const BString& text)
	{
		// TODO: Full-width character support.
		int32 textWidth = text.CountChars();
		if (textWidth == fWidth)
			return text;

		// truncate, if too long
		if (textWidth > fWidth) {
			BString result(text);
			result.TruncateChars(fWidth);
			return result;
		}

		// align, if too short
		int32 missing = fWidth - textWidth;
		switch (fAlignment) {
			case B_ALIGN_LEFT:
			default:
			{
				BString result(text);
				result.Append(' ', missing);
				return result;
			}

			case B_ALIGN_RIGHT:
			{
				BString result;
				result.Append(' ', missing);
				result.Append(text);
				return result;
			}

			case B_ALIGN_CENTER:
			{
				BString result;
				result.Append(' ', missing / 2);
				result.Append(text);
				result.Append(' ', missing - missing / 2);
				return result;
			}
		}
	}

private:
	BString			fTitle;
	enum alignment fAlignment;
	bool			fCanBeTruncated;
	int32			fNeededWidth;
	int32			fMinWidth;
	int32			fWidth;
};


// #pragma mark - TextTable


TextTable::TextTable()
	:
	fColumns(10, true),
	fRows(100, true)
{
}


TextTable::~TextTable()
{
}


int32
TextTable::CountColumns() const
{
	return fColumns.CountItems();
}


void
TextTable::AddColumn(const BString& title, enum alignment align,
	bool canTruncate)
{
	Column* column = new Column(title, align, canTruncate);
	if (!fColumns.AddItem(column)) {
		delete column;
		throw std::bad_alloc();
	}
}


int32
TextTable::CountRows() const
{
	return fRows.CountItems();
}


BString
TextTable::TextAt(int32 rowIndex, int32 columnIndex) const
{
	BStringList* row = fRows.ItemAt(rowIndex);
	if (row == NULL)
		return BString();
	return row->StringAt(columnIndex);
}


void
TextTable::SetTextAt(int32 rowIndex, int32 columnIndex, const BString& text)
{
	// If necessary append empty rows up to the specified row index.
	while (rowIndex >= fRows.CountItems()) {
		BStringList* row = new BStringList();
		if (!fRows.AddItem(row)) {
			delete row;
			throw std::bad_alloc();
		}
	}

	// If necessary append empty strings up to the specified column index.
	BStringList* row = fRows.ItemAt(rowIndex);
	while (columnIndex >= row->CountStrings()) {
		if (!row->Add(BString()))
			throw std::bad_alloc();
	}

	// set the text
	if (!row->Replace(columnIndex, text))
		throw std::bad_alloc();
}


void
TextTable::Print(int32 maxWidth)
{
	int32 columnCount = fColumns.CountItems();
	if (columnCount == 0)
		return;

	// determine the column widths
	int32 rowCount = fRows.CountItems();
	for (int32 rowIndex = 0; rowIndex < rowCount; rowIndex++) {
		BStringList* row = fRows.ItemAt(rowIndex);
		int32 rowColumnCount = std::min(row->CountStrings(), columnCount);
		for (int32 columnIndex = 0; columnIndex < rowColumnCount;
			columnIndex++) {
			fColumns.ItemAt(columnIndex)->UpdateNeededWidth(
				row->StringAt(columnIndex));
		}
	}

	int32 neededWidth = (columnCount - 1) * 2;
		// spacing
	for (int32 i = 0; i < columnCount; i++)
		neededWidth += fColumns.ItemAt(i)->NeededWidth();

	int32 width = neededWidth;
	int32 missingWidth = neededWidth - std::min(maxWidth, neededWidth);

	for (int32 i = 0; i < columnCount; i++) {
		Column* column = fColumns.ItemAt(i);
		if (missingWidth > 0 && column->CanBeTruncated()) {
			int32 truncateBy = std::min(missingWidth,
				column->NeededWidth() - column->MinWidth());
			column->SetWidth(column->NeededWidth() - truncateBy);
			missingWidth -= truncateBy;
			width -= truncateBy;
		} else
			column->SetWidth(column->NeededWidth());
	}

	// print the header
	BString line;
	for (int32 i = 0; i < columnCount; i++) {
		if (i > 0)
			line << "  ";

		Column* column = fColumns.ItemAt(i);
		line << column->Format(column->Title());
	}
	line << '\n';
	fputs(line.String(), stdout);

	line.SetTo('-', width);
	line << '\n';
	fputs(line.String(), stdout);

	// print the rows
	for (int32 rowIndex = 0; rowIndex < rowCount; rowIndex++) {
		line.Truncate(0);
		BStringList* row = fRows.ItemAt(rowIndex);
		for (int32 columnIndex = 0; columnIndex < columnCount; columnIndex++) {
			if (columnIndex > 0)
				line << "  ";

			line << fColumns.ItemAt(columnIndex)->Format(
				row->StringAt(columnIndex));
		}

		line << '\n';
		fputs(line.String(), stdout);
	}
}


} // namespace BPrivate
