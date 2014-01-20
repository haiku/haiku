/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#define CLVColumnLabelView_CPP

#include "CLVColumnLabelView.h"
#include "ColumnListView.h"
#include "CLVColumn.h"
#include "MouseWatcher.h"


CLVColumnLabelView::CLVColumnLabelView(BRect frame, ColumnListView* parent,
	const BFont* font)
	:
	BView(frame, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fDragGroupsList(10)
{
	SetFont(font);
	SetViewColor(BeBackgroundGrey);
	SetLowColor(BeBackgroundGrey);
	SetHighColor(Black);
	fParent = parent;
	fDisplayList = &fParent->fColumnDisplayList;
	fColumnClicked = NULL;
	fColumnDragging = false;
	fColumnResizing = false;
	font_height fontAttributes;
	font->GetHeight(&fontAttributes);
	fFontAscent = ceil(fontAttributes.ascent);
}


CLVColumnLabelView::~CLVColumnLabelView()
{
	int32 groupCount = fDragGroupsList.CountItems();
	for (int32 i = 0; i < groupCount; i++)
		fDragGroupsList.RemoveItem(int32(0));
}


void
CLVColumnLabelView::Draw(BRect updateRect)
{
	BRegion clippingRegion;
	GetClippingRegion(&clippingRegion);
	BRect bounds = Bounds();

	// draw each column label in turn
	float columnBegin = 0.0;
	float columnEnd = -1.0;
	bool shouldMergeWithLeft = false;
	int32 columnCount = fDisplayList->CountItems();
	BPoint startingPoint;
	BPoint stoppingPoint;
	for (int32 i = 0; i < columnCount; i++) {
		CLVColumn* column = (CLVColumn*)fDisplayList->ItemAt(i);
		if (column->IsShown()) {
			// figure out where this column is
			columnBegin = column->fColumnBegin;
			columnEnd = column->fColumnEnd;
			// start by figuring out if this column will merge
			// with a shown column to the right
			bool shouldMergeWithRight = false;
			if ((column->fFlags & CLV_MERGE_WITH_RIGHT) != 0) {
				for (int32 j = i + 1; j < columnCount; j++) {
					CLVColumn* nextColumn = (CLVColumn*)fDisplayList->ItemAt(j);
					if (nextColumn->IsShown()) {
						// next column is shown
						shouldMergeWithRight = true;
						break;
					} else if ((nextColumn->fFlags & CLV_MERGE_WITH_RIGHT)
							== 0) {
						// next column is not shown and doesn't
						// pass on the merge
						break;
					}
				}
			}

			if (clippingRegion.Intersects(BRect(columnBegin,
					bounds.top, columnEnd, bounds.bottom))) {
				// need to draw this column
				BeginLineArray(4);

				// top line
				startingPoint.Set(columnBegin, bounds.top);
				stoppingPoint.Set(columnEnd - 1.0, bounds.top);
				if (shouldMergeWithRight && !(column == fColumnClicked
					&& fColumnResizing)) {
					stoppingPoint.x = columnEnd;
				}
				AddLine(startingPoint, stoppingPoint, BeHighlight);

				// left line
				if (!shouldMergeWithLeft) {
					AddLine(BPoint(columnBegin, bounds.top + 1.0),
						BPoint(columnBegin, bounds.bottom), BeHighlight);
				}

				// bottom line
				startingPoint.Set(columnBegin + 1.0, bounds.bottom);
				if (shouldMergeWithLeft)
					startingPoint.x = columnBegin;

				stoppingPoint.Set(columnEnd - 1.0, bounds.bottom);
				if (shouldMergeWithRight && !(column == fColumnClicked
					&& fColumnResizing)) {
					stoppingPoint.x = columnEnd;
				}
				AddLine(startingPoint, stoppingPoint, BeShadow);

				// right line
				if (column == fColumnClicked && fColumnResizing) {
					AddLine(BPoint(columnEnd, bounds.top),
					BPoint(columnEnd, bounds.bottom), BeFocusBlue);
				} else if (!shouldMergeWithRight) {
					AddLine(BPoint(columnEnd, bounds.top),
						BPoint(columnEnd, bounds.bottom), BeShadow);
				}
				EndLineArray();

				// add the label
				if (column->fLabel != NULL) {
					// limit the clipping region to the interior of the box
					BRegion textRegion;
					textRegion.Include(BRect(columnBegin + 1.0,
						bounds.top + 1.0, columnEnd - 1.0,
						bounds.bottom - 1.0));
					ConstrainClippingRegion(&textRegion);

					// draw the label
					if (column == fColumnClicked && !fColumnResizing)
						SetHighColor(BeFocusBlue);

					SetDrawingMode(B_OP_OVER);
					DrawString(column->fLabel,
						BPoint(columnBegin + 9.0,
							bounds.top + 2.0 + fFontAscent));
					SetDrawingMode(B_OP_COPY);

					// underline if this is a selected sort column
					if (fParent->fSortKeyList.HasItem(column)
						&& column->fSortMode != SORT_MODE_NONE) {
						float Width = StringWidth(column->fLabel);
						StrokeLine(BPoint(columnBegin + 8.0,
							bounds.top + 2.0 + fFontAscent + 2.0),
							BPoint(columnBegin + 8.0 + Width,
								bounds.top + 2.0 + fFontAscent + 2.0));
					}
					if (column == fColumnClicked && !fColumnResizing)
						SetHighColor(Black);

					// restore the clipping region
					ConstrainClippingRegion(NULL);
				}
			}

			// set shouldMergeWithLeft flag for the next column to
			// the appropriate state
			shouldMergeWithLeft = shouldMergeWithRight;
		}
	}

	// add highlight and shadow to the region after the columns if necessary
	if (columnEnd < bounds.right) {
		columnBegin = columnEnd + 1.0;

		if (clippingRegion.Intersects(BRect(columnEnd + 1.0,
				bounds.top, bounds.right, bounds.bottom))) {
			BeginLineArray(3);

			// top line
			AddLine(BPoint(columnBegin, bounds.top),
				BPoint(bounds.right, bounds.top), BeHighlight);

			// left line
			AddLine(BPoint(columnBegin, bounds.top + 1.0),
				BPoint(columnBegin, bounds.bottom), BeHighlight);

			// bottom line
			startingPoint.Set(columnBegin + 1.0, bounds.bottom);
			if (shouldMergeWithLeft)
				startingPoint.x = columnBegin;

			stoppingPoint.Set(bounds.right, bounds.bottom);
			AddLine(startingPoint, stoppingPoint, BeShadow);
			EndLineArray();
		}
	}

	// draw the dragging box if necessary
	if (fColumnClicked && fColumnDragging) {
		float dragOutlineLeft = fPreviousMousePosition.x
			- fDragBoxMouseHoldOffset;
		float groupBegin = ((CLVDragGroup*)fDragGroupsList.ItemAt(
			fDragGroupIndex))->groupBeginIndex;
		if (dragOutlineLeft < groupBegin && fSnapGroupBeforeIndex == -1)
			dragOutlineLeft = groupBegin;

		if (dragOutlineLeft > groupBegin && fSnapGroupAfterIndex == -1)
			dragOutlineLeft = groupBegin;

		float DragOutlineRight = dragOutlineLeft + fDragBoxWidth;

		BeginLineArray(4);
		AddLine(BPoint(dragOutlineLeft, bounds.top),
			BPoint(DragOutlineRight, bounds.top), BeFocusBlue);
		AddLine(BPoint(dragOutlineLeft, bounds.bottom),
			BPoint(DragOutlineRight, bounds.bottom), BeFocusBlue);
		AddLine(BPoint(dragOutlineLeft, bounds.top + 1.0),
			BPoint(dragOutlineLeft, bounds.bottom - 1.0), BeFocusBlue);
		AddLine(BPoint(DragOutlineRight, bounds.top + 1.0),
			BPoint(DragOutlineRight, bounds.bottom - 1.0), BeFocusBlue);
		EndLineArray();

		fPreviousDragOutlineLeft = dragOutlineLeft;
		fPreviousDragOutlineRight = DragOutlineRight;
	}
}


void
CLVColumnLabelView::MouseDown(BPoint where)
{
	// pay attention only to primary mouse button
	bool shouldWatchMouse = false;
	BPoint mousePosition;
	uint32 buttons;
	GetMouse(&mousePosition, &buttons);
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		BRect bounds = Bounds();

		// Make sure no other column was already clicked.
		// If so, just discard the old one and redraw the view.
		if (fColumnClicked != NULL) {
			Invalidate();
			fColumnClicked = NULL;
		}

		// find the column that the user clicked, if any
		bool didGrabResizeTab = false;
		int32 columnCount = fDisplayList->CountItems();
		int32 i;
		CLVColumn* column = NULL;
		for (i = 0; i < columnCount; i++) {
			column = (CLVColumn*)fDisplayList->ItemAt(i);
			if (column->IsShown()) {
				float columnBegin = column->fColumnBegin;
				float columnEnd = column->fColumnEnd;
				if ((where.x >= columnBegin && where.x <= columnEnd)
					|| ((i == columnCount - 1) && (where.x >= columnBegin))) {
					// anything after the rightmost column can drag... jaf
				    const float resizeTolerance = 5.0f;
					// jaf is too clumsy to click on a 2 pixel space.  :)

					// user clicked in this column
					if (where.x <= columnBegin + resizeTolerance) {
						// user clicked the resize tab preceding this column
						for (i--; i >= 0; i--) {
							column = (CLVColumn*)fDisplayList->ItemAt(i);
							if (column->IsShown()) {
								didGrabResizeTab = true;
								break;
							}
						}
					} else if (where.x >= columnEnd-resizeTolerance) {
						// user clicked the resize tab for (after) this column
						didGrabResizeTab = true;
					} else {
						// user clicked in this column
						fColumnClicked = (CLVColumn*)fDisplayList->ItemAt(i);
						fColumnResizing = false;
						fPreviousMousePosition = where;
						fMouseClickedPosition = where;
						fColumnDragging = false;
						SetSnapMinMax();
						fDragBoxMouseHoldOffset = where.x
							- ((CLVDragGroup*)fDragGroupsList.ItemAt(
								fDragGroupIndex))->groupBeginIndex;

							Invalidate(BRect(columnBegin + 1.0,
								bounds.top + 1.0, columnEnd - 1.0,
								bounds.bottom - 1.0));

						// start watching the mouse
						shouldWatchMouse = true;
					}
					break;
				}
			}
		}

		if (didGrabResizeTab) {
			// user grabbed a resize tab, check to see if
			// resizing this column is allowed
			if ((column->fFlags & CLV_NOT_RESIZABLE) == 0) {
				fColumnClicked = (CLVColumn*)fDisplayList->ItemAt(i);
				fColumnResizing = true;
				fPreviousMousePosition = where;
				fMouseClickedPosition = where;
				fColumnDragging = false;
				fResizeMouseHoldOffset = where.x - fColumnClicked->fColumnEnd;
				Invalidate(BRect(fColumnClicked->fColumnEnd, bounds.top,
					column->fColumnEnd, bounds.bottom));

				// start watching the mouse
				shouldWatchMouse = true;
			}
		}
	}

	if (shouldWatchMouse) {
		thread_id MouseWatcherThread = StartMouseWatcher(this);
		if (MouseWatcherThread == B_NO_MORE_THREADS
			|| MouseWatcherThread == B_NO_MEMORY) {
			fColumnClicked = NULL;
		}
	}
}


