/*
 * Copyright 2007-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_FIELD_H
#define SUDOKU_FIELD_H


#include <Archivable.h>
#include <SupportDefs.h>


enum {
	kInitialValue	= 0x01,
};


class SudokuField : public BArchivable {
public:
								SudokuField(uint32 size);
								SudokuField(const BMessage* archive);
								SudokuField(const SudokuField& other);
	virtual						~SudokuField();

	status_t					InitCheck();

	virtual	status_t			Archive(BMessage* archive, bool deep) const;
	static	SudokuField*		Instantiate(BMessage* archive);

			status_t			SetTo(char base, const char* data);
			void				SetTo(const SudokuField* other);
			void				Reset();

			bool				IsSolved() const;
			bool				IsEmpty() const;
			bool				IsValueCompleted(uint32 value) const;

			uint32				Size() const { return fSize; }
			uint32				BlockSize() const { return fBlockSize; }

			void				SetHintMaskAt(uint32 x, uint32 y,
									uint32 hintMask);
			uint32				HintMaskAt(uint32 x, uint32 y) const;

			void				SetValidMaskAt(uint32 x, uint32 y,
									uint32 validMask);
			uint32				ValidMaskAt(uint32 x, uint32 y) const;

			void				SetFlagsAt(uint32 x, uint32 y, uint32 flags);
			uint32				FlagsAt(uint32 x, uint32 y) const;

			void				SetValueAt(uint32 x, uint32 y, uint32 value,
									bool setSolved = false);
			uint32				ValueAt(uint32 x, uint32 y) const;

			void				Dump();

private:
	struct field {
		field();

		uint32		hint_mask;
		uint32		valid_mask;
		uint32		flags;
		uint32		value;
	};

			bool				_ValidValueAt(uint32 x, uint32 y) const;
			void				_ComputeValidMask(uint32 x, uint32 y,
									bool setSolved);
			void				_UpdateValidMaskChanged(uint32 x, uint32 y,
									bool setSolved);
			const field&		_FieldAt(uint32 x, uint32 y) const;
			field&				_FieldAt(uint32 x, uint32 y);

private:
			uint32				fSize;
			uint32				fBlockSize;
			uint32				fMaxMask;
			field*				fFields;
};


#endif	// SUDOKU_FIELD_H
