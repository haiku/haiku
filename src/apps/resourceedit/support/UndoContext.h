/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef UNDO_CONTEXT_H
#define UNDO_CONTEXT_H


#include <List.h>
#include <String.h>


class UndoContext {
public:
	class Action {
	public:
						Action(const BString& label);
		virtual			~Action();

		virtual	void	Do();
		virtual	void	Undo();

		void			SetLabel(const BString& label);
		BString			Label() const;

	private:
		BString			fLabel;
	};

				UndoContext();
				~UndoContext();

	void		SetLimit(int32 limit);
	int32		Limit() const;

	void		Do(Action* action);

	void 		Undo();
	void 		Redo();

	bool		CanUndo() const;
	bool		CanRedo() const;

	BString		UndoLabel() const;
	BString		RedoLabel() const;

private:
	int32		fAt;
	int32		fLimit;
	BList* 		fHistory;

};


#endif
