/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef REGISTERS_VIEW_H
#define REGISTERS_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class Architecture;
class BMenu;


class RegistersView : public BGroupView, private TableListener {
public:
								RegistersView(Architecture* architecture);
								~RegistersView();

	static	RegistersView*		Create(Architecture* architecture);
									// throws

	virtual	void				MessageReceived(BMessage* message);

			void				SetCpuState(CpuState* cpuState);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class RegisterValueColumn;
			class RegisterTableModel;

private:
	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

	virtual	void				TableCellMouseDown(Table* table, int32 rowIndex,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);

			void				_Init();
			status_t			_AddFormatItem(BMenu* menu, int32 format);

private:
			Architecture*		fArchitecture;
			CpuState*			fCpuState;
			Table*				fRegisterTable;
			RegisterTableModel*	fRegisterTableModel;
};


#endif	// REGISTERS_VIEW_H
