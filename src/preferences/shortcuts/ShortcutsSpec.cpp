/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "ShortcutsSpec.h"


#include <ctype.h>
#include <stdio.h>

#include <Region.h>
#include <Window.h>
#include <Directory.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Beep.h>


#include "ColumnListView.h"


#include "Colors.h"
#include "MetaKeyStateMap.h"
#include "BitFieldTesters.h"
#include "CommandActuators.h"
#include "ParseCommandLine.h"

#define CLASS "ShortcutsSpec : "

const float _height = 20.0f;		

static MetaKeyStateMap _metaMaps[ShortcutsSpec::NUM_META_COLUMNS];
static bool _metaMapsInitialized = false;

static bool _fontCached = false;
static BFont _viewFont;
static float _fontHeight;
static BBitmap * _actuatorBitmaps[2];

// These meta-keys are pretty standard
#define SHIFT_NAME		"Shift"
#define CONTROL_NAME	"Control"

// These meta-keys have different names on an Intel keyboard
#if __INTEL__
	#define OPTION_NAME		"Window"
	#define COMMAND_NAME	"Alt"
#else
	#define OPTION_NAME		"Option"
	#define COMMAND_NAME	"Command"
#endif

#define ICON_BITMAP_RECT BRect(0.0f, 0.0f, 15.0f, 15.0f)
#define ICON_BITMAP_SPACE B_COLOR_8_BIT

// Returns the (pos)'th char in the string, or '\0' if (pos) if off the end of
// the string
static char GetLetterAt(const char* str, int pos);

static char 
GetLetterAt(const char* str, int pos)
{
	for (int i = 0; i < pos; i++) 
		if (str[i] == '\0') 
			return '\0';
	return str[pos];
}


// Setup the states in a standard manner for a pair of meta-keys.
static void 
SetupStandardMap(MetaKeyStateMap& map, const char* name, uint32 both, 
	uint32 left, uint32 right)
{
	map.SetInfo(name);
	
	// In this state, neither key may be pressed.
	map.AddState("(None)", new HasBitsFieldTester(0, both));
	
	// Here, either may be pressed. (Remember both is NOT a 2-bit chord, it's 
	// another bit entirely)
	map.AddState("Either", new HasBitsFieldTester(both));
	
	// Here, only the left may be pressed
	map.AddState("Left", new HasBitsFieldTester(left, right));
	
	// Here, only the right may be pressed
	map.AddState("Right", new HasBitsFieldTester(right, left));
	
	// Here, both must be pressed.
	map.AddState("Both", new HasBitsFieldTester(left | right));
}


MetaKeyStateMap & GetNthKeyMap(int which)
{
	return _metaMaps[which];
}


static BBitmap* MakeActuatorBitmap(bool lit);
static BBitmap* 
MakeActuatorBitmap(bool lit)
{
	BBitmap* map = new BBitmap(ICON_BITMAP_RECT, ICON_BITMAP_SPACE, true);
	const rgb_color yellow = {255, 255, 0};
	const rgb_color red = {200, 200, 200};
	const rgb_color black = {0, 0, 0};
	const BPoint points[10] = {
		BPoint(8, 0), BPoint(9.8, 5.8), BPoint(16, 5.8),
		BPoint(11, 9.0), BPoint(13, 16), BPoint(8, 11),
		BPoint(3, 16), BPoint(5, 9.0), BPoint(0, 5.8),
		BPoint(6.2, 5.8) };

	BView* view = new BView(BRect(0, 0, 16, 16), NULL, B_FOLLOW_ALL_SIDES, 0L);
	map->AddChild(view); 
	map->Lock();
	view->SetHighColor(B_TRANSPARENT_32_BIT);
	view->FillRect(ICON_BITMAP_RECT);
	view->SetHighColor(lit ? yellow : red);
	view->FillPolygon(points, 10);
	view->SetHighColor(black);
	view->StrokePolygon(points, 10);
	map->Unlock(); 
	map->RemoveChild(view);
	delete view;
	return map;
}