void
CLVColumnLabelView::MessageReceived(BMessage* message)
{
	if (message->what != MW_MOUSE_MOVED && message->what != MW_MOUSE_DOWN
		&& message->what != MW_MOUSE_UP) {
		BView::MessageReceived(message);
	} else if (fColumnClicked != NULL) {
		BPoint mousePosition;
		message->FindPoint("where", &mousePosition);
		uint32 buttons;
		message->FindInt32("buttons", (int32*)&buttons);
		uint32 modifiers;
		message->FindInt32("modifiers", (int32*)&modifiers);
		BRect bounds;
		bounds = Bounds();
		uint32 columnFlags = fColumnClicked->Flags();
		if (buttons == B_PRIMARY_MOUSE_BUTTON) {
			// mouse is still held down
			if (!fColumnResizing) {
				// user is clicking or dragging
				if ((mousePosition.x < fMouseClickedPosition.x - 2.0
						|| mousePosition.x > fMouseClickedPosition.x + 2.0)
					&& !fColumnDragging) {
					// user is initiating a drag
					if ((fDragGroup->flags & CLV_NOT_MOVABLE) != 0) {
						// not allowed to drag this column - terminate the click
						Invalidate(BRect(fColumnClicked->fColumnBegin,
							bounds.top, fColumnClicked->fColumnEnd,
							bounds.bottom));
						fColumnClicked = NULL;
					} else {
						// actually initiate a drag
						fColumnDragging = true;
						fPreviousDragOutlineLeft = -1.0;
						fPreviousDragOutlineRight = -1.0;
					}
				}

				// now deal with dragging
				if (fColumnDragging) {
					// user is dragging
					if (mousePosition.x < fPreviousMousePosition.x
						|| mousePosition.x > fPreviousMousePosition.x) {
						// mouse moved since I last checked
						bounds = Bounds();

						bool hasColumnSnapped;
						do {
							// live dragging of columns
							hasColumnSnapped = false;
							float columnsUpdateLeft = 0;
							float columnsUpdateRight = 0;
							float MainViewUpdateLeft = 0;
							float MainViewUpdateRight = 0;
							CLVColumn* lastSwapColumn = NULL;
							if (fSnapMin != -1.0
								&& mousePosition.x < fSnapMin) {
								// shift the group left
								columnsUpdateLeft
									= fShownGroupBefore->groupBeginIndex;
								columnsUpdateRight = fDragGroup->groupEndIndex;
								MainViewUpdateLeft = columnsUpdateLeft;
								MainViewUpdateRight = columnsUpdateRight;
								lastSwapColumn
									= fShownGroupBefore->lastColumnShown;
								if ((fDragGroup->lastColumnShown->fFlags
										& CLV_MERGE_WITH_RIGHT) != 0) {
									columnsUpdateRight += 1.0;
								} else if ((fShownGroupBefore->
									lastColumnShown->fFlags
										& CLV_MERGE_WITH_RIGHT) != 0) {
									columnsUpdateRight += 1.0;
								}

								ShiftDragGroup(fSnapGroupBeforeIndex);
								hasColumnSnapped = true;
							}

							if (fSnapMax != -1.0
								&& mousePosition.x > fSnapMax) {
								// shift the group right
								columnsUpdateLeft = fDragGroup->groupBeginIndex;
								columnsUpdateRight
									= fShownGroupAfter->groupEndIndex;
								MainViewUpdateLeft = columnsUpdateLeft;
								MainViewUpdateRight = columnsUpdateRight;
								lastSwapColumn = fDragGroup->lastColumnShown;
								if ((fDragGroup->lastColumnShown->fFlags
										& CLV_MERGE_WITH_RIGHT) != 0) {
									columnsUpdateRight += 1.0;
								} else if ((fShownGroupAfter
									->lastColumnShown->fFlags
										& CLV_MERGE_WITH_RIGHT) != 0) {
									columnsUpdateRight += 1.0;
								}
								ShiftDragGroup(fSnapGroupAfterIndex + 1);
								hasColumnSnapped = true;
							}

							if (hasColumnSnapped) {
								// redraw the snapped column labels
								Invalidate(BRect(columnsUpdateLeft,
									bounds.top, columnsUpdateRight,
									bounds.bottom));
								BRect mainBounds = fParent->Bounds();
								// modify MainViewUpdateRight if more
								// columns are pushed by expanders
								if ((lastSwapColumn->fFlags & CLV_EXPANDER) != 0
										|| (lastSwapColumn->fPushedByExpander
									&& ((lastSwapColumn->fFlags & CLV_PUSH_PASS)
										!= 0))) {
									int32 columnCount
										= fDisplayList->CountItems();
									for (int32 j = fDisplayList->IndexOf(
											lastSwapColumn) + 1;
										j < columnCount; j++) {
										CLVColumn* column =
											(CLVColumn*)fDisplayList->ItemAt(j);
										if (column->IsShown()) {
											if (column->fPushedByExpander)
												MainViewUpdateRight
													= column->fColumnEnd;
											else
												break;
										}
									}
								}
								fParent->Invalidate(BRect(MainViewUpdateLeft,
									mainBounds.top, MainViewUpdateRight,
									mainBounds.bottom));
							}
						} while (hasColumnSnapped);
						// erase and redraw the drag rectangle but not the
						// interior to avoid label flicker
						float min = fPreviousDragOutlineLeft;
						float max = fPreviousDragOutlineRight;
						float min2 = mousePosition.x - fDragBoxMouseHoldOffset;
						float groupBegin = ((CLVDragGroup*)fDragGroupsList.ItemAt(
							fDragGroupIndex))->groupBeginIndex;
						if (min2 < groupBegin && fSnapGroupBeforeIndex == -1)
							min2 = groupBegin;

						if (min2 > groupBegin && fSnapGroupAfterIndex == -1)
							min2 = groupBegin;

						float max2 = min2 + fDragBoxWidth;
						float Temp;
						if (min2 < min || min == -1.0) {
							Temp = min2;
							min2 = min;
							min = Temp;
						}

						if (max2 > max || max == -1.0) {
							Temp = max2;
							max2 = max;
							max = Temp;
						}
						Invalidate(BRect(min, bounds.top + 1.0, min,
							bounds.bottom - 1.0));
						if (min2 != -1.0) {
							Invalidate(BRect(min2, bounds.top + 1.0, min2,
								bounds.bottom - 1.0));
						}
						Invalidate(BRect(max, bounds.top + 1.0, max,
							bounds.bottom - 1.0));
						if (max2 != -1.0) {
							Invalidate(BRect(max2, bounds.top + 1.0, max2,
								bounds.bottom - 1.0));
						}
						Invalidate(BRect(min, bounds.top, max, bounds.top));
						Invalidate(BRect(min, bounds.bottom, max,
							bounds.bottom));
					}
				}
			} else {
				// user is resizing the column
				if (mousePosition.x < fPreviousMousePosition.x
					|| mousePosition.x > fPreviousMousePosition.x) {
					float newWidth = mousePosition.x - fResizeMouseHoldOffset
						- fColumnClicked->fColumnBegin;
					if (newWidth < fColumnClicked->fMinWidth)
						newWidth = fColumnClicked->fMinWidth;

					if (newWidth != fColumnClicked->fWidth) {
						fColumnClicked->SetWidth(newWidth);
						fParent->ColumnWidthChanged(fParent->IndexOfColumn(
							fColumnClicked), newWidth);
					}
				}
			}
		} else if (buttons == 0) {
			// mouse button was released
			if (!fColumnDragging && !fColumnResizing) {
				// column was clicked
				if ((columnFlags & CLV_SORT_KEYABLE) != 0) {
					// column is a "sortable" column
					if ((modifiers & B_SHIFT_KEY) == 0) {
						// user wants to select it as the main sorting column
						if (fParent->fSortKeyList.ItemAt(0) == fColumnClicked) {
							// column was already selected; switch sort modes
							fParent->ReverseSortMode(
								fParent->fColumnList.IndexOf(fColumnClicked));
						} else {
							//The user selected this column for sorting
							fParent->SetSortKey(
								fParent->fColumnList.IndexOf(fColumnClicked));
						}
					} else {
						// user wants to add it as a secondary sorting column
						if (fParent->fSortKeyList.HasItem(fColumnClicked)) {
							// column was already selected; switch sort modes
							fParent->ReverseSortMode(
								fParent->fColumnList.IndexOf(fColumnClicked));
						} else {
							// user selected this column for sorting
							fParent->AddSortKey(
								fParent->fColumnList.IndexOf(fColumnClicked));
						}
					}
				}
			} else if (fColumnDragging) {
				// column was dragging; erase the drag box but not
				// the interior to avoid label flicker
				Invalidate(BRect(fPreviousDragOutlineLeft, bounds.top + 1.0,
					fPreviousDragOutlineLeft, bounds.bottom - 1.0));
				Invalidate(BRect(fPreviousDragOutlineRight, bounds.top + 1.0,
					fPreviousDragOutlineRight, bounds.bottom - 1.0));
				Invalidate(BRect(fPreviousDragOutlineLeft, bounds.top,
					fPreviousDragOutlineRight, bounds.top));
				Invalidate(BRect(fPreviousDragOutlineLeft, bounds.bottom,
					fPreviousDragOutlineRight, bounds.bottom));
			} else {
				// column was resizing; erase the drag tab
				Invalidate(BRect(fColumnClicked->fColumnEnd,
					bounds.top, fColumnClicked->fColumnEnd,
					bounds.bottom));
			}

			// unhighlight the label and forget the column
			Invalidate(BRect(fColumnClicked->fColumnBegin + 1.0,
				bounds.top + 1.0, fColumnClicked->fColumnEnd - 1.0,
				bounds.bottom - 1.0));
			fColumnClicked = NULL;
			fColumnDragging = false;
			fColumnResizing = false;
		} else {
			// unused button combination, unhighlight the label and
			// forget the column
			Invalidate(BRect(fColumnClicked->fColumnBegin + 1.0,
				bounds.top + 1.0, fColumnClicked->fColumnEnd - 1.0,
				bounds.bottom - 1.0));
			fColumnClicked = NULL;
			fColumnDragging = false;
			fColumnResizing = false;
		}

		fPreviousMousePosition = mousePosition;
	}
}


