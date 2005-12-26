/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <WindowScreen.h>
#include <Screen.h>
#include <string.h>

typedef long (*blit_hook)(long,long,long,long,long,long);
typedef long (*sync_hook)();

class NApplication:public BApplication {
public:
   NApplication();
   bool is_quitting; // So that the WindowScreen knows what
                     //       to do when disconnected.
private:
   bool QuitRequested();
   void ReadyToRun();
};

class NWindowScreen:public BWindowScreen {
public:
   NWindowScreen(status_t*);
private:
   void ScreenConnected(bool);
   long MyCode();
   static long Entry(void*);
   thread_id tid;
   sem_id sem;
   area_id area;
   uint8* save_buffer;
   uint8* frame_buffer;
   ulong line_length;
   bool thread_is_locked; // small hack to allow to quit the
                          //     app from ScreenConnected()
   blit_hook blit; // hooks to the graphics driver functions
   sync_hook sync;
};

int main(int, char**) {
   NApplication app;
}

NApplication::NApplication()
      :BApplication("application/x-vnd.Be-sample-jbq1") {
   Run(); // see you in ReadyToRun()
}

void NApplication::ReadyToRun() {
   status_t ret=B_ERROR;
   is_quitting=false;
   NWindowScreen* ws=new NWindowScreen(&ret);
// exit if constructing the WindowScreen failed.
   if ((ws==NULL)||(ret<B_OK))
      PostMessage(B_QUIT_REQUESTED);
}

bool NApplication::QuitRequested() {
   is_quitting=true;
   return true;
}

NWindowScreen::NWindowScreen(status_t* ret)
      :BWindowScreen("Example",B_8_BIT_640x480,ret) {
   thread_is_locked=true;
   tid=0;
   if (*ret==B_OK) {
// this semaphore controls the access to the WindowScreen
      sem=create_sem(0,"WindowScreen Access");
// this area is used to save the whole framebuffer when
//       switching workspaces. (better than malloc()).
      area=create_area("save",(void**)&save_buffer,B_ANY_ADDRESS
            ,640*2048,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA);
// exit if an error occured.
      if ((sem<B_OK)||(area<B_OK)) *ret=B_ERROR;
      else Show(); // let's go. See you in ScreenConnected.
   }
}

void NWindowScreen::ScreenConnected(bool connected) {
   if (connected) {
      if ((SetSpace(B_8_BIT_640x480)<B_OK)
               ||(SetFrameBuffer(640,2048)<B_OK)) {
// properly set the framebuffer. exit if an error occurs.
         be_app->PostMessage(B_QUIT_REQUESTED);
         return;
      }
// get the hardware acceleration hooks. get them each time
// the WindowScreen is connected, because of multiple
// monitor support
      blit=(blit_hook)CardHookAt(7);
      sync=(sync_hook)CardHookAt(10);
// cannot work with no hardware blitting
      if (blit==NULL) {
         be_app->PostMessage(B_QUIT_REQUESTED);
         return;
      }
// get the framebuffer-related info, each time the
// WindowScreen is connected (multiple monitor)
      frame_buffer=(uint8*)(CardInfo()->frame_buffer);
      line_length=FrameBufferInfo()->bytes_per_row;
      if (tid==0) {
// clean the framebuffer
        memset(frame_buffer,0,2048*line_length);
// spawn the rendering thread. exit if an error occurs.
         if (((tid=spawn_thread(Entry,"rendering thread",
                     B_URGENT_DISPLAY_PRIORITY,this))<B_OK)
               ||(resume_thread(tid)<B_OK))
            be_app->PostMessage(B_QUIT_REQUESTED);
      } else for (int y=0;y<2048;y++)
// restore the framebuffer when switching back from
// another workspace.
            memcpy(frame_buffer+y*line_length,
                  save_buffer+640*y,640);
// set our color list.
	  rgb_color palette[256];
      for (int i=0;i<128;i++) {
         rgb_color c1={i*2,i*2,i*2};
         rgb_color c2={127+i,2*i,254};
		 palette[i]=c1;
		 palette[i+128]=c2;
      }
	  SetColorList(palette);
// allow the rendering thread to run.
      thread_is_locked=false;
      release_sem(sem);
   } else {
// block the rendering thread.
      if (!thread_is_locked) {
         acquire_sem(sem);
         thread_is_locked=true;
      }
// kill the rendering and clean up when quitting
      if ((((NApplication*)be_app)->is_quitting)) {
         status_t ret;
         kill_thread(tid);
         wait_for_thread(tid,&ret);
         delete_sem(sem);
         delete_area(area);
      } else {
// set the color list black so that the screen doesn't seem
// to freeze while saving the framebuffer
         rgb_color c={0,0,0};
		 rgb_color palette[256];
         for (int i=0;i<256;i++)
			palette[i]=c;
         SetColorList(palette);

// save the framebuffer
         for (int y=0;y<2048;y++)
            memcpy(save_buffer+640*y,
                  frame_buffer+y*line_length,640);
      }
   }
}

long NWindowScreen::Entry(void* p) {
   return ((NWindowScreen*)p)->MyCode();
}

long NWindowScreen::MyCode() {
// gain access to the framebuffer before writing to it.

   acquire_sem(sem);

   for (int j=1440;j<2048;j++) {
      for (int i=0;i<640;i++) {
// draw the backgroud ripple pattern
         float val=63.99*(1+cos(2*M_PI*((i-320)*(i-320)
               +(j-1744)*(j-1744))/1216));
         frame_buffer[i+line_length*j]=int(val);
      }
   }
   ulong numframe=0;
   bigtime_t trgt=0;
   ulong y_origin;
   uint8* current_frame;
   while(true) {
// the framebuffer coordinates of the next frame
      y_origin=480*(numframe%3);
// and a pointer to it
      current_frame=frame_buffer+y_origin*line_length;
// copy the background
      int ytop=numframe%608,ybot=ytop+479;
      if (ybot<608) {
	      blit(0,1440+ytop,0,y_origin,639,479);
      } else {
	      blit(0,1440+ytop,0,y_origin,639,1086-ybot);
	      blit(0,1440,0,y_origin+1087-ybot,639,ybot-608);
      }
// calculate the circle position. doing such calculations
// between blit() and sync() is a good idea.
      uint32 x=(uint32)(287.99*(1+sin(numframe/72.)));
      uint32 y=(uint32)(207.99*(1+sin(numframe/52.)));
      if (sync) sync();
// draw the circle
      for (int j=0;j<64;j++) {
         for (int i=0;i<64;i++) {
            if ((i-31)*(i-32)+(j-31)*(j-32)<=1024)
               current_frame[x+i+line_length*(y+j)]+=128;
         }
      }
// release the semaphore while waiting. gotta release it
// at some point or nasty things will happen!
      release_sem(sem);
// try to sync with the vertical retrace
      if (BScreen(this).WaitForRetrace()!=B_OK) {
// we're doing some triple buffering. unwanted things would
// happen if we rendered more pictures than the card can
// display. we here make sure not to render more than 55.5 
// pictures per second if the card does not support retrace
// syncing
         if (system_time()<trgt) snooze(trgt-system_time());
         trgt=system_time()+18000;
	  }
// acquire the semaphore back before talking to the driver
      acquire_sem(sem);
// do the page-flipping
      MoveDisplayArea(0,y_origin);
// and go to the next frame!
      numframe++;
   }
   return 0;
}