void InitializeMetaMaps()
{
	_metaMapsInitialized = true;
	SetupStandardMap(_metaMaps[ShortcutsSpec::SHIFT_COLUMN_INDEX], SHIFT_NAME, 
		B_SHIFT_KEY, B_LEFT_SHIFT_KEY, B_RIGHT_SHIFT_KEY);
		
	SetupStandardMap(_metaMaps[ShortcutsSpec::CONTROL_COLUMN_INDEX], 
		CONTROL_NAME, B_CONTROL_KEY, B_LEFT_CONTROL_KEY, B_RIGHT_CONTROL_KEY);
		
	SetupStandardMap(_metaMaps[ShortcutsSpec::COMMAND_COLUMN_INDEX], 
		COMMAND_NAME, B_COMMAND_KEY, B_LEFT_COMMAND_KEY, B_RIGHT_COMMAND_KEY);
		
	SetupStandardMap(_metaMaps[ShortcutsSpec::OPTION_COLUMN_INDEX], OPTION_NAME
		, B_OPTION_KEY, B_LEFT_OPTION_KEY, B_RIGHT_OPTION_KEY);

	_actuatorBitmaps[0] = MakeActuatorBitmap(false);
	_actuatorBitmaps[1] = MakeActuatorBitmap(true);
}


ShortcutsSpec::ShortcutsSpec(const char* cmd) 
	:
	CLVListItem(0, false, false, _height), 
	fCommand(NULL), 
	fTextOffset(0), 
	fBitmap(ICON_BITMAP_RECT, ICON_BITMAP_SPACE), 
	fLastBitmapName(NULL), 
	fBitmapValid(false),
	fKey(0),
	fCursorPtsValid(false)
{
	for (int i = 0; i < NUM_META_COLUMNS; i++) 
		fMetaCellStateIndex[i] = 0;
	SetCommand(cmd);
}


ShortcutsSpec::ShortcutsSpec(const ShortcutsSpec& from)
	:
	CLVListItem(0, false, false, _height), 
	fCommand(NULL),
	fTextOffset(from.fTextOffset), 
	fBitmap(ICON_BITMAP_RECT, ICON_BITMAP_SPACE), 
	fLastBitmapName(NULL), 
	fBitmapValid(false),
	fKey(from.fKey),
	fCursorPtsValid(false)
{
	for (int i = 0; i < NUM_META_COLUMNS; i++) 
		fMetaCellStateIndex[i] = from.fMetaCellStateIndex[i];
	
	SetCommand(from.fCommand);
	SetSelectedColumn(from.GetSelectedColumn());
}


ShortcutsSpec::ShortcutsSpec(BMessage* from)
	:
	CLVListItem(0, false, false, _height), 
	fCommand(NULL), 
	fTextOffset(0), 
	fBitmap(ICON_BITMAP_RECT, ICON_BITMAP_SPACE), 
	fLastBitmapName(NULL), 
	fBitmapValid(false),
	fCursorPtsValid(false)
{
	const char* temp;	
	if (from->FindString("command", &temp) != B_NO_ERROR) {
		printf(CLASS " Error, no command string in archive BMessage!\n");
		temp = "";
	}

	SetCommand(temp);

	if (from->FindInt32("key", (int32*) &fKey) != B_NO_ERROR)
		printf(CLASS " Error, no key int32 in archive BMessage!\n");

	for (int i = 0; i < NUM_META_COLUMNS; i++)
		if (from->FindInt32("mcidx", i, (int32*)&fMetaCellStateIndex[i]) 
			!= B_NO_ERROR)
			printf(CLASS " Error, no modifiers int32 in archive BMessage!\n");
}


void
ShortcutsSpec::SetCommand(const char* command)
{
	delete[] fCommand;	// out with the old (if any)...
	fCommandLen = strlen(command) + 1;
	fCommandNul = fCommandLen - 1;
	fCommand = new char[fCommandLen];
	strcpy(fCommand, command);
	_UpdateIconBitmap();
}


const char*
ShortcutsSpec::GetColumnName(int i)
{
	return _metaMaps[i].GetName();
}


status_t
ShortcutsSpec::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	if (ret != B_NO_ERROR)
		return ret;

	into->AddString("class", "ShortcutsSpec");

	// These fields are for our prefs panel's benefit only
	into->AddString("command", fCommand);
	into->AddInt32("key", fKey);

	// Assemble a BitFieldTester for the input_server add-on to use...
	MinMatchFieldTester test(NUM_META_COLUMNS, false);
	for (int i = 0; i < NUM_META_COLUMNS; i++) {
		// for easy parsing by prefs applet on load-in
		into->AddInt32("mcidx", fMetaCellStateIndex[i]);
		test.AddSlave(_metaMaps[i].GetNthStateTester(fMetaCellStateIndex[i]));
	}

	BMessage testerMsg;
	ret = test.Archive(&testerMsg);
	if (ret != B_NO_ERROR)
		return ret;

	into->AddMessage("modtester", &testerMsg);

	// And also create a CommandActuator for the input_server add-on to execute
	CommandActuator* act = CreateCommandActuator(fCommand);
	BMessage actMsg;
	ret = act->Archive(&actMsg);
	if (ret != B_NO_ERROR)
		return ret;
	delete act;

	into->AddMessage("act", &actMsg);
	return ret;
}


