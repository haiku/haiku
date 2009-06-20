/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugModel.h"

#include <new>

#include <AutoLocker.h>


// #pragma mark - TeamDebugModel


TeamDebugModel::TeamDebugModel(Team* team, DebuggerInterface* debuggerInterface,
	Architecture* architecture)
	:
	fTeam(team),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture)
{
}


TeamDebugModel::~TeamDebugModel()
{
}


status_t
TeamDebugModel::Init()
{
	return B_OK;
}


void
TeamDebugModel::AddListener(Listener* listener)
{
	AutoLocker<TeamDebugModel> locker(this);
	fListeners.Add(listener);
}


void
TeamDebugModel::RemoveListener(Listener* listener)
{
	AutoLocker<TeamDebugModel> locker(this);
	fListeners.Remove(listener);
}


// #pragma mark - Event


TeamDebugModel::Event::Event(uint32 type, TeamDebugModel* model)
	:
	fEventType(type),
	fModel(model)
{
}


// #pragma mark - Listener


TeamDebugModel::Listener::~Listener()
{
}
