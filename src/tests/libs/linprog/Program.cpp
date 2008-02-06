
#include <List.h>
#include <SupportDefs.h>

#include "Constraint.h"
#include "LinearSpec.h"
#include "OperatorType.h"
#include "Variable.h"

#include <stdio.h>


void PrintVars(LinearSpec* ls) {
	int32 size = ls->Variables()->CountItems();
	Variable* variable;
	for (int i = 0; i < size; i++) {
		variable = (Variable*)ls->Variables()->ItemAt(i);
		printf("%f ", variable->Value());
	}
	printf("%d\n", ls->Result());
}


void Test1() {
	LinearSpec* ls = new LinearSpec();
	Variable* x1 = ls->AddVariable();
	Variable* x2 = ls->AddVariable();
	
	BList* coeffs1 = new BList(1);
	coeffs1->AddItem(new double(1.0));
	BList* vars1 = new BList(1);
	vars1->AddItem(x1);
	Constraint* c1 = ls->AddConstraint(coeffs1, vars1, OperatorType(LE), 108);
	
	BList* coeffs2 = new BList(1);
	coeffs2->AddItem(new double(1.0));
	BList* vars2 = new BList(1);
	vars2->AddItem(x2);
	Constraint* c2 = ls->AddConstraint(coeffs2, vars2, OperatorType(GE), 113);
	
	BList* coeffs3 = new BList(2);
	coeffs3->AddItem(new double(1.0));
	coeffs3->AddItem(new double(1.0));
	BList* vars3 = new BList(2);
	vars3->AddItem(x1);
	vars3->AddItem(x2);
	ls->SetObjFunction(coeffs3, vars3);
	ls->Solve();
	PrintVars(ls);
	
	delete c2;
	ls->Solve();
	PrintVars(ls);
	
	BList* coeffs4 = new BList(1);
	coeffs4->AddItem(new double(1.0));
	BList* vars4 = new BList(1);
	vars4->AddItem(x2);
	c2 = ls->AddConstraint(coeffs4, vars4, OperatorType(GE), 113);
	ls->Solve();
	PrintVars(ls);
}


int main() {
	Test1();
}