static bool IsValidActuatorName(const char* c);
static bool
IsValidActuatorName(const char* c)
{
	return ((strcmp(c, "InsertString")	== 0) ||
			(strcmp(c, "MoveMouse")		== 0) ||
			(strcmp(c, "MoveMouseTo")	== 0) ||
			(strcmp(c, "MouseButton")	== 0) ||
			(strcmp(c, "LaunchHandler")	== 0) ||	// new for v1.21 --jaf
			(strcmp(c, "Multi")			== 0) ||	// new for v1.24 --jaf
			(strcmp(c, "MouseDown")		== 0) ||	// new for v1.24 --jaf
			(strcmp(c, "MouseUp")		== 0) ||	// new for v1.24 --jaf
			(strcmp(c, "SendMessage")	== 0) ||	// new for v1.25 --jaf
			(strcmp(c, "Beep")			== 0));
}


BArchivable* 
ShortcutsSpec::Instantiate(BMessage* from)
{
	bool validateOK = false;
	if (validate_instantiation(from, "ShortcutsSpec")) 
		validateOK = true; 
	else //test the old one.
		if (validate_instantiation(from, "SpicyKeysSpec")) 
			validateOK = true;

	if (!validateOK)
		return NULL;

	return new ShortcutsSpec(from);
}


ShortcutsSpec::~ShortcutsSpec()
{
	delete[] fCommand;
	delete[] fLastBitmapName;
}


void
ShortcutsSpec::_CacheViewFont(BView* owner)
{
	if (_fontCached == false) {
		_fontCached = true;
		owner->GetFont(&_viewFont);
		font_height fh;
		_viewFont.GetHeight(&fh);
		_fontHeight = fh.ascent - fh.descent;
	}
}


void
ShortcutsSpec::DrawItemColumn(BView* owner, BRect item_column_rect, 
	int32 column_index, bool columnSelected, bool complete)
{
	const float STRING_COLUMN_LEFT_MARGIN = 25.0f; // 16 for the icon,+9 empty

	rgb_color color;
	bool selected = IsSelected();
	if (selected)
		color = columnSelected ? BeBackgroundGrey : BeListSelectGrey;
	else
		color = BeInactiveControlGrey;
	owner->SetLowColor(color);
	owner->SetDrawingMode(B_OP_COPY);
	owner->SetHighColor(color);
	owner->FillRect(item_column_rect);

	const char* text = GetCellText(column_index);

	if (text == NULL)
		return;
	
	float textWidth = _viewFont.StringWidth(text);
	BPoint point;
	rgb_color lowColor = color;

	if (column_index == STRING_COLUMN_INDEX) {
		// left justified
		point.Set(item_column_rect.left + STRING_COLUMN_LEFT_MARGIN, 
			item_column_rect.top + fTextOffset);
		
		item_column_rect.left = point.x;//keep text from drawing into icon area

		// scroll if too wide
		float rectWidth = item_column_rect.Width() - STRING_COLUMN_LEFT_MARGIN;
		float extra = textWidth - rectWidth;
		if (extra > 0.0f)
			point.x -= extra;
	} else {
		if ((column_index < NUM_META_COLUMNS) && (text[0] == '('))
			return; // don't draw for this ...
		
		if ((column_index <= NUM_META_COLUMNS) && (text[0] == '\0'))
			return; // don't draw for this ...

		// centered
		point.Set((item_column_rect.left + item_column_rect.right) / 2.0, 
			item_column_rect.top + fTextOffset);
		_CacheViewFont(owner);
		point.x -= textWidth / 2.0f;
	}

	BRegion Region;
	Region.Include(item_column_rect);
	owner->ConstrainClippingRegion(&Region);
	if (column_index != STRING_COLUMN_INDEX) {
		const float KEY_MARGIN = 3.0f;
		const float CORNER_RADIUS = 3.0f;
		_CacheViewFont(owner);

		// How about I draw a nice "key" background for this one?
		BRect textRect(point.x - KEY_MARGIN, (point.y-_fontHeight) - KEY_MARGIN
			, point.x + textWidth + KEY_MARGIN - 2.0f, point.y + KEY_MARGIN);
		
		if (column_index == KEY_COLUMN_INDEX)
			lowColor = ReallyLightPurple;
		else
			lowColor = LightYellow;
		
		owner->SetHighColor(lowColor);
		owner->FillRoundRect(textRect, CORNER_RADIUS, CORNER_RADIUS);
		owner->SetHighColor(Black);
		owner->StrokeRoundRect(textRect, CORNER_RADIUS, CORNER_RADIUS);
	}

	owner->SetHighColor(Black);
	owner->SetLowColor(lowColor);
	owner->DrawString(text, point);
	// with a cursor at the end if highlighted
	if (column_index == STRING_COLUMN_INDEX) {
		// Draw cursor
		if ((columnSelected) && (selected)) {
			point.x += textWidth;
			point.y += (fTextOffset / 4.0f);

			BPoint pt2 = point;
			pt2.y -= fTextOffset;
			owner->StrokeLine(point, pt2);

			fCursorPt1 = point;
			fCursorPt2 = pt2;
			fCursorPtsValid = true;
		}

		BRegion bitmapRegion;
		item_column_rect.left	-= (STRING_COLUMN_LEFT_MARGIN - 4.0f);
		item_column_rect.right	= item_column_rect.left + 16.0f;
		item_column_rect.top	+= 3.0f;
		item_column_rect.bottom	= item_column_rect.top + 16.0f;

		bitmapRegion.Include(item_column_rect);
		owner->ConstrainClippingRegion(&bitmapRegion);
		owner->SetDrawingMode(B_OP_OVER);

		if ((fCommand != NULL) && (fCommand[0] == '*'))
			owner->DrawBitmap(_actuatorBitmaps[fBitmapValid ? 1 : 0], 
				ICON_BITMAP_RECT, item_column_rect);
		else
			// Draw icon, if any
			if (fBitmapValid) 
				owner->DrawBitmap(&fBitmap, ICON_BITMAP_RECT, 
					item_column_rect);
	}

	owner->SetDrawingMode(B_OP_COPY);
	owner->ConstrainClippingRegion(NULL);
}


