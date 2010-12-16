/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_STATE_H
#define VARIABLES_VIEW_STATE_H


#include <Referenceable.h>
#include <util/OpenHashTable.h>


class ObjectID;
class StackFrameValues;
class TypeComponentPath;


class VariablesViewNodeInfo {
public:
								VariablesViewNodeInfo();
								VariablesViewNodeInfo(
									const VariablesViewNodeInfo& other);

			VariablesViewNodeInfo& operator=(
									const VariablesViewNodeInfo& other);

			bool				IsNodeExpanded() const
									{ return fNodeExpanded; }
			void				SetNodeExpanded(bool expanded);

private:
			bool				fNodeExpanded;
};


class VariablesViewState : public BReferenceable {
public:
								VariablesViewState();
	virtual						~VariablesViewState();

			status_t			Init();

			StackFrameValues*	Values() const
									{ return fValues; }
			void				SetValues(StackFrameValues* values);

			const VariablesViewNodeInfo* GetNodeInfo(ObjectID* variable,
									const TypeComponentPath* path) const;
	inline	const VariablesViewNodeInfo* GetNodeInfo(ObjectID* variable,
									const TypeComponentPath& path) const;

			status_t			SetNodeInfo(ObjectID* variable,
									TypeComponentPath* path,
									const VariablesViewNodeInfo& info);
									// requires an on-heap path

private:
			struct Key;
			struct InfoEntry;
			struct InfoEntryHashDefinition;

			typedef BOpenHashTable<InfoEntryHashDefinition> NodeInfoTable;

private:
			void				_Cleanup();

private:
			NodeInfoTable*		fNodeInfos;
			StackFrameValues*	fValues;
};


const VariablesViewNodeInfo*
VariablesViewState::GetNodeInfo(ObjectID* variable,
	const TypeComponentPath& path) const
{
	return GetNodeInfo(variable, &path);
}


#endif	// VARIABLES_VIEW_STATE_H
