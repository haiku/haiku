/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include "CalcEngine.h"
#include <string.h>
#include <Beep.h>

CalcEngine::CalcEngine(int max_digits_in)
{
	max_digits = max_digits_in;
	Clear();
}


void CalcEngine::Clear()
{
	trig_inverse_active = false;
	trig_is_radians = false;
	already_have_infix = false;
	number_stack_index = 0;
	symbol_stack_index = 0;
	keyed_value = 0.0;
	text[0] = '\0';
}


CalcEngine::~CalcEngine()
{
}


double CalcEngine::GetValue(char *str)
{
	if (strlen(text) == 0)
	{
		if (str != NULL)
		{
			int precision = max_digits+1;
			char format[10];
			do
			{
				precision--;
				sprintf(format, "%%.%dg", precision);
				sprintf(str, format, keyed_value);
			} while (!TextWidthOK(str));
		}
		return keyed_value;
	}
	else
	{
		if (str != NULL)
			strcpy(str, text);
		return atof(text);
	}
}


bool CalcEngine::TextWidthOK(const char *str)
{
	return (int)strlen(str) < max_digits+1 ? true : false;
}


void CalcEngine::DoCommand(ulong what)
{
	bool is_infix_symbol = false;
	int i;
	double d;

	switch (what)
	{
		case 'clr':
			if (keyed_value == 0 && strlen(text)==0)
			{
				// Clear was just pressed before, so reset everything.
				Clear();
			}
			SetValue(0.0);
			break;
		case '¹':
		case 'p':
		case 'pi':
			SetValue(M_PI);
			break;
		case 'e':
			SetValue(M_E);
			break;
		case '-/+':
			if (strlen(text)==0) keyed_value = -keyed_value;
			else
			{
				// Must stick a minus sign in front of the text or
				// after 'e' in exponential form.
				char *p = strchr(text, 'e');
				if (p == NULL) p = text;
				else p += 1;	// one char after 'e'
				
				// p now points to insertion point
				if (p[0] == '-')
					p[0] = '+';
				else if (p[0] == '+')
					p[0] = '-';
				else
				{
					memmove(p+1, p, strlen(p)+1);
					p[0] = '-';
				}
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			Append(what);
			break;
		case 'exp':
			if (strlen(text) > 0 && strchr(text,'e')==NULL)
				Append('e');
			else
				beep();
			break;
		case '.':
			if (strlen(text) == 0)
				SetValue("0.");
			else if (strchr(text,'.')==NULL && strchr(text,'e')==NULL)
				Append('.');
			else beep();
			break;
		case '(':
			PushSymbol(what);
			SetValue(0.0);
			break;
		case ')':
			PushKeyedValue();
			while (symbol_stack_index > 0
				   && symbol_stack[symbol_stack_index-1] != '(') // )
			{
				d = DoStackedCalculation();
			}
			if (symbol_stack_index > 0
				&& symbol_stack[symbol_stack_index-1] == '(') // )
			  symbol_stack_index--;
			SetValue(number_stack[--number_stack_index]);
			break;
		case '+':
		case '-':
			is_infix_symbol = true;
			if (already_have_infix) symbol_stack_index--;
			else PushKeyedValue();
			d = GetValue();
			while (symbol_stack_index > 0
				   && (symbol_stack[symbol_stack_index-1] == '+'
				   	   || symbol_stack[symbol_stack_index-1] == '-'
				   	   || symbol_stack[symbol_stack_index-1] == '*'
				   	   || symbol_stack[symbol_stack_index-1] == '/'
				   	   || symbol_stack[symbol_stack_index-1] == '^'
				   	   || symbol_stack[symbol_stack_index-1] == '^1/y'))
			{
				d = DoStackedCalculation();
			}
			SetValue(d);
			PushSymbol(what);
			break;
		case '*':
		case '/':
			is_infix_symbol = true;
			if (already_have_infix) symbol_stack_index--;
			else PushKeyedValue();
			d = GetValue();
			while (symbol_stack_index > 0
				   && (symbol_stack[symbol_stack_index-1] == '*'
				   	   || symbol_stack[symbol_stack_index-1] == '/'
				   	   || symbol_stack[symbol_stack_index-1] == '^'
				   	   || symbol_stack[symbol_stack_index-1] == '^1/y'))
			{
				d = DoStackedCalculation();
			}
			SetValue(d);
			PushSymbol(what);
			break;
		case '^':
		case '^1/y':
			is_infix_symbol = true;
			if (already_have_infix) symbol_stack_index--;
			else PushKeyedValue();
			d = GetValue();
			while (symbol_stack_index > 0
				   && (symbol_stack[symbol_stack_index-1] == '^'
				   	   || symbol_stack[symbol_stack_index-1] == '^1/y'))
			{
				d = DoStackedCalculation();
			}
			SetValue(d);
			PushSymbol(what);
			break;
		case 'x^2':
			d = GetValue();
			SetValue(d * d);
			break;
		case 'sqrt':
			SetValue(sqrt(GetValue()));
			break;
		case '10^x':
			SetValue(pow(10.0, GetValue()));
			break;
		case 'log':
			SetValue(log10(GetValue()));
			break;
		case 'e^x':
			SetValue(exp(GetValue()));
			break;
		case 'ln x':
			SetValue(log(GetValue()));
			break;
		case '1/x':
			SetValue(1.0 / GetValue());
			break;
		case '!':
			d = GetValue();
			if (d > 1000) d = 1000;
			else if (d < -1000) d = -1000;

			if (d >= 0)
			{
				i = (int)(d + 0.0000000001);
				d = 1.0;
			}
			else
			{
				i = (int)(-d + 0.0000000001);
				d = -1.0;
			}
			while (i > 0)
			{
				d *= i;
				i--;
			}
			SetValue(d);
			break;
		case 'sin':
			if (trig_inverse_active)
				SetValue(TrigAngleOutput(asin(GetValue())));
			else
				SetValue(sin(TrigAngleInput(GetValue())));
			break;
		case 'cos':
			if (trig_inverse_active)
				SetValue(TrigAngleOutput(acos(GetValue())));
			else
				SetValue(cos(TrigAngleInput(GetValue())));
			break;
		case 'tan':
			if (trig_inverse_active)
				SetValue(TrigAngleOutput(atan(GetValue())));
			else
				SetValue(tan(TrigAngleInput(GetValue())));
			break;
		case '=':
		case 'entr':
			if (already_have_infix) symbol_stack_index--;
			else PushKeyedValue();
			while (symbol_stack_index > 0)
			{
				d = DoStackedCalculation();
			}
			d = number_stack[--number_stack_index];
			number_stack_index = 0;
			SetValue(d);
			break;
		case 'trgA':
			trig_inverse_active = !trig_inverse_active;
			break;
		case 'trgM':
			trig_is_radians = !trig_is_radians;
			break;
	}

	already_have_infix = is_infix_symbol;

	if (what != 'trgA' && trig_inverse_active)
	{
		trig_inverse_active = !trig_inverse_active;
	}

}


void CalcEngine::PushKeyedValue()
{
	double d = GetValue();
	number_stack[number_stack_index++] = d;
	SetValue(d);
}


void CalcEngine::PushSymbol(ulong sym)
{
	symbol_stack[symbol_stack_index++] = sym;
}


double CalcEngine::DoStackedCalculation()
{
	ulong sym = symbol_stack[--symbol_stack_index];
	double right_number = number_stack[--number_stack_index];
	double left_number = number_stack[--number_stack_index];
	double result;

	switch (sym)
	{
		case '+':
			result = left_number + right_number;
			break;
		case '-':
			result = left_number - right_number;
			break;
		case '*':
			result = left_number * right_number;
			break;
		case '/':
			result = left_number / right_number;
			break;
		case '^':
			result = pow(left_number, right_number);
			break;
		case '^1/y':
			result = pow(left_number, 1.0/right_number);
			break;
		default:
			// bogus !
			result = 0;
			break;
	}

	number_stack[number_stack_index++] = result;

	return result;
}