void
ShortcutsSpec::Update(BView* owner, const BFont* font)
{
	CLVListItem::Update(owner, font);
	font_height FontAttributes;
	be_plain_font->GetHeight(&FontAttributes);
	float fontHeight = ceil(FontAttributes.ascent) + 
		ceil(FontAttributes.descent);
	fTextOffset = ceil(FontAttributes.ascent) + (Height() - fontHeight) / 2.0;
}


const char*
ShortcutsSpec::GetCellText(int whichColumn) const
{
	const char* temp = ""; // default
	switch(whichColumn)
	{
		case KEY_COLUMN_INDEX:
		{
			if ((fKey > 0) && (fKey <= 0xFF)) {
				temp = GetKeyName(fKey);
				if (temp == NULL)
					temp = "";
			} else if (fKey > 0xFF) {
				sprintf(fScratch, "#%x", fKey);
				return fScratch;
			}
		}
		break;

		case STRING_COLUMN_INDEX:
			temp = fCommand;
		break;

		default:
			if ((whichColumn >= 0) && (whichColumn < NUM_META_COLUMNS)) 
				temp = _metaMaps[whichColumn].GetNthStateDesc(
							fMetaCellStateIndex[whichColumn]);
		break;
	}
	return temp;
}


bool
ShortcutsSpec::ProcessColumnMouseClick(int whichColumn)
{
	if ((whichColumn >= 0) && (whichColumn < NUM_META_COLUMNS)) {
		// same as hitting space for these columns: cycle entry
		const char temp = B_SPACE;

		// 3rd arg isn't correct but it isn't read for this case anyway
		return ProcessColumnKeyStroke(whichColumn, &temp, 0);
	}
	return false;
}


bool
ShortcutsSpec::ProcessColumnTextString(int whichColumn, const char* string)
{
	switch(whichColumn) {
		case STRING_COLUMN_INDEX:
			SetCommand(string);
			return true;
		break;

		case KEY_COLUMN_INDEX:
		{
			fKey = FindKeyCode(string);
			return true;
			break;
		}		

		default:
			return ProcessColumnKeyStroke(whichColumn, string, 0);
	}
}


