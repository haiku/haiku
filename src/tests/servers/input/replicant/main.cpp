// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//
//  Program:	 desklink
//  Author:      Jérôme DUVAL
//  Description: MethodReplicant Test
//  Created :    October 20, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Application.h>
#include <Deskbar.h>
#include <Roster.h>
#include <stdio.h>
#include "MethodReplicant.h"

const char *app_signature = "application/x-vnd.obos.testmethodreplicant";
// the application signature used by the replicant to find the supporting
// code

extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new MethodReplicant(app_signature);
}

int main(int argc, char **argv) 
{	
	BApplication app(app_signature);
			
	app_info info;
	app.GetAppInfo(&info);
		
	status_t err = BDeskbar().AddItem(&info.ref);
	
	if(err!=B_OK) {
		printf("desklink: Deskbar refuses link\n");	
		return 1;
	}
		
	return 0;
}
