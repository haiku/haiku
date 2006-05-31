/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef __CALC_ENGINE__
#define __CALC_ENGINE__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned long ulong;

class CalcEngine
{
public:
			CalcEngine(int max_digits = 12);
	virtual	~CalcEngine();

	void	DoCommand(ulong key);
	double	GetValue(char *val=NULL);
	void	Clear(void);

	bool	TrigInverseActive() { return trig_inverse_active; };
	bool	TrigIsRadians() { return trig_is_radians; };

	virtual bool TextWidthOK(const char *text);

private:
	
	enum	{ max_stack_size = 20 };
	enum	{ mode_start=0, mode_input };

	char	text[64];
	double	keyed_value;
	
	void	SetValue(double d) { keyed_value=d; text[0]='\0'; };
	void	SetValue(const char *str) { keyed_value=0; strcpy(text,str);};
	void	Append(char c) { char *p=text+strlen(text); *p++=c; *p='\0'; };

	double	DoStackedCalculation();
	void	PushKeyedValue();
	void	PushSymbol(ulong sym);

	int		max_digits;
	bool	trig_inverse_active;
	bool	trig_is_radians;
	bool	already_have_infix;

	int		number_stack_index;
	double	number_stack[max_stack_size];
	int		symbol_stack_index;
	ulong	symbol_stack[max_stack_size];

	double	TrigAngleInput(double d) { return trig_is_radians?d:M_PI/180*d; };
	double	TrigAngleOutput(double d) { return trig_is_radians?d:180/M_PI*d; };
};

#endif