// Shift the drag group into a new position
void
CLVColumnLabelView::ShiftDragGroup(int32 newPosition)
{
	int32 groupCount = fDragGroupsList.CountItems();
	CLVDragGroup* group;
	BList newDisplayList;

	//Copy the groups up to the new position
	for (int32 i = 0; i < newPosition; i++) {
		if (i != fDragGroupIndex) {
			group = (CLVDragGroup*)fDragGroupsList.ItemAt(i);
			for (int32 j = group->groupStartDisplayListIndex;
					j <= group->groupStopDisplayListIndex; j++) {
				newDisplayList.AddItem(fDisplayList->ItemAt(j));
			}
		}
	}

	// copy the group into the new position
	group = (CLVDragGroup*)fDragGroupsList.ItemAt(fDragGroupIndex);
	for (int32 j = group->groupStartDisplayListIndex;
			j <= group->groupStopDisplayListIndex; j++) {
		newDisplayList.AddItem(fDisplayList->ItemAt(j));
	}

	// copy the rest of the groups, but skip the dragging group
	for (int32 i = newPosition; i < groupCount; i++) {
		if (i != fDragGroupIndex) {
			group = (CLVDragGroup*)fDragGroupsList.ItemAt(i);
			for (int32 j = group->groupStartDisplayListIndex;
					j <= group->groupStopDisplayListIndex; j++) {
				newDisplayList.AddItem(fDisplayList->ItemAt(j));
			}
		}
	}

	// set the new order
	*fDisplayList = newDisplayList;

	// update columns and drag groups
	fParent->ShiftDragGroup();
	UpdateDragGroups();
	SetSnapMinMax();

	// inform the program that the display order changed
	int32* newOrder = fParent->DisplayOrder();
	fParent->DisplayOrderChanged(newOrder);
	delete[] newOrder;
}


