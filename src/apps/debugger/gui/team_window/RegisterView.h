/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef REGISTER_VIEW_H
#define REGISTER_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class Architecture;


class RegisterView : public BGroupView, private TableListener {
public:
								RegisterView(Architecture* architecture);
								~RegisterView();

	static	RegisterView*		Create(Architecture* architecture);
									// throws

			void				SetCpuState(CpuState* cpuState);

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


#endif	// REGISTER_VIEW_H
