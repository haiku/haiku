#include "router.h"
#include <InterfaceDefs.h>
#include <String.h>
#include <View.h>
#include <AppDefs.h>
#include <stdlib.h>
#include <stdio.h>
#include <Message.h>
#include <Roster.h>

#include "PortLink.h"
#include "ServerProtocol.h"
//#define SERVER_PORT_NAME "OBappserver"
//#define SERVER_INPUT_PORT "OBinputport"


/*******************************************************
*   
*******************************************************/
BInputServerFilter* instantiate_input_filter(){
   return (new RouterInputFilter());
}

/*******************************************************
*   
*******************************************************/
RouterInputFilter::RouterInputFilter():BInputServerFilter(){
   port_id pid = find_port(SERVER_INPUT_PORT);
   serverlink = new PortLink(pid);
   
}

/*******************************************************
*   
*******************************************************/
RouterInputFilter::~RouterInputFilter(){
}

/*******************************************************
*   Everthing went just find .. right?
*******************************************************/
status_t RouterInputFilter::InitCheck(){
   return B_OK;
}



/*******************************************************
*   
*******************************************************/
filter_result RouterInputFilter::Filter(BMessage *message, BList *outList){
   if(serverlink == NULL){
      return B_DISPATCH_MESSAGE;
   }
   port_id pid = find_port(SERVER_INPUT_PORT);
   if(pid < 0){
      return B_DISPATCH_MESSAGE;
   }   
   serverlink->SetPort(pid);
   
   switch(message->what){
   case B_MOUSE_MOVED:{
      port_id pid = find_port(SERVER_INPUT_PORT);
       if(pid==B_NAME_NOT_FOUND)
       	break;
      serverlink->SetPort(pid);
      BPoint p;
      uint32 buttons = 0;
      // get piont and button from msg
      if(message->FindPoint("where",&p) != B_OK){
         
      }
      int64 time=(int64)real_time_clock();
      
      serverlink->SetOpCode(B_MOUSE_MOVED);
      serverlink->Attach(&time,sizeof(int64));
      serverlink->Attach(&p.x,sizeof(float));
      serverlink->Attach(&p.y,sizeof(float));
      serverlink->Attach(&buttons,sizeof(int32));

      // prevent server crashes from hanging the Input Server
      serverlink->Flush(50000);
      }break;
   case B_MOUSE_UP:{
        port_id pid = find_port(SERVER_INPUT_PORT);
        if(pid==B_NAME_NOT_FOUND)
        	break;
        serverlink->SetPort(pid);
		BPoint p;
	
		uint32 mod=modifiers();
		// Eventually, we will also be sending a uint32 buttons.
	
		int64 time=(int64)real_time_clock();
	
        message->FindPoint("where",&p);
	
		serverlink->SetOpCode(B_MOUSE_UP);
		serverlink->Attach(&time, sizeof(int64));
		serverlink->Attach(&p.x,sizeof(float));
		serverlink->Attach(&p.y,sizeof(float));
		serverlink->Attach(&mod, sizeof(uint32));

        // prevent server crashes from hanging the Input Server
        serverlink->Flush(50000);
      }break;
   case B_MOUSE_DOWN:{
        port_id pid = find_port(SERVER_INPUT_PORT);
        if(pid==B_NAME_NOT_FOUND)
        	break;
        serverlink->SetPort(pid);
		BPoint p;
	
		uint32 buttons,
			mod=modifiers();
		int32 clicks;
		message->FindInt32("clicks",&clicks);
        message->FindPoint("where",&p);
        message->FindInt32("buttons",(int32*)&buttons);
	
		int64 time=(int64)real_time_clock();

		serverlink->SetOpCode(B_MOUSE_DOWN);
		serverlink->Attach(&time, sizeof(int64));
		serverlink->Attach(&p.x,sizeof(float));
		serverlink->Attach(&p.y,sizeof(float));
		serverlink->Attach(&mod, sizeof(uint32));
		serverlink->Attach(&buttons, sizeof(uint32));
		serverlink->Attach(&clicks, sizeof(uint32));

        // prevent server crashes from hanging the Input Server
        serverlink->Flush(50000);
      }break;

   // Should be some Mouse Down and Up code here ..
   // Along with some Key Down and up codes ..
   default:
      break;
   }
   
   //delete serverlink;
   
   // Let all msg flow normally to the 
   // Be app_server
   return B_DISPATCH_MESSAGE;
}