// Make a copy of the DragGroups list, use it to store
// the CLVDragGroup's for recycling.
void
CLVColumnLabelView::UpdateDragGroups()
{
	BList tempList(fDragGroupsList);
	fDragGroupsList.MakeEmpty();
	int32 columnCount = fDisplayList->CountItems();
	bool shouldContinueGroup = false;
	CLVDragGroup* CurrentGroup = NULL;

	for (int32 i = 0; i < columnCount; i++) {
		CLVColumn* CurrentColumn = (CLVColumn*)fDisplayList->ItemAt(i);
		if (!shouldContinueGroup) {
			// recycle or obtain a new CLVDragGroup
			CurrentGroup = (CLVDragGroup*)tempList.RemoveItem(int32(0));
			if (CurrentGroup == NULL)
				CurrentGroup = new CLVDragGroup;

			// add the CLVDragGroup to the DragGroups list
			fDragGroupsList.AddItem(CurrentGroup);
			// set up the new DragGroup
			CurrentGroup->groupStartDisplayListIndex = i;
			CurrentGroup->groupStopDisplayListIndex = i;
			CurrentGroup->flags = 0;

			if (CurrentColumn->IsShown()) {
				CurrentGroup->groupBeginIndex = CurrentColumn->fColumnBegin;
				CurrentGroup->groupEndIndex = CurrentColumn->fColumnEnd;
				CurrentGroup->lastColumnShown = CurrentColumn;
				CurrentGroup->isShown = true;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING)
					CurrentGroup->isAllLockBeginning = true;
				else
					CurrentGroup->isAllLockBeginning = false;

				if (CurrentColumn->fFlags & CLV_LOCK_AT_END)
					CurrentGroup->isAllLockEnd = true;
				else
					CurrentGroup->isAllLockEnd = false;
			} else {
				CurrentGroup->groupBeginIndex = -1.0;
				CurrentGroup->groupEndIndex = -1.0;
				CurrentGroup->lastColumnShown = NULL;
				CurrentGroup->isShown = false;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING)
					CurrentGroup->isAllLockBeginning = true;
				else
					CurrentGroup->isAllLockBeginning = false;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_END)
					CurrentGroup->isAllLockEnd = true;
				else
					CurrentGroup->isAllLockEnd = false;
			}
		} else {
			// add this column to the current DragGroup
			CurrentGroup->groupStopDisplayListIndex = i;
			if (CurrentColumn->IsShown()) {
				if (CurrentGroup->groupBeginIndex == -1.0)
					CurrentGroup->groupBeginIndex = CurrentColumn->fColumnBegin;
				CurrentGroup->groupEndIndex = CurrentColumn->fColumnEnd;
				CurrentGroup->lastColumnShown = CurrentColumn;
				CurrentGroup->isShown = true;
			}
			if ((CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING) == 0)
				CurrentGroup->isAllLockBeginning = false;
			if ((CurrentColumn->fFlags & CLV_LOCK_AT_END) == 0)
				CurrentGroup->isAllLockEnd = false;
		}

		CurrentGroup->flags = CurrentColumn->fFlags
			& (CLV_NOT_MOVABLE | CLV_LOCK_AT_BEGINNING
				| CLV_LOCK_AT_END);
		// see if I should add more columns to this group
		if (CurrentColumn->fFlags & CLV_LOCK_WITH_RIGHT)
			shouldContinueGroup = true;
		else
			shouldContinueGroup = false;
	}
	// if any unused groups remain in tempList, delete them
	while ((CurrentGroup = (CLVDragGroup*)tempList.RemoveItem(int32(0)))
			!= NULL) {
		delete CurrentGroup;
	}
}