bool
ShortcutsSpec::_AttemptTabCompletion()
{
	bool ret = false;

	int32 argc;
	char** argv = ParseArgvFromString(fCommand, argc);
	if (argc > 0) {
		// Try to complete the path partially expressed in the last argument!
		char* arg = argv[argc - 1];
		char* fileFragment = strrchr(arg, '/');
		if (fileFragment) {
			const char* directoryName = (fileFragment == arg) ? "/" : arg;
			*fileFragment = '\0';
			fileFragment++;
			int fragLen = strlen(fileFragment);

			BDirectory dir(directoryName);
			if (dir.InitCheck() == B_NO_ERROR) {
				BEntry nextEnt;
				BPath nextPath;
				BList matchList;
				int maxEntryLen = 0;

				// Read in all the files in the directory whose names start 
				// with our fragment.
				while (dir.GetNextEntry(&nextEnt) == B_NO_ERROR) {
					if (nextEnt.GetPath(&nextPath) == B_NO_ERROR) {
						char* filePath = strrchr(nextPath.Path(), '/') + 1;
						if (strncmp(filePath, fileFragment, fragLen) == 0) {
							int len = strlen(filePath);
							if (len > maxEntryLen)
								maxEntryLen = len;
							char* newStr = new char[len + 1];
							strcpy(newStr, filePath);
							matchList.AddItem(newStr);
						}
					}
				}

				// Now slowly extend our keyword to its full length, counting
				// numbers of matches at each step. If the match list length 
				// is 1, we can use that whole entry. If it's greater than one
				// , we can use just the match length.
				int matchLen = matchList.CountItems();
				if (matchLen > 0) {
					int i;
					BString result(fileFragment);
					for (i = fragLen; i < maxEntryLen; i++) {
						// See if all the matching entries have the same letter
						// in the next position... if so, we can go farther.
						char commonLetter = '\0';
						for (int j = 0; j < matchLen; j++) {
							char nextLetter = GetLetterAt(
								(char*)matchList.ItemAt(j), i);
							if (commonLetter == '\0')
								commonLetter = nextLetter;
						
							if ((commonLetter != '\0') 
								&& (commonLetter != nextLetter)) {
								commonLetter = '\0';// failed;
								beep();
								break;
							}
						}
						if (commonLetter == '\0')
							break;
						else
							result.Append(commonLetter, 1);
					}

					// Free all the strings we allocated
					for (int k = 0; k < matchLen; k++)
						delete [] ((char*)matchList.ItemAt(k));

					DoStandardEscapes(result);

					BString wholeLine;
					for (int l = 0; l < argc - 1; l++) {
						wholeLine += argv[l];
						wholeLine += " ";
					}
				
					BString file(directoryName);
					DoStandardEscapes(file);

					if (directoryName[strlen(directoryName) - 1] != '/')
						file += "/";

					file += result;

					// Remove any trailing slash...
					const char* fileStr = file.String();
					if (fileStr[strlen(fileStr)-1] == '/')
						file.RemoveLast("/");

					// And re-append it iff the file is a dir.
					BDirectory testFileAsDir(file.String());
					if ((strcmp(file.String(), "/") != 0) 
						&& (testFileAsDir.InitCheck() == B_NO_ERROR))
						file.Append("/");

					wholeLine += file;

					SetCommand(wholeLine.String());
					ret = true;
				}
			}
			*(fileFragment - 1) = '/';
		}
	}
	FreeArgv(argv);
	return ret;
}


