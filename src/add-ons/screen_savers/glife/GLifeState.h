/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFESTATE_H
#define _GLIFE_GLIFESTATE_H


// Constants
const int32 kDefGridWidth = 60;
const int32 kDefGridHeight = 40;
const int32 kDefGridBorder = 0;
const int32 kDefGridDelay = 2;


// GLifeState Class Declaration & Definition
class GLifeState
{
private:
	int32	fGridWidth;
	int32	fGridHeight;
	int32	fGridBorder;
	int32	fGridDelay;
	
public:
	// Constructor
				GLifeState( void ) : fGridWidth(kDefGridWidth),
									 fGridHeight(kDefGridHeight),
									 fGridBorder(kDefGridBorder),
									 fGridDelay(kDefGridDelay) { };

	// Save/Restore State Methods
	status_t SaveState(BMessage* pbmPrefs) const {
		// Store current preferences
		pbmPrefs->AddInt32("gridWidth", fGridWidth);
		pbmPrefs->AddInt32("gridHeight", fGridHeight);
		pbmPrefs->AddInt32("gridBorder", fGridBorder);
		pbmPrefs->AddInt32("gridDelay", fGridDelay);
					
		return B_OK;
	};
	
	void RestoreState( BMessage* pbmPrefs )
	{
		// Retrieve preferences, substituting defaults
		if (pbmPrefs->FindInt32("gridWidth", &fGridWidth) != B_OK)
			fGridWidth = kDefGridWidth;
		if (pbmPrefs->FindInt32("gridHeight", &fGridHeight) != B_OK)
			fGridHeight = kDefGridHeight;
		if (pbmPrefs->FindInt32("gridBorder", &fGridBorder) != B_OK)
			fGridBorder = kDefGridBorder;
		if (pbmPrefs->FindInt32("gridDelay", &fGridDelay) != B_OK)
			fGridDelay = kDefGridDelay; 
	};
	
	// Accessor Methods
	int32&		GridWidth(void) { return fGridWidth; };
	int32&		GridHeight(void) { return fGridHeight; };
	int32&		GridBorder(void) { return fGridBorder; };
	int32&		GridDelay(void) { return fGridDelay; };
};


#endif /* _GLIFE_GLIFESTATE_H */