// find the column group that the user is dragging and the shown
// group before it
void
CLVColumnLabelView::SetSnapMinMax()
{
	int32 groupCount = fDragGroupsList.CountItems();
	int32 columnCount;
	fDragGroupIndex = -1;
	fShownGroupBefore = NULL;
	fSnapGroupBeforeIndex = -1;
	CLVDragGroup* group;
	for (int32 i = 0; i < groupCount; i++) {
		group = (CLVDragGroup*)fDragGroupsList.ItemAt(i);
		for (columnCount = group->groupStartDisplayListIndex;
			columnCount <= group->groupStopDisplayListIndex; columnCount++) {
			if (fDisplayList->ItemAt(columnCount) == fColumnClicked) {
				fDragGroupIndex = i;
				fDragGroup = group;
				break;
			}
		}

		if (fDragGroupIndex != -1)
			break;
		else if (group->isShown) {
			fShownGroupBefore = group;
			fSnapGroupBeforeIndex = i;
		}
	}

	// find the position of shown group after the one that the user is dragging
	fShownGroupAfter = NULL;
	fSnapGroupAfterIndex = -1;
	for (int32 i = fDragGroupIndex + 1; i < groupCount; i++) {
		group = (CLVDragGroup*)fDragGroupsList.ItemAt(i);
		if (group->isShown) {
			fShownGroupAfter = group;
			fSnapGroupAfterIndex = i;
			break;
		}
	}

	// see if it can actually snap in the given direction
	if (fSnapGroupBeforeIndex != -1) {
		if ((fShownGroupBefore->flags & CLV_LOCK_AT_BEGINNING) != 0
			&& !fDragGroup->isAllLockBeginning) {
			fSnapGroupBeforeIndex = -1;
		}
		if ((fDragGroup->flags & CLV_LOCK_AT_END) != 0
			&& !fShownGroupBefore->isAllLockEnd) {
			fSnapGroupBeforeIndex = -1;
		}
	}
	if (fSnapGroupAfterIndex != -1) {
		if (fShownGroupAfter->flags & CLV_LOCK_AT_END
			&& !fDragGroup->isAllLockEnd) {
				fSnapGroupAfterIndex = -1;
		}
		if (fDragGroup->flags & CLV_LOCK_AT_BEGINNING
			&& !fShownGroupAfter->isAllLockBeginning) {
				fSnapGroupAfterIndex = -1;
		}
	}

	// find the minumum and maximum positions for the group to snap
	fSnapMin = -1.0;
	fSnapMax = -1.0;
	fDragBoxWidth = fDragGroup->groupEndIndex - fDragGroup->groupBeginIndex;
	if (fSnapGroupBeforeIndex != -1) {
		fSnapMin = fShownGroupBefore->groupBeginIndex + fDragBoxWidth;
		if (fSnapMin > fShownGroupBefore->groupEndIndex)
			fSnapMin = fShownGroupBefore->groupEndIndex;
	}
	if (fSnapGroupAfterIndex != -1){
		fSnapMax = fShownGroupAfter->groupEndIndex - fDragBoxWidth;
		if (fSnapMax < fShownGroupAfter->groupBeginIndex)
			fSnapMax = fShownGroupAfter->groupBeginIndex;
	}
}