bool
ShortcutsSpec::ProcessColumnKeyStroke(int whichColumn, const char* bytes, 
	int32 key)
{
	bool ret = false;
	switch(whichColumn) {
		case KEY_COLUMN_INDEX:
			if (key != -1) {
				if (fKey != key) {
					fKey = key;
					ret = true;
				}
			}
		break;

		case STRING_COLUMN_INDEX:
		{
			switch(bytes[0]) {
				case B_BACKSPACE: 
				case B_DELETE:
					if (fCommandNul > 0) {
						//trim a char off the string
						fCommand[fCommandNul - 1] = '\0';
						fCommandNul--;	// note new nul position
						ret = true;
						_UpdateIconBitmap();
					}
				break;

				case B_TAB:
					if (_AttemptTabCompletion()) {
						_UpdateIconBitmap();
						ret = true;
					} else
						beep();
				break;

				default:
				{
					int newCharLen = strlen(bytes);
					if ((newCharLen > 0) && (bytes[0] >= ' ')) {
						bool reAllocString = false;
						// Make sure we have enough room in our command string 
						// to add these chars...
						while (fCommandLen - fCommandNul <= newCharLen) {
							reAllocString = true;
							// enough for a while...
							fCommandLen = (fCommandLen + 10) * 2;
						}
						
						if (reAllocString) {
							char* temp = new char[fCommandLen];
							strcpy(temp, fCommand);
							delete [] fCommand;
							fCommand = temp;
							// fCommandNul is still valid since it's an offset 
							// and the string length is the same for now
						}

						// Here we should be guaranteed enough room.
						strncat(fCommand, bytes, fCommandLen);
						fCommandNul += newCharLen;
						ret = true;
						_UpdateIconBitmap();
					}
				}
			}
		}
		break;

		default:
			if ((whichColumn >= 0) && (whichColumn < NUM_META_COLUMNS)) {
				MetaKeyStateMap * map = &_metaMaps[whichColumn];
				int curState = fMetaCellStateIndex[whichColumn];
				int origState = curState;
				int numStates = map->GetNumStates();

				switch(bytes[0])
				{
					case B_RETURN: 
						// cycle to the previous state
						curState = (curState + numStates - 1) % numStates;
					break;

					case B_SPACE:
						// cycle to the next state
						curState = (curState + 1) % numStates;
					break;

					default:
					{
						// Go to the state starting with the given letter, if 
						// any
						char letter = bytes[0];
						if (islower(letter))
							letter = toupper(letter); // convert to upper case
						
						if ((letter == B_BACKSPACE) || (letter == B_DELETE))
							letter = '(';//so space bar will blank out an entry
						
						for (int i = 0; i < numStates; i++) {
							const char* desc = map->GetNthStateDesc(i);

							if (desc) {
								if (desc[0] == letter) {
									curState = i;
									break;
								}
							} else 
								printf("Error, NULL state description?\n");
						}
					}
					break;
				}
				fMetaCellStateIndex[whichColumn] = curState;
				
				if (curState != origState)
					ret = true;
			}
		break;
	}

	return ret;
}


int
ShortcutsSpec::MyCompare(const CLVListItem* a_Item1, const CLVListItem* a_Item2
	, int32 KeyColumn)
{
	ShortcutsSpec* left = (ShortcutsSpec*) a_Item1;
	ShortcutsSpec* right = (ShortcutsSpec*) a_Item2;

	int ret = strcmp(left->GetCellText(KeyColumn), 
		right->GetCellText(KeyColumn));
	return (ret > 0) ? 1 : ((ret == 0) ? 0 : -1);
}


void
ShortcutsSpec::Pulse(BView* owner)
{
	if ((fCursorPtsValid)&&(owner->Window()->IsActive())) {
		rgb_color prevColor = owner->HighColor();
		rgb_color backgroundColor = (GetSelectedColumn() == 
			STRING_COLUMN_INDEX) ? BeBackgroundGrey : BeListSelectGrey;
		rgb_color barColor = ((GetSelectedColumn() == STRING_COLUMN_INDEX) 
			&& ((system_time() % 1000000) > 500000)) ? Black : backgroundColor;
		owner->SetHighColor(barColor);
		owner->StrokeLine(fCursorPt1, fCursorPt2);
		owner->SetHighColor(prevColor);
	}
}


void
ShortcutsSpec::_UpdateIconBitmap()
{
	BString firstWord = ParseArgvZeroFromString(fCommand);

	// Only need to change if the first word has changed...
	if ((fLastBitmapName == NULL) || (firstWord.Length() == 0) || 
		(firstWord.Compare(fLastBitmapName))) {
		if (firstWord.ByteAt(0) == '*')
			fBitmapValid = IsValidActuatorName(&firstWord.String()[1]);
		else {
			fBitmapValid = false; // default till we prove otherwise!
 
			if (firstWord.Length() > 0) {
				delete [] fLastBitmapName;
				fLastBitmapName = new char[firstWord.Length() + 1];
				strcpy(fLastBitmapName, firstWord.String());

				BEntry progEntry(fLastBitmapName, true);
				if ((progEntry.InitCheck() == B_NO_ERROR) 
					&& (progEntry.Exists())) {
					BNode progNode(&progEntry);
					if (progNode.InitCheck() == B_NO_ERROR) {
						BNodeInfo progNodeInfo(&progNode);
						if ((progNodeInfo.InitCheck() == B_NO_ERROR) 
						&& (progNodeInfo.GetTrackerIcon(&fBitmap, B_MINI_ICON) 
						== B_NO_ERROR))
							fBitmapValid = fBitmap.IsValid();
					}
				}
			}
		}
	}
}
