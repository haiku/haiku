/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef REGISTERS_VIEW_H
#define REGISTERS_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class Architecture;


class RegistersView : public BGroupView, private TableListener {
public:
								RegistersView(Architecture* architecture);
								~RegistersView();

	static	RegistersView*		Create(Architecture* architecture);
									// throws

			void				SetCpuState(CpuState* cpuState);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class RegisterValueColumn;
			class RegisterTableModel;

private:
	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

			void				_Init();

private:
			Architecture*		fArchitecture;
			CpuState*			fCpuState;
			Table*				fRegisterTable;
			RegisterTableModel*	fRegisterTableModel;
};


#endif	// REGISTERS_VIEW_H
